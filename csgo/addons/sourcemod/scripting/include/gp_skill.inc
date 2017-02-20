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

new Handle:hGpSkillEnabled = INVALID_HANDLE;
new Handle:hGpSkillShowRating = INVALID_HANDLE;
new Handle:hGpSkillConn = INVALID_HANDLE;

new Float:defaultStats[3];

GpSkill_Init()
{
    hGpSkillEnabled = CreateConVar("gp_skill_enabled", "1",
            "Use GoonPUG skill ranking.", FCVAR_SPONLY);
    
    hGpSkillShowRating = CreateConVar("gp_skill_display_postmatch", "1",
            "Show the user their new ranking after each match.", FCVAR_SPONLY);
            
    defaultStats[0] = 1500.00; // default rating
    defaultStats[1] = 350.00;  // default deviation
    defaultStats[2] = 0.0600;  // default volatility
}

GpSkill_Fini()
{
    if (hGpSkillEnabled != INVALID_HANDLE)
        CloseHandle(hGpSkillEnabled);
    if (hGpSkillShowRating != INVALID_HANDLE)
        CloseHandle(hGpSkillShowRating);
    if (hGpSkillConn != INVALID_HANDLE)
        CloseHandle(hGpSkillConn);
}

bool:GpSkill_Enabled()
{
    if (hGpSkillEnabled == INVALID_HANDLE)
        return false;

    new enabled = GetConVarInt(hGpSkillEnabled);
    if (0 == enabled)
        return false;
    else
        return true;
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
    decl String:error[512];
    if (GpSkill_Enabled())
    {
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
        Format(query, sizeof(query), "SELECT rating, deviation, volatility FROM gpskill WHERE steam_id = '%s' LIMIT 1;", auth);
        new Handle:hResult = SQL_Query(hGpSkillConn, query);

        if (INVALID_HANDLE == hResult)
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
            stats[0] = SQL_FetchFloat(hResult, 0); // rating
            stats[1] = SQL_FetchFloat(hResult, 1); // deviation
            stats[2] = SQL_FetchFloat(hResult, 2); // volatility
            PrintToServer("Got stats for %s: rating = %.02f, deviation = %.02f, volatility = %.04f", auth, stats[0], stats[1], stats[2]);
            
            CloseHandle(hResult);
            
            return stats;
        }
        else
        {
            PrintToServer("No stats result for %s. Creating default record.", auth);
            decl String:defRecordQuery[256];
            Format(defRecordQuery, sizeof(defRecordQuery), "INSERT INTO gpskill (steam_id, rating, deviation, volatility) VALUES ('%s', %.02f,  %.02f, %.04f);", auth, defaultStats[0], defaultStats[1], defaultStats[2]);
            SQL_Query(hGpSkillConn, defRecordQuery);
            
            CloseHandle(hResult);
            
            return defaultStats;
        }
    }
}

