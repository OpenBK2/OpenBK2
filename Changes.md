
# Changes to the game

- The game can run in a window instead of over the whole screen, for running two copies side by side when testing multiplayer: `--window=left` and `--window=right` put it in the top quarter of the primary screen on that side, and `--window=WxH+X+Y` anywhere. The window is borderless, so two of them tile edge to edge, and the picture is scaled into it as it is into the full screen. `./launch.sh -w` passes it, choosing left for the first game and right when one is already running. Nothing about it is saved in the profile.

- LAN games are found across computers on Linux. The game worked out where to broadcast its search from the address its own host name resolves to, and Debian, among others, resolves that to 127.0.1.1, so the search never left the computer and the game list stayed empty however many games were hosted next door. It now broadcasts on every network the computer is connected to, reading them from the network interfaces on Linux and Windows alike, and that also mends the address a joining player gives the host. On Windows it used to guess each network's size from its address, so a home network inside 10.0.0.0/8 was broadcast to as a whole 10.x.x.x; it now uses each network's real mask.

- A second game on the same computer can now find and join a LAN game hosted by the first. Every instance asked for UDP port 4201, the second was refused it, and the LAN screen then sat on "loading game list" forever. An instance that finds the port taken now uses a free one instead: it can browse and join, and if it tries to host it says why it cannot, since other players look for games on 4201 only. When the network cannot be opened at all, the error now returns to the multiplayer menu and says which call failed, on which port, and the system's own error, for example `bind(UDP 0.0.0.0:4201) failed: error 98, Address already in use`, instead of only that a critical error happened.

- Universal MOD-18's European Axis side gets a sharp flag: Hungary's, Italy's and Romania's side by side, redrawn as the mod draws its icon.

- Total Conversion's China side gets a sharp flag: the Republic of China and the People's Republic side by side, each showing its canton, redrawn as the mod draws its icon. Sides whose picture combines several flags can now be redrawn the same way, from the drawings of their members.

- MPBMod6's USA and Japan sides get sharp flags with `obk2_flags.pak` as well, so all five of its sides now have them: the 48-star flag its own icon shows, and for Japan the Hinomaru of 1870, the design of the original game's Japan icon that it uses.

- Universal MOD-18's Japan, Great Britain, USA and USSR sides get sharp flags with `obk2_flags.pak`, each drawn as the mod draws it: Japan's Rising Sun with its disc in the middle, as the mod has it, rather than the Army flag's. Its German side, whose flag carries a swastika, and its two sides that combine several flags keep the mod's own pictures.

- The editor's toolbar buttons, object tree, property grid and movie editor buttons have new icons, from the Lucide icon set. The old ones were 16x16 pictures in 16 colours, black-outlined, which nearly disappeared on a dark theme. The new ones are drawn in the theme's text colour and stay sharp at any size and display scaling. Four buttons with no stock picture (the forced and no-references export variants, and Save Camera) have icons composed from Lucide parts, and four map overlays (shoot areas, AI map, passability, grid) wear stand-ins until icons are drawn for them. The terrain height palette, the cursors and the Run Game crest are unchanged. `MapEditorLib/res/icons/README.md` says how to change one.

- Total Conversion's multiplayer sides get sharp flags too, with `obk2_flags.pak` in its `Data`: fourteen of its nineteen sides, each drawn as the mod draws it, arms and all, in every tech level. Its Germany and Spanish Nationalists sides, whose flags carry a swastika and the Francoist eagle, keep the mod's own pictures, as do its three sides that combine several flags. MPBMod6's Great Britain gets the sharp Union Flag as well.

- The "random country" dice in the multiplayer game room are redrawn as a sharp vector picture, in the original's arrangement of two dice with soft shadows. Every installation and mod checked uses the same dice picture, so they all get the new one with `obk2_flags.pak`.

