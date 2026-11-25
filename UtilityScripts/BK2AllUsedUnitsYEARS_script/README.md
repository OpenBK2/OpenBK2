# Multiplayer unit maps by tech level

This script makes one map for each multiplayer tech level. Count the enabled
nations in the busiest tech level to get the number of rows on every map. If
the busiest level has five nations, all maps have five equally spaced rows.
By default, in a year with only two enabled nations, they take the first two rows in
multiplayer side order and the last three stay empty. Each nation's flagstaff
stands in the first column, with its units grouped by reinforcement type to
the right.

## Generate your first maps

1. Prepare a template map with at least one player and room for the unit table.
   For this example, put its `mapinfo.xdb` in your mod under
   `Custom/Missions/AllUsedUnits_YEARS/`. If you used an older template, remove
   its hand-placed row labels so they don't appear beside the generated flags.
2. Open a terminal in the folder containing `all_used_units_per_year.py`.
   You'll need Python 3.10 or newer. Install the XML reader once with
   `python -m pip install lxml`. On Linux, use `python3` if that's the name
   of your Python command.
3. On Windows, open `run.bat` in a text editor. Change `--datafolder` to your
   game's data folder and `--modfolder` to the folder for your mod. Keep
   `--mapfolder` as the path to the template inside the mod. Save the file and
   run `.\run.bat` in the terminal.

You can also run Python directly on Windows or Linux. Replace the two example
paths in quotes with your own. For the base game, omit `--modfolder` and put
the template under the data folder instead.

```sh
python all_used_units_per_year.py --datafolder "/path/to/Data" --modfolder "/path/to/Mods/MyMod" --mapfolder "Custom/Missions/AllUsedUnits_YEARS"
```

Look in the template folder for `mapinfo1.xdb` and `name1.txt` for the first
tech level, `mapinfo2.xdb` and `name2.txt` for the second, and so on.
Running the command again updates those numbered files. Each map starts from
the original `mapinfo.xdb`.

## Move a nation to a free row

Use `--row NATION_TAG FREE_MAP_ROW_INDEX`. Count rows from **0**. For example,
to put Australia in the **fifth row** whenever it is enabled, append
`--row Australia 4` to the Python command above. On Windows you can use the
batch file after setting its paths.

```powershell
.\run.bat --row Australia 4
```

When Australia is enabled, the generator puts it at index 4 and fills the
other rows from the top with the remaining enabled nations. When Australia
is disabled, another nation can use index 4. An enabled nation with an empty
reinforcement list still counts toward the grid size and gets its flagstaff.

Repeat `--row` for each nation you want to pin. For example,
`--row France 4 --row Italy 4` lets France and Italy use the same row in
different years. If both are enabled in any tech level, the generator reports
the row, nations and tech level before writing any maps. Choose a different
index for one of them. A five-row map accepts indices 0 through 4.

## Where do I find the nation tag?

1. Open `Consts/Test/Test_MultiplayerConsts.xdb` in the game or mod data
   you're using. Find `<Sides>`, then the `<Item>` for the nation you want.
   The base game's list contains Germany, USA and USSR. Names such as Australia,
   France or Italy work when your mod includes those sides.
2. Inside that item, find `<PartyInfo href="..."/>`. Open the XDB file named
   in the quotes. The file path ends at `.xdb`, before `#xpointer`.
3. Copy the text inside `<GeneralPartyName>`. For example,
   `<GeneralPartyName>France</GeneralPartyName>` gives you `France` for the
   command. Case is ignored. Put names containing spaces in double quotes.
   The `KeyBuildingFlag` field in the same file supplies the flagstaff.
   [French party record](../../Versions/Current/Data/Scenario/PartyDependentInfo/Game/France_PartyDependentInfo.xdb)
   · [Italian party record](../../Versions/Current/Data/Scenario/PartyDependentInfo/Game/Italy_PartyDependentInfo.xdb)

If two sides use the same `GeneralPartyName`, use their positions in the
multiplayer `<Sides>` list. Count the first `<Item>` as `0`, the second as
`1`, and so on. For example, `--row 1 4` moves the second side to the fifth
map row.
