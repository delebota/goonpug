/* Copyright (c) 2014 Astroman Technologies LLC
 *
 * This file is part of GoonPUG.
 *
 * GoonPUG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundataion, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GoonPUG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GoonPUG.  If not, see <http://www.gnu.org/licenses/>.
 *
 * vim: set ts=4 et ft=sourcepawn :
 */
/**
 * GoonPUG TrueSkill module
 *
 * Author: Peter "astroman" Rowlands <peter@pmrowla.com>
 */

#include <gp_gaussian_functions>

// Convars
new Handle:hGpSkillShowRating = INVALID_HANDLE;
new Handle:hGpSkillConn = INVALID_HANDLE;

// Default stats
new Float:defaultStats[3];
new Float:defaultMean = 25.00;
new Float:defaultStandardDeviation = 8.333;
new Float:defaultConservativeRating = 0.00;
new Float:defaultBeta = 4.1665;
new Float:defaultDynamicsFactor = 0.08333;
new Float:defaultDrawProbability = 0.10;

new Float:conservativeRatingMultiplier = 3.00;

// enums
enum MatchOutcome
{
    LOSE = -1.0,
    DRAW = 0.0,
    WIN = 1.0,
};

GpSkill_Init()
{
    hGpSkillShowRating = CreateConVar("gp_skill_display_postmatch", "1",
            "Show the user their new ranking after each match.", FCVAR_SPONLY);
            
    defaultStats[0] = defaultMean;
    defaultStats[1] = defaultStandardDeviation;
    defaultStats[2] = defaultConservativeRating;
    
    decl String:error[256];
    if (hGpSkillConn == INVALID_HANDLE)
    {
        hGpSkillConn = SQL_Connect("gpskill", true, error, sizeof(error));
        if (hGpSkillConn == INVALID_HANDLE)
        {
            LogError("Could not connect to gpskill db: %s", error);
        }
    }
    
    decl String:query[256];
    Format(query, sizeof(query), "CREATE TABLE IF NOT EXISTS gpskill (steam_id VARCHAR (32) NOT NULL PRIMARY KEY, mean REAL NOT NULL DEFAULT (25.00), deviation REAL NOT NULL DEFAULT (8.333), rating REAL NOT NULL DEFAULT (0.00));");
    new Handle:hResult = SQL_Query(hGpSkillConn, query);
    if (hResult == INVALID_HANDLE)
    {
        if (SQL_GetError(hGpSkillConn, error, sizeof(error)))
        {
            LogError("Could not create gpskill table: %s", error);
        }
        else
        {
            LogError("Could not create gpskill table.");
        }
    }
    
    CloseHandle(hResult);
}

GpSkill_Fini()
{
    if (hGpSkillShowRating != INVALID_HANDLE)
        CloseHandle(hGpSkillShowRating);
    if (hGpSkillConn != INVALID_HANDLE)
        CloseHandle(hGpSkillConn);
}

bool:GpSkill_Show_Rating()
{
    if (hGpSkillShowRating == INVALID_HANDLE)
        return false;

    new enabled = GetConVarInt(hGpSkillShowRating);
    if (0 == enabled)
        return false;
    else
        return true;
}

Float:GpSkill_FetchPlayerStats(const String:auth[])
{
    decl String:error[256];
    if (hGpSkillConn == INVALID_HANDLE)
    {
        hGpSkillConn = SQL_Connect("gpskill", true, error, sizeof(error));
        if (hGpSkillConn == INVALID_HANDLE)
        {
            LogError("Could not connect to gpskill db: %s", error);
            return defaultStats;
        }
    }
    
    decl String:query[256];
    Format(query, sizeof(query), "SELECT mean, deviation, rating FROM gpskill WHERE steam_id = '%s' LIMIT 1;", auth);
    new Handle:hResult = SQL_Query(hGpSkillConn, query);
    if (hResult == INVALID_HANDLE)
    {
        if (SQL_GetError(hGpSkillConn, error, sizeof(error)))
        {
            LogError("Could not retrieve stats for %s: %s", auth, error);
        }
        else
        {
            LogError("Could not retrieve stats for %s", auth);
        }
        
        return defaultStats;
    }
    
    new Float:stats[3];
    if (SQL_FetchRow(hResult))
    {
        stats[0] = SQL_FetchFloat(hResult, 0); // mean
        stats[1] = SQL_FetchFloat(hResult, 1); // deviation
        stats[2] = SQL_FetchFloat(hResult, 2); // conservative rating
        
        CloseHandle(hResult);
        
        return stats;
    }
    else
    {
        PrintToServer("No stats result for %s. Creating default record.", auth);
        decl String:defRecordQuery[256];
        Format(defRecordQuery, sizeof(defRecordQuery), "INSERT INTO gpskill (steam_id, mean, deviation, rating) VALUES ('%s', %.2f,  %.3f, %.2f);", auth, defaultStats[0], defaultStats[1], defaultStats[2]);
        SQL_Query(hGpSkillConn, defRecordQuery);
        
        CloseHandle(hResult);
        
        return defaultStats;
    }
}