- The sharp multiplayer flags work over an installed copy of the original game, Fire of the Rising and Liberation as well: put `obk2_flags.pak`, which the build installs into `Data`, beside the installed game's own archives. Those games keep their multiplayer settings in their own archives, which this does not replace; their sides are recognised by their icons and their tech levels by name. Germany shows the stand-in, the USSR its 1936 flag, and the Allies side keeps the white star emblem of the original, redrawn sharp. Mods are not recognised yet and keep their own pictures.

- Records of a kind added after a game was released now load in that game's data. The game learned which kinds of record exist only from the `types.xml` in the data it plays, and an installed game's is as old as the game, so a new record there was found but never created. It now also knows every kind compiled into the program.

- The country flags in the multiplayer game room are sharp at any screen resolution, and keep their real proportions. They were 32x22 pictures stretched to fit the screen, about 80x46 pixels at 2560x1600, which blurred every edge and made them a fifth too wide. A flag can now be an SVG drawing, which the game renders at exactly the size it takes on screen, and which flag a side shows follows the years of the chosen tech level: the 48-star United States flag up to 1959, the Soviet flag of 1936 and its 1955 revision. Flags with a swastika are not shipped in any form, so the German side shows a stand-in after the one the original game used. Sides, tech levels and mods without the new records keep their old pictures.

- Battles with many tanks on screen run much faster: a replay with dozens of them went from about 46 to about 68 frames per second. Every animated model is written into one shared buffer each frame, and it was sized at 768K vertices for the scenes of 2005. When it filled, the game stopped and waited for the graphics card to finish everything it had queued, and with many units in view it did that every other frame, which was a third of the frame time. The buffer now holds 2178309 vertices, 70 MB of graphics memory instead of 27 MB, and the console variable `gfx_dynamic_geometry_vertices` sets it. It is read when the graphics device is created, so set it in the profile's `user.cfg` or in the console and restart; `gfx_stats` in the console shows how much of the buffer is in use. A device that cannot create a buffer of the size asked for is given a smaller one, down to the old size, and the console says so.

- `ExtendedStats.BlockLooseAirAttacks` prevents ground mechanical units from autonomously chasing aircraft, including vehicles with zero setup time and AA-capable secondary weapons. They still attack visible enemy aircraft they can shoot from their current position, and direct attack orders retain normal pursuit. The flag defaults to false; missing ExtendedStats preserve existing behavior. See `OBK2_Examples/ExtendedStats/BlockLooseAirAttacks.xdb`.

- Fixed `--mission`, `--campaign`, `--mod` and the other command line options being ignored on Windows when the whole argument is quoted, which is how PowerShell, `cmd` and most launchers quote anything with a space in it: `"--mission=Custom/Missions/Mission Impossible/mapinfo.xdb"` started the game at the main menu instead of the map. The command line was split by a parser that kept the quotes inside the argument, so the option was not recognised, and that also treated apostrophes and brackets as quotes, which split a path such as `Wittman's Korea` in two. It is now split by the rules Windows itself uses.

- Fixed a turret briefly turning back toward its default angle during Binoculars. Automatic return is suppressed for the selected platform while the ability is active and resumes when it ends; other turrets retain their normal behavior.

- Ground mechanical units can use the Binoculars ability when assigned in their unit data. `ExtendedStats.BinocularsPlatform` selects a platform by its `RotatePoint`; empty or invalid names use the hull. The selected platform aims at the clicked point, with hull assistance at turret limits and no translation. Its sight cone adds to normal circular sight and `SpecialSightsArray`, using infantry binocular settings and the fog range cap. See `OBK2_Examples/ExtendedStats/Binoculars.xdb`.

