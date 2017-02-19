# GoonPUG Changelog

## GoonPUG 2.0
* Removed web integrations and other extensions to make plugin leaner. (Thanks to Talljoe)
* Updated to MetaMod 1.10.6 and SourceMod 1.8 (Thanks to Talljoe)
* GPSkill integrated into plugin.
* GPSkill now uses Glicko-2 instead of Trueskill. See [Glicko-2](http://www.glicko.net/glicko/glicko2.pdf) and [Abstracting Glicko-2 for Team Games](http://rhetoricstudios.com/downloads/AbstractingGlicko2ForTeamGames.pdf)
* Cvar 'gp\_skill\_enabled' is now '1' by default.
* New cvar 'gp\_skill\_display\_postmatch' to display your updated GPSkill rating after a match. Defaults to '1' which is enabled.
* Cvar 'gp\_restrict\_captain\_limit' is now '5' by default.
* New player command '.rating' which displays your current GPSkill Rating.