GpSkill_SavePlayerStats(const String:auth[], Float:mean, Float:deviation, Float:conservativeRating)
{
    decl String:error[512];
    if (hGpSkillConn == INVALID_HANDLE)
    {
        hGpSkillConn = SQL_Connect("gpskill", true, error, sizeof(error));
        if (hGpSkillConn == INVALID_HANDLE)
        {
            LogError("Could not connect to gpskill db: %s", error);
        }
    }
    
    decl String:query[256];
    Format(query, sizeof(query), "UPDATE gpskill SET mean = %.6f, deviation = %.6f, rating = %.6f WHERE steam_id = '%s';", mean, deviation, conservativeRating, auth);
    new Handle:hResult = SQL_Query(hGpSkillConn, query);
    if (hResult == INVALID_HANDLE)
    {
        if (SQL_GetError(hGpSkillConn, error, sizeof(error)))
        {
            LogError("Could not update stats for %s: %s", auth, error);
        }
        else
        {
            LogError("Could not update stats for %s", auth);
        }
    }
    else
    {
        CloseHandle(hResult);
    }
}

public Float:GpSkill_MatchQuality(Handle:handleTeam1, Handle:handleTeam2)
{
    int totalPlayers = GetArraySize(handleTeam1) + GetArraySize(handleTeam2);
    new Float:betaSquared = defaultBeta * defaultBeta;

    new Float:team1Stats[2];
    team1Stats = GetTeamStatsSums(handleTeam1);
    new Float:team2Stats[2];
    team2Stats = GetTeamStatsSums(handleTeam2);

    new Float:sqrtPart = Float:SquareRoot((totalPlayers * betaSquared) / ((totalPlayers * betaSquared) + team1Stats[1] + team2Stats[1]));
    new Float:meanSumSquared = (team1Stats[0] - team2Stats[0]) * (team1Stats[0] - team2Stats[0]);
    new Float:expPart = Float:Exponential((-1.0 * meanSumSquared) / (2.0 * ((totalPlayers * betaSquared) + team1Stats[1] + team2Stats[1])));
    
    return (expPart * sqrtPart);
}

public GpSkill_RunTrueskill(Handle:handleTeam1, Handle:handleTeam2, int teamWinner)
{
    PrintToServer("[GP] Running Trueskill update.");
    
    new MatchOutcome:team1Outcome;
    new MatchOutcome:team2Outcome;
    switch (teamWinner)
    {
        case 0:
        {
            team1Outcome = DRAW;
            team2Outcome = DRAW;
        }
        case 1:
        {
            team1Outcome = WIN;
            team2Outcome = LOSE;
        }
        case 2:
        {
            team1Outcome = LOSE;
            team2Outcome = WIN;
        }
    }
    
    new Float:team1Stats[2];
    team1Stats = GetTeamStatsSums(handleTeam1);
    
    new Float:team2Stats[2];
    team2Stats = GetTeamStatsSums(handleTeam2);
    
    GpSkill_UpdatePlayerRatings(handleTeam1, team1Stats, handleTeam2, team2Stats, team1Outcome);
    GpSkill_UpdatePlayerRatings(handleTeam2, team2Stats, handleTeam1, team1Stats, team2Outcome);
}