- `ExtendedStats.AggressiveAttacker` enables automatic attacks for idle or guarding tanks and non-AA SPGs. They use shared side visibility and full usable weapon range, respect Hold Fire and existing orders, and aim without chasing or repositioning. Hidden, friendly, dead or unreachable targets end the automatic attack. The flag defaults to false. See `OBK2_Examples/ExtendedStats/AggressiveAttacker.xdb`.
- Every mission start now records how long the map took to load, and in which phase, to the log and the in-game console: `map load: <record> 11x11 patches in 7.956 s (screens 0.537, ai_init 0.002, load_map 0.581, ai_after_map_load 1.718, world_update 0.471, load_graphics 4.646)`. Reports that a map loads slowly, which total conversions attract, could previously say only that the whole of it was slow; the line says whether the time went on the terrain, on creating the units, or on the models and textures. There is a `--exit-after-load` command line option that loads a map and leaves, printing the same line, for measuring one map or profiling the load; `docs/MapLoadProfiling.md` describes it.

- Mechanical units can add sight sectors through `ExtendedStats.SpecialSightsArray`. Each entry defines start/end angles in radians, a sight range, and an optional `PlatformName` matching a platform's `RotatePoint`; empty uses hull facing. Sectors follow hull/turret rotation and add to normal visibility, obeying terrain obstruction, normal sight modifiers, and `AIConsts.WarFog.MaxRadius`. The array defaults to empty. See `OBK2_Examples/ExtendedStats/SpecialSights.xdb`.

- Damaged data no longer corrupts memory while it loads. Four places took a length out of a file and used it without checking it, because the checks that were there are assertions, and assertions are compiled out of the builds anyone plays. A binary field in a record whose text had an odd number of characters made the hex decoder step over the string's terminator and then run on through memory until it happened to meet a zero byte, writing a byte of output for every pair it read; three characters were enough to write eight bytes into a one-byte buffer. A blob field could declare itself four gigabytes long and be believed. A chunk shorter than the field it is read into filled the rest of that field with whatever happened to follow it in the file, so the value loaded depended on its neighbours, and could read past the end of the file's buffer. Nothing the game's own tools write can trigger the first of these, since the encoder always emits exactly two characters per byte, so this is about damaged saves, hand-edited records and mods: those now load without the bad value, or are refused, instead of writing somewhere they should not. A field whose chunk is short now reads as zeroes, which is what a missing field has always done.

- Fixed a transport ordered to hook artillery on open ground never picking a gun. Given the hook order with a gun or its crew clicked, a truck tows it, which is why towing looked like it worked. Given the same order on the ground, the simulation looks for the nearest towable gun within resupply range, and it rejected every candidate: the test for "may this gun be taken" read the user-action mask with a command constant, and `ACTION_COMMAND_TAKE_ARTILLERY` is 31, which in that mask is the fifth squad formation order, a bit no gun ever carries. So the order was dropped and the truck stood still. The fifteen gun records that allow being towed are the ones whose behavior changes: heavy and light anti-aircraft, anti-tank, field artillery, Panzerwerfer, Karl and the projector. Replays recorded before this change that contain a hook order given without a gun named will no longer verify.

- Fixed squads offering the wrong formation order. The orders a squad may be given were worked out as "the first formation action plus the formation's type", and the two lists do not run in step: the five formation actions are declared in the order 0, 3, 2, 1, 4, so a squad that has a march formation was offered the offensive order instead, and a squad that has an offensive formation the march order. Pressing the offered order then named a formation the squad does not have, so the squad stopped and stayed as it was. Of the 285 squad records shipped, 21 are affected: the 104 that carry all four formations offer the same four orders either way. The multiplayer compatibility checksum of squad records changes with this, since the available-orders mask is part of it.

- Fixed the crash when starting a map whose terrain file is missing. `MapInfo.xdb` and the `map.b2m` beside it are separate files, and a record without its map file used to load and start, leaving the terrain manager holding a terrain that was never loaded; the map's own crags and entrenchments then dug into it and the game died on the first tile. Starting a mission, from the menus, from the `map` and `mission` console commands or from a campaign, now checks for the file first and reports which one is missing. None of the fourteen missions shipped under `Data/Custom/Missions` has its `map.b2m` beside it, so without the install step below they are refused rather than fatal.