GpSkill_UpdatePlayerStats(const String:auth[], float rating, float deviation, float volatility)
{
    decl String:error[512];
    if (GpSkill_Enabled())
    {
        if (hGpSkillConn == INVALID_HANDLE)
        {
            hGpSkillConn = SQL_Connect("gpskill", true, error, sizeof(error));
            if (hGpSkillConn == INVALID_HANDLE)
            {
                LogError("Could not connect to gpskill db: %s", error);
            }
        }

        decl String:query[256];
        Format(query, sizeof(query), "UPDATE gpskill SET rating = %.02f, deviation = %.02f, volatility = %.04f WHERE steam_id = '%s';", rating, deviation, volatility, auth);
        new Handle:hResult = SQL_Query(hGpSkillConn, query);

        if (INVALID_HANDLE == hResult)
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
}

// Implementation of Glicko 2 ranking system. http://www.glicko.net/glicko/glicko2.pdf
// Slightly modified to better represent Team vs Team according to http://rhetoricstudios.com/downloads/AbstractingGlicko2ForTeamGames.pdf
Float:GpSkill_RunGlickoTwo(Handle:handleTeam1, Handle:handleTeam2, int teamWinner)
{
    if (GpSkill_Enabled())
    {
        PrintToServer("[GP] Running Glicko 2 skill update.");
        
        // Who won the match
        float team1Outcome = 0.0;
        float team2Outcome = 0.0;
        if (teamWinner == 0)
        {   //Draw
            team1Outcome = 0.5;
            team2Outcome = 0.5;
        }
        else if (teamWinner == 1)
        {   // Team 1 Wins
            team1Outcome = 1.0;
            team2Outcome = 0.0;
        }
        else if (teamWinner == 2)
        {   // Team 2 Wins
            team1Outcome = 0.0;
            team2Outcome = 1.0;
        }
        
        // Calculate Team 1 average rating and deviation
        float team1RatingTotal = 0.0;
        float team1DeviationTotal = 0.0;
        for (int i = 0; i < GetArraySize(handleTeam1); i++)
        {
            decl String:auth[STEAMID_LEN];
            GetArrayString(handleTeam1, i, auth, STEAMID_LEN);
            new Float:stats[3];
            stats = GpSkill_FetchPlayerStats(auth);
            
            team1RatingTotal += stats[0];
            team1DeviationTotal += stats[1];
        }
        float team1RatingAvg = (team1RatingTotal / GetArraySize(handleTeam1));
        float team1DeviationAvg = (team1DeviationTotal / GetArraySize(handleTeam1));
        
        // Calculate Team 2 average rating and deviation
        float team2RatingTotal = 0.0;
        float team2DeviationTotal = 0.0;
        for (int i = 0; i < GetArraySize(handleTeam2); i++)
        {
            decl String:auth[STEAMID_LEN];
            GetArrayString(handleTeam2, i, auth, STEAMID_LEN);
            new Float:stats[3];
            stats = GpSkill_FetchPlayerStats(auth);
            
            team2RatingTotal += stats[0];
            team2DeviationTotal += stats[1];
        }
        float team2RatingAvg = (team2RatingTotal / GetArraySize(handleTeam2));
        float team2DeviationAvg = (team2DeviationTotal / GetArraySize(handleTeam2));
        
        // Update Team 1
        PrintToServer("[GP] Updating Team 1 skill ratings.");
        for (int i = 0; i < GetArraySize(handleTeam1); i++)
        {
            decl String:auth[STEAMID_LEN];
            GetArrayString(handleTeam1, i, auth, STEAMID_LEN);
            new Float:playerStats[3];
            playerStats = GpSkill_FetchPlayerStats(auth);
            
            new Float:oppTeamStats[3];
            oppTeamStats[0] = team2RatingAvg;
            oppTeamStats[1] = team2DeviationAvg;
            
            GpSkill_CalculateGlicko(auth, playerStats, oppTeamStats, team1Outcome);
        }
        
        // Update Team 2
        PrintToServer("[GP] Updating Team 2 skill ratings.");
        for (int i = 0; i < GetArraySize(handleTeam2); i++)
        {
            decl String:auth[STEAMID_LEN];
            GetArrayString(handleTeam2, i, auth, STEAMID_LEN);
            new Float:playerStats[3];
            playerStats = GpSkill_FetchPlayerStats(auth);
            
            new Float:oppTeamStats[3];
            oppTeamStats[0] = team1RatingAvg;
            oppTeamStats[1] = team1DeviationAvg;
            
            GpSkill_CalculateGlicko(auth, playerStats, oppTeamStats, team2Outcome);
        }
        
        PrintToServer("[GP] Finished Glicko 2 skill updates.");
    }
}

GpSkill_CalculateGlicko(const String:auth[], Float:playerStats[3], Float:oppTeamStats[3], float teamOutcome)
{
    float glicko_two_constant = 173.7178;
    float tau = 0.5;
    float euler_number = 2.71828;
    float convergence_tolerance = 0.000001;

    // Step 1 and 2 of Glicko 2
    float playerMu = ((playerStats[0] - 1500) / glicko_two_constant); // Rating
    float playerPhi = (playerStats[1] / glicko_two_constant); // Deviation
    float playerSigma = playerStats[2]; // volatility
    
    float opponentMu = ((oppTeamStats[0] - 1500) / glicko_two_constant); // Rating
    float opponentPhi = (oppTeamStats[1] / glicko_two_constant); // Deviation
    
    // Step 3 of Glicko 2
    float opponentGValue = (1.0 / (Float:SquareRoot(1.0 + ((3.0 * (opponentPhi * opponentPhi)) / (3.14159 * 3.14159)))));
    float opponentEValue = (1.0 / (1.0 + (Float:Exponential((-1.0 * opponentGValue) * (playerMu - opponentMu)))));
    float v = Float:Pow(((opponentGValue * opponentGValue) * opponentEValue * (1.0 - opponentEValue)), -1.0);
    
    // Step 4 of Glicko 2
    float delta = v * (opponentGValue * (teamOutcome - opponentEValue));
    
    // Step 5 of Glicko 2
    float a = Float:Logarithm((playerSigma * playerSigma), euler_number);
    
    float A = a;
    float B = 0.0;
    if ((delta * delta) > ((playerPhi * playerPhi) + v))
    {
        B = Float:Logarithm((delta * delta) - (playerPhi * playerPhi) - v);
    }
    else
    {
        float k = 1.0;
        B = a - (k * tau);
        while (GpSkill_IllinoisAlgorithm(B, delta, playerPhi, v, a, tau) < 0.0)
        {
            k = k + 1.0;
            B = a - (k * tau);
        }
    }
    
    float fA = GpSkill_IllinoisAlgorithm(A, delta, playerPhi, v, a, tau);
    float fB = GpSkill_IllinoisAlgorithm(B, delta, playerPhi, v, a, tau);
    
    while (Float:FloatAbs(B - A) > convergence_tolerance)
    {
        float C = A + (((A - B) * fA) / (fB - fA));
        float fC = GpSkill_IllinoisAlgorithm(C, delta, playerPhi, v, a, tau);
        
        if (fC * fB < 0.0)
        {
            A = B;
            fA = fB;
        }
        else
        {
            fA = (fA / 2.0);
        }
        
        B = C;
        fB = fC;
    }
    
    float newPlayerSigma = Float:Exponential(A / 2.0);
    
    // Step 6 of Glicko 2
    float phiStar = Float:SquareRoot((playerPhi * playerPhi) + (newPlayerSigma * newPlayerSigma));
    
    // Step 7 of Glicko 2
    float newPlayerPhi = 1.0 / Float:SquareRoot((1.0 / (phiStar * phiStar)) + (1.0 / v));
    float newPlayerMu = playerMu + ((newPlayerPhi * newPlayerPhi) * (opponentGValue * (teamOutcome - opponentEValue)));
    
    // update DB values
    GpSkill_UpdatePlayerStats(auth, ((newPlayerMu * glicko_two_constant) + 1500), (newPlayerPhi * glicko_two_constant), newPlayerSigma);
    
    // print new rating and change to user
    if (GpSkill_Show_Rating())
    {
        new client = FindClientByAuthString(auth);
        if (client > 0 && IsClientConnected(client) && IsClientInGame(client) && !IsClientSourceTV(client) && !IsFakeClient(client))
        {
            decl String:msg[256];
            Format(msg, sizeof(msg), "New GPSkill Rating: %.02f (", ((newPlayerMu * glicko_two_constant) + 1500));
            
            float difference = (newPlayerMu - playerMu);
            if (difference > 0.0)
                StrCat(msg, sizeof(msg), "\x04+");
            else
                StrCat(msg, sizeof(msg), "\x02");
            
            StrCat(msg, sizeof(msg), "%.02f\x01)");
            Format(msg, sizeof(msg), msg, (difference * glicko_two_constant));
            PrintToChat(client, msg);
        }
    }
}

Float:GpSkill_IllinoisAlgorithm(float x, float delta, float phi, float v, float a, float tau)
{
    float result = Float:Exponential(x) * ((delta * delta) - (phi * phi) - v - Float:Exponential(x)) / (2.0 * Float:Pow((phi * phi) + v + Float:Exponential(x), 2.0)) - ((x - a) / (tau * tau));
    return result;
}
