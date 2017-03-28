GoonPUG
=======

CS:GO competitive PUG plugin

A server running this plugin will alternate between an idle/warmup phase and an actual match.

While waiting for 10 players, the server should be left on some aim/dm type maps.
During the idle phase, all players will respawn automatically.
When enough players have readied up (via /ready), the game will progress to the match phase.

After all players have readied up, the following steps will occur:

1. Vote for the map to play.
2. Vote for team captains.
3. Pick teams.
4. Switch to the match map.
5. Wait for all players to load the map and ready up.
6. Play match.
7. Switch back to an aim/dm map and return to idle mode.


Requirements
------------

- [MetaMod: Source 1.10.6](http://www.metamodsource.net/)
- [SourceMod 1.8](http://www.sourcemod.net/downloads.php)


Plugin Installation
-------------------

1.  Install MetaMod and SourceMod.
    Download links are in the above requirements section.
2.  Extract the contents of the `goonpug_<version>.zip` file into your csgo server folder.
    Everything is properly organized underneath the standard `csgo/` directory.
3.  Install the plugin.
    A 'stable' pre-compiled .smx file is included with the plugin, but this version may not include the latest changes from the git source.
    If you wish to compile the most recent version of the plugin yourself, please note that it has only been tested using the compiler bundled with SM distrubtions, and the web compiler is not supported.
    ```
    cd csgo/addons/sourcemod/scripting
    ./compile.sh goonpug.sp
    cp compiled/goonpug.sp ../plugins/
    ```
4.  Configure your server to work properly with the Steam Workshop.
    Instructions can be found on the [Valve Developer Wiki](https://developer.valvesoftware.com/wiki/CSGO_Workshop_For_Server_Operators#How_to_host_workshop_maps_with_a_CS:GO_dedicated_server).
    Please note that you MUST put your API key in `webapi_authkey.txt` for the plugin to work, even if you run srcds with the `-authkey` command line option.


Map Configuration
-----------------

Map rotations must be specified using SourceMod map lists.
Example map lists are included with the plugin.
Maps specified in `goonpug_idle_maplist.txt` will be used for the idle/warmup rotation, and maps specified in `goonpug_match_maplist.txt` will be used for the match map vote selection.
If you don't need any special functionality, you can use the example maplists.cfg file as well.
```
cd csgo/addons/sourcemod/configs
cp maplists.cfg.example maplists.cfg
```
Please note that workshop maps must be entered as workshop/####/mapname in the maplist files.
The "mapname" does not have to match the current name of the map .bsp file for workshop maps since they tend to be updated regularly.
The proper .bsp filename will be fetched via the Steam Workshop API.


Plugin Cvars
------------

GoonPUG cvars can be set or overridden in your `csgo/cfg/sourcemod/sourcemod.cfg` file.

-   `gp_skill_display_postmatch` specifies whether or not to show players their new GPSkill ratting after each match. Defaults to `1`.
-   `gp_restrict_captains_limit` restricts the number of potential captains to the top N players. Defaults to `5`.
-   `gp_enforce_rates` enforces client rate cvars. Defaults to `0`.
-   `gp_ot_vote_unanimous` determines if everyone must agree to overtime. Defaults to `0` (51% of vote required).


Server configs
--------------

Two .cfg files are included with GoonPUG and can be found in `csgo/cfg`.
The default values should be sufficient for most users, but they can be customized as necessary.

-   `goonpug_pug.cfg` will be executed for any matches started by the plugin.
    For convenience, this should be the file executed by your gamemodes_server.txt for the Classic Competitive gametype.
-   `goonpug_warmup.cfg` will be executed for any warmup rounds.


User Commands
-------------

All user commands can be prefixed in chat with any one of the `.`, `!` or `/` characters.
They can also be issued in the console by prefixing the command with `sm_`.
For example, to ready up a player could say any one of `.ready`, `!ready` or `/ready` in chat, or the player could use `sm_ready` in his or her console.

-   `.ready` Set yourself as ready.
-   `.notready` Set yourself as not ready.
-   `.unready` Alias for `.notready`.

-   `.rating` or `.gpskill` Shows your current GPSkill Rating.


Admin Commands
--------------

All admin commands require the SM `ADMIN_CHANGEMAP` privilege.
Admin commands can be prefixed with `!` or `/` in chat, or with `sm_` in the console.
Currently, the plugin admin commands do not appear in the `sm_admin` menu.

-   `sm_lo3` Start a match with the current teams and on the current map.
-   `sm_endmatch` End the current match.
    Match results and stats will be saved to the GoonPUG server and the GO:TV demo will be saved.
-   `sm_abortmatch` Abort the current match.
    Match results and stats will not be saved and the GO:TV demo will be discarded.
-   `sm_restartmatch` Restart the current match.
    Note that if the match is in the second half, the plugin will swap the teams back to the sides they originally started on.
    
For easy access these commands can be added to the Sourcemod admin menu. 
Copy the GoonPUG section from 'csgo/addons/sourcemod/configs/adminmenu_custom.txt.example' into your adminmenu_custom.txt file and copy the GoonPUG section from 'csgo/addons/sourcemod/configs/adminmenu_sorting.txt.example' into your adminmenu_sorting.txt file.


Display GPSkill Online
----------------------

GPSkill stats are stored in an SQLite database located at /csgo/addons/sourcemod/data/sqlite/gpskill.sq3

The table name is trueskill and can be queried for steam id, mean, deviation, and conservativeRating. 
In order to fetch information about the player, such as current username or profile links use the [Steam API](https://developer.valvesoftware.com/wiki/Steam_Web_API).

Current DB Structure:

- steam_id            VARCHAR (32) NOT NULL PRIMARY KEY
- mean                REAL         NOT NULL DEFAULT (25.00)
- deviation           REAL         NOT NULL DEFAULT (8.333)
- conservativeRating  REAL         NOT NULL DEFAULT (0.00)


Notes
-----

GO:TV match demos will be automatically recorded and saved if GO:TV is enabled on the server.


License
-------
GoonPUG is copyright (c) 2014 Astroman Technologies LLC.
GoonPUG is distributed under the GNU General Public License version 3.
See [COPYING.md](https://github.com/goonpug/goonpug/blob/master/COPYING.md) for more information.
