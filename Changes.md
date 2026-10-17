
# Changes to the game

- Fixed the crash when starting a map whose terrain file is missing. `MapInfo.xdb` and the `map.b2m` beside it are separate files, and a record without its map file used to load and start, leaving the terrain manager holding a terrain that was never loaded; the map's own crags and entrenchments then dug into it and the game died on the first tile. Starting a mission, from the menus, from the `map` and `mission` console commands or from a campaign, now checks for the file first and reports which one is missing. None of the fourteen missions shipped under `Data/Custom/Missions` has its `map.b2m` beside it, so without the install step below they are refused rather than fatal.

- A mission that cannot be started now returns to the main menu instead of crashing or quitting. The last step of starting one assumed both a scenario tracker and a loadable map, which is not true for a mission started from `autoexec.cfg`: `game.cfg` is read afterwards and its `main_menu` line takes the scenario tracker away again, leaving nothing for the queued mission to read. The console and the log say which of the two was missing.

- The custom mission screen lists only the missions it can actually start, and writes to the log and the console how many records it left out. A record with no `map.b2m` beside it has no map to play, so offering it only led to a refusal.

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