public GpSkill_UpdatePlayerRatings(Handle:thisTeam, Float:thisTeamStats[], Handle:otherTeam, Float:otherTeamStats[], MatchOutcome:matchOutcome)
{
    new Float:drawMargin = GetDrawMarginFromDrawProbability(defaultDrawProbability, defaultBeta);
    new Float:betaSquared = (defaultBeta * defaultBeta);
    new Float:tauSquared = (defaultDynamicsFactor * defaultDynamicsFactor);

    int totalPlayers = GetArraySize(thisTeam) + GetArraySize(otherTeam);

    new Float:totalVarianceSum = 0.0;
    new Float:thisTeamMeanSum = 0.0;
    new Float:otherTeamMeanSum = 0.0;
    
    thisTeamMeanSum = thisTeamStats[0];
    totalVarianceSum += thisTeamStats[1];
    
    otherTeamMeanSum = otherTeamStats[0];
    totalVarianceSum += otherTeamStats[1];
    
    new Float:c = Float:SquareRoot(totalVarianceSum + (totalPlayers * betaSquared));
    
    new Float:winningMean = thisTeamMeanSum;
    new Float:losingMean = otherTeamMeanSum;

    switch (matchOutcome)
    {
        case WIN:{}
        case DRAW:{}
        case LOSE:
        {
            winningMean = otherTeamMeanSum;
            losingMean = thisTeamMeanSum;
        }
    }

    new Float:meanDelta = winningMean - losingMean;
    
    new Float:v;
    new Float:w;
    new Float:rankMultiplier;

    if (matchOutcome == WIN)
    {
        // win
        v = GaussianVExceedsMargin(meanDelta / c, drawMargin / c);
        w = GaussianWExceedsMargin(meanDelta / c, drawMargin / c);
        rankMultiplier = 1.0;
    }
    else if (matchOutcome == LOSE)
    {
        // lose
        v = GaussianVExceedsMargin(meanDelta / c, drawMargin / c);
        w = GaussianWExceedsMargin(meanDelta / c, drawMargin / c);
        rankMultiplier = -1.0;
    }
    else
    {
        // draw
        v = GaussianVWithinMargin(meanDelta / c, drawMargin / c);
        w = GaussianWWithinMargin(meanDelta / c, drawMargin / c);
        rankMultiplier = 0.0;
    }
    
    for (int i = 0; i < GetArraySize(thisTeam); i++)
    {
        decl String:auth[STEAMID_LEN];
        GetArrayString(thisTeam, i, auth, sizeof(auth));
        
        new Float:stats[3];
        stats = GpSkill_FetchPlayerStats(auth);
        
        new Float:meanMultiplier = ((stats[1] * stats[1]) + tauSquared) / c;
        new Float:stdDevMultiplier = ((stats[1] * stats[1]) + tauSquared) / (c * c);
        new Float:playerMeanDelta = (rankMultiplier * meanMultiplier * v);
        
        new Float:newMean = stats[0] + playerMeanDelta;
        new Float:newStdDev = Float:SquareRoot(((stats[1] * stats[1]) + tauSquared) * (1.0 - w * stdDevMultiplier));
        new Float:newRating = GpSkill_CalculateConservativeRating(newMean, newStdDev);
        
        // update DB values
        GpSkill_SavePlayerStats(auth, newMean, newStdDev, newRating);
    
        // print new rating and change to user
        if (GpSkill_Show_Rating())
        {
            new client = FindClientByAuthString(auth);
            if (client > 0 && IsClientConnected(client) && IsClientInGame(client) && !IsClientSourceTV(client) && !IsFakeClient(client))
            {
                decl String:msg[64];
                Format(msg, sizeof(msg), "New GPSkill Rating: %.2f (", newRating);
            
                float difference = (newRating - stats[2]);
                if (difference > 0.0)
                    StrCat(msg, sizeof(msg), "+");
                
                StrCat(msg, sizeof(msg), "%.2f)");
                Format(msg, sizeof(msg), msg, difference);
            
                Menu menu = new Menu(GPSkillChangeDisplay);
                menu.SetTitle(msg);
                menu.AddItem("Dismiss", "Dismiss");
                menu.ExitButton = false;
                menu.Display(client, 30);
            }
        }
    }
}

public Float:GpSkill_CalculateConservativeRating(Float:mean, Float:deviation)
{
    return (mean - (conservativeRatingMultiplier * deviation));
}

Float:GetDrawMarginFromDrawProbability(Float:drawProbability, Float:beta)
{
    new Float:inverseCumulative = GaussianInverseCumulativeTo(0.5 * (drawProbability + 1.0));
    return (inverseCumulative * Float:SquareRoot(1.0 + 1.0) * beta);
}

public int GPSkillChangeDisplay(Menu menu, MenuAction action, int param1, int param2)
{
	/* If an option was selected, tell the client about the item. */
	if (action == MenuAction_Select)
	{
		//char info[32];
		//bool found = menu.GetItem(param2, info, sizeof(info));
		//PrintToConsole(param1, "You selected item: %d (found? %d info: %s)", param2, found, info);
	}
	/* If the menu was cancelled, print a message to the server about it. */
	else if (action == MenuAction_Cancel)
	{
		PrintToServer("Client %d's menu was cancelled. Reason: %d", param1, param2);
	}
	/* If the menu has ended, destroy it */
	else if (action == MenuAction_End)
	{
		delete menu;
	}
}

Float:GetTeamStatsSums(Handle:handleTeam)
{
    new Float:totalMean = 0.0;
    new Float:totalStdDev = 0.0;
    
    for (int i = 0; i < GetArraySize(handleTeam); i++)
    {
        decl String:auth[STEAMID_LEN];
        GetArrayString(handleTeam, i, auth, sizeof(auth));
        
        new Float:stats[3];
        stats = GpSkill_FetchPlayerStats(auth);
        
        totalMean += stats[0];
        totalStdDev += (stats[1] * stats[1]);
    }
    
    new Float:totalStats[2];
    totalStats[0] = totalMean;
    totalStats[1] = totalStdDev;
    return totalStats;
}