- A mission that cannot be started now returns to the main menu instead of crashing or quitting. The last step of starting one assumed both a scenario tracker and a loadable map, which is not true for a mission started from `autoexec.cfg`: `game.cfg` is read afterwards and its `main_menu` line takes the scenario tracker away again, leaving nothing for the queued mission to read. The console and the log say which of the two was missing.

- The custom mission screen lists only the missions it can actually start, and writes to the log and the console how many records it left out. A record with no `map.b2m` beside it has no map to play, so offering it only led to a refusal.

- Installing the game now installs the map files its own records are missing. A map record and the `map.b2m` it is played on are separate files, and the released data tree holds only the records; the maps are under `Complete/`, in the layout each record names in its `MapFilesPath`. `cmake --install` copies 405 of them, about 130 MB, next to the records that need them, which is every custom mission the repository has a map for and most of the test maps under `Data/Test/TestMaps`. `-DINSTALL_TEST_MAPS=OFF` leaves them out.

- A terrain file for a different map than the record describes no longer crashes the game. A record's `MapFilesPath` survives being renamed or copied, so the data contains records pointing at somebody else's map, and a file even one patch larger than the record walked off the end of the per-patch arrays while building the terrain graphics. The size mismatch is now reported and flat terrain is generated instead, the same as for a record with no map file at all.

- A mission or a campaign can be started from the command line: `Game.exe --mission="Custom/Missions/Alternative/MapInfo.xdb"` starts one map as a single mission, and `Game.exe --campaign="Scenario/Campaigns/GER/Campaign.xdb;0;0;normal"` starts one mission of a campaign with that campaign's state behind it, where the fields after the record are the chapter, the mission and the difficulty and each may be left off. The console commands they are built from, `map` and `mission`, are unchanged; what is new is that reaching them no longer means editing a file in `Profiles`.

- Fixed multi-turret artillery installing while a turret was still aimed away. The check that waits for every turret to return to its travel angle advanced its index twice per pass, so it only ever looked at the even-numbered ones, and the odd ones were neither waited for nor ordered back. Of the shipped units only BM-8-48 has two turrets, so it is the only one whose behavior changes. Replays recorded before this change that contain a BM-8-48 install or uninstall will no longer verify.

- Fixed a building's ammunition reading dropping to a single weapon's worth. The total was summed correctly when the building appeared, but the update that arrives from the simulation stopped after the first weapon, so a building with several gun slots understated its ammunition for the rest of the match.

- Temporary resource storages can now be filled manually. The "fill resources" order and the infinite-supplies reading in the unit info panel were only offered on main storages, because the client tested the main-storage type twice instead of testing the temporary one. The simulation always counted both, registering temporary storages and routing transports to them, so only the player's ability to order a fill was missing.

- Fixed a desync (ASYNC) in multiplayer and replays when the AI launched free air reconnaissance: the scouting waypoints were shuffled with a per-process random generator, so each client sent its scouts in a different order. They are shuffled again with the shared seeded generator, as before the port, which restores the original AI behavior. Replays recorded before this change that contain AI air scouting will no longer verify.

