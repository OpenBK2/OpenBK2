# Multiplayer unit maps by tech level

This script makes one map for each multiplayer tech level. Each row shows one
enabled nation. Its flagstaff stands in the first column, and its units appear
to the right, grouped by reinforcement type. If you just want gaps from disabled
nations removed, run the script with its default layout.

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

## Let two nations use the same row

Suppose your mod enables France in 1940 and Italy in 1942, and they are never
enabled together. To give both the same place on every map, run this after
setting the paths in `run.bat`. With the Python command above, append the
same `--row France Italy` option.

```powershell
.\run.bat --row France Italy
```

| Nations enabled in that tech level | What appears in the first row |
| --- | --- |
| France only | French flag and French units |
| Italy only | Italian flag and Italian units |
| Neither | The reserved row stays empty |
| Both | The script reports both nations and the tech level before writing any maps |

Each `--row` reserves one row, in the order you give it. To keep France and
Italy on **separate** rows, use `--row France --row Italy`. Other enabled
nations fill the rows below in multiplayer side order. An enabled nation with
an empty reinforcement list still gets its flag.

## Where do “France” and “Italy” come from?

1. Open `Consts/Test/Test_MultiplayerConsts.xdb` in the game or mod data
   you're using. Find `<Sides>`, then the `<Item>` for the nation you want.
   The base game's list contains Germany, USA and USSR. The France/Italy
   example needs a mod that includes those sides.
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
`1`, and so on. For example, `--row 1 4` lets the second and fifth sides
share a row if they are enabled in different tech levels.
