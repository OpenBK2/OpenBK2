# BK2FS python variant

Collection of scripts for making tools that manipulate Blitzkrieg 2 game files.

## One unit map per nation

Run `all_used_units.py` from this folder with three paths in order. For example,
replace the game and mod paths below with your own. The third path is the
folder inside your mod that contains the template `mapinfo.xdb`.

```sh
python all_used_units.py "/path/to/Data" "/path/to/Mods/MyMod" "Custom/Missions/AllUsedUnits_NATIONS"
```

Look in the template folder for `mapinfo1.xdb`, `mapinfo2.xdb`, and so on,
one per nation in multiplayer side order. Each row corresponds to the same
tech level on every map. For example, if a nation is disabled at level 2,
its second row stays empty and its level 3 units still appear in the third row.