- Fixed the editor crash when loading or assigning `ExtendedStats`, and failure to create new records: its resource type ID collided with the helicopter simulation type. The regenerated schema and updated binaries must be used together.
- Mechanical units can reference optional `ExtendedStats`: `ForwardTowed` lets artillery face forward while towed, using a front `LTowingPoint` authored in its model (positive exported `TowPoint.y`); `VerticalOffsetAngleRad` adds a signed offset to ballistic projectile launch elevation in radians and adjusts speed to preserve the intended impact point and weapon range. Defaults are `false` and `0.0`, and units without extended stats keep their original behavior.
- Fixed artillery hookup repeatedly turning or retrying after alignment: the precision approach now uses an initialized path, matches the final hookup tolerance, and rebuilds its path if the towing points are still apart.
- Multiplayer compatibility checksums are now cached between startup, mod load/unload (including returning to vanilla for campaigns/tutorials), and MP host/join attempts. Main-menu re-entry and repeated join checks reuse the snapshot; changing the MP map refreshes its checksum.
- Fixed multiplayer compatibility checksums ignoring nested tech-level, reinforcement, unit-stat and map data. Checksum calculations now refresh cached record values on each call, so changes to loaded data are detected too. String arrays and nested arrays now hash their contents instead of allocation storage.
- Added optional smooth mouse-wheel camera zoom: `setvar smooth_mouse_zoom = 1` enables it (integer, default `0`). `setvar smooth_zoom_speed = 1.0` controls how quickly zoom settles: higher values are faster, lower positive values are smoother, and non-positive or non-finite values use the default `1.0`. Zoom distance per wheel step is unchanged. Both settings are saved in the user profile.
- Game was ported to x64 architecture
- Fixed bugs from the given BK2 1.0 ~beta version:
	- Reinf time text not showing was fixed
 	- Fixed the bug where units would not engage with each other due to poorly written weapon range pick logic (it now picks the weapon with the largest range, not the first one from the weapons array)
- Ported the video player from old Bink to FFPMEG, allowing the use of modern video formats such as mp4
- Sound API was ported from FMOD to SDL mixer as the adapter
- Reinforcement slot inheritance was removed (in MP) - making the mechanic more flexible
- Base stealing bug (green team advantage) was fixed by making the cap bar yellow if both teams have units in the base circle, the base will get capped when filled and when only one team has units around the circle left
    - the yellow color is defined in `Consts/All/VisObjIconsSet_Texture.dds` after all player colors (16 of them) from MP
- Extended support for 16 player colors in MP
- Added the `Disabled` option to the MP tech levels, allowing the modders to disable some nations in some techlevels, the default value of this property is `false` - so all nations are enabled by default in a techlevel
- Max possible `CallForHelpRadius` constant was increased to 3840
- Mods can now have `mod_config.cfg` file in their folder that'll be execute for the update of scripted game constants
- Replay file names now include the date & time of the match in their name
- Fixed the rare - but troubling and random sound system division by 0 crash that was tormenting all previous BK2 versions
- Added crashpad as a crash reporter (for debugging)
- The game now calculates its data checksum (data folder + current mod) for MP and displays it in hexadecimal form next to the game version, that value is also used to check if game versions match during MP room joining
- Fixed the 2 know ASYNC bugs from AI Logic code - ones that are easy to reporoduce
- 16 bit color mode is now disabled by default as modern systems do not support it - it can still be "enabled" in the settings, but it's actually always 32 bit color now!
- MP chat from players in the game room is now outlined
- The player chat messages are now outlined and the player name now matches his units' color
- Game will now do a windows ping if someone sends a new chat message in MP (both in game and in the room)
- Added plenty of new RT_TYPES:
    - RT_EXTRA_GROUND_1 = 21
	- RT_EXTRA_GROUND_2 = 22
	- RT_EXTRA_GROUND_3 = 23
	- RT_EXTRA_GROUND_4 = 24
	- RT_EXTRA_GROUND_5 = 25
	- RT_EXTRA_GROUND_6 = 26
	- RT_EXTRA_GROUND_7 = 27
	- RT_EXTRA_GROUND_8 = 28
	- RT_EXTRA_GROUND_9 = 29
	- RT_EXTRA_GROUND_10 = 30
	- RT_EXTRA_AIR_1 = 31
	- RT_EXTRA_AIR_2 = 32
	- RT_EXTRA_AIR_3 = 33
	- RT_EXTRA_AIR_4 = 34
	- RT_EXTRA_AIR_5 = 35
	- RT_EXTRA_MIXED_1 = 36
	- RT_EXTRA_MIXED_2 = 37
	- RT_EXTRA_MIXED_3 = 38
	- RT_EXTRA_MIXED_4 = 39
	- RT_EXTRA_MIXED_5 = 40
	- RT_EXTRA_MAXLVL_GROUND_1 = 41
	- RT_EXTRA_MAXLVL_GROUND_2 = 42
	- RT_EXTRA_MAXLVL_GROUND_3 = 43
	- RT_EXTRA_MAXLVL_GROUND_4 = 44
	- RT_EXTRA_MAXLVL_GROUND_5 = 45
	- RT_EXTRA_MAXLVL_AIR_1 = 46
	- RT_EXTRA_MAXLVL_AIR_2 = 47
	- RT_EXTRA_MAXLVL_AIR_3 = 48
	- RT_EXTRA_MAXLVL_AIR_4 = 49
	- RT_EXTRA_MAXLVL_AIR_5 = 50
	- RT_EXTRA_MAXLVL_MIXED_1 = 51
	- RT_EXTRA_MAXLVL_MIXED_2 = 52
	- RT_EXTRA_MAXLVL_MIXED_3 = 53
	- RT_EXTRA_MAXLVL_MIXED_4 = 54
	- RT_EXTRA_MAXLVL_MIXED_5 = 55
	- _RT_NONE = 56 (it was moved)
