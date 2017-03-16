# GoonPUG Changelog

## GoonPUG 2.0.4
* Added '.gpskill' command as an alias for '.rating'.
* Added example files for how to add GoonPUG commands to Sourcemod Admin Menu.
* Remove de_fire from default match map list.
* Add de_season and de_canals to default match map list.

## GoonPUG 2.0.3
* If match is live, halftime, or overtime and an assigned player rejoins, force them into spectator on join to prevent suicide bug.
* Fixed OT bug and re-enabled the OT vote system.
* New cvar 'gp\_ot\_vote\_unanimous' to determine if OT must be agreed to by everyone. Defaults to '0' which is disabled (51% of vote required).

## GoonPUG 2.0.2
* Ratings are displayed as integers, but still tracked as floats.
* Post-match rating changes are now displayed in a dismissable menu.
* Add player disconnect logic to MS_MAP_VOTE state.
* Various bugfixes.

## GoonPUG 2.0.1
* Updated match settings config files.
* Added Deagle7k, Goonball, and Nuketown to idle maplist.
* Disabled OT as it can break the plugin.
* Various bugfixes.

## GoonPUG 2.0.0
* Removed web integrations and other extensions to make plugin leaner. (Thanks to Talljoe)
* Updated to MetaMod 1.10.6 and SourceMod 1.8 (Thanks to Talljoe)
* GPSkill integrated into plugin.
* GPSkill now uses Glicko-2 instead of Trueskill. See [Glicko-2](http://www.glicko.net/glicko/glicko2.pdf) and [Abstracting Glicko-2 for Team Games](http://rhetoricstudios.com/downloads/AbstractingGlicko2ForTeamGames.pdf)
* Cvar 'gp\_skill\_enabled' is now '1' by default.
* New cvar 'gp\_skill\_display\_postmatch' to display your updated GPSkill rating after a match. Defaults to '1' which is enabled.
* Cvar 'gp\_restrict\_captain\_limit' is now '5' by default.
* New player command '.rating' which displays your current GPSkill Rating.