- Restored the single-player cheats. The password check compared the entered password against a hash and a salt that were both scrubbed from the released source, so no input could ever match and every cheat silently did nothing. The expected checksum is now rebuilt from the known password. Open the console with the tilde key, type `password Barbarossa`, and the script commands become available, for example `@God(0,2)` for one-shot kills or `@Win(0)` to finish a mission. `@Password("Barbarossa")` is accepted as well; previously the `@` route refused to run anything until the cheats were already unlocked, which locked out the very call that unlocks them. Cheats remain disabled in network games, as before.
- Maximum map size in the editor was raised from 20x20 to 32x32 patches. The limit was never an engine one: nothing on the game side validates the patch count, and the two checks that existed both sat in the editor's New Map path, so they applied when a map was created and never when one was opened. Larger maps cost memory quadratically in the side, roughly 125 MB of AI and war fog state at 32 patches against 50 MB at 20. Two constants still cap this above 32 and have to be raised first if it goes higher: `LONG_PATH_LENGTH` in `CommonPathFinder.cpp`, which refuses paths longer than 2000 AI tiles and so runs out at about 44 patches, and `MAX_MAP_SIZE`, which seeds minimum searches in `Common_RTS_AI/Terrain.cpp` and stops covering the map at 64 patches.
- UI text no longer stretches horizontally on screens wider than 4:3. Glyphs are scaled by the same factor on both axes, taken from the screen height, so letters keep their proportions and wide windows get more room beside the text instead of wider letters. Text height is unchanged.
- Fixed the last letter of menu buttons and other UI labels being clipped on the right, and the bottom of the line being clipped, at any resolution other than 1024x768. Label sizes were rounded down to whole units of the 1024x768 layout, which at 2560 pixels wide threw away up to 2.5 pixels of the text.
- UI text is now sharp at every resolution. The game rasterises its fonts itself with FreeType at exactly the size each piece of text is drawn at, instead of stretching small pre-made font images, which is what made text blurry above 1024x768. The fonts are now open ones that can be shipped with the game: PT Sans for body text, Oswald Bold for headers and Liberation Sans for numbers. Characters none of them have, such as names or chat in other scripts, are drawn from fonts installed on the system where one has them. A font record opts in with the new FontFile field (and FallbackFontFiles); records without it, as in older mods, use their pre-made fonts as before.

- Restored the original visible UI font size and widescreen width after the runtime-font update. Replacement fonts are sized from the shipped atlas's capital height and character advances, then rasterised at the final pixel dimensions. Accents retain their full cell instead of shrinking every letter; GUI font sizes are unchanged.
