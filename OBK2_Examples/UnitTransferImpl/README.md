# Multiplayer unit transfer

`unit_transfer.pak` adds a **Transfer** button above the selected-unit portrait in the bottom HUD. The button, popover, rows, and controls use the game's existing textured artwork and standard text appearance. The popover lists present teammates in their match colors. Click a teammate's checkmark to transfer your selected units; use the top down-arrow button or **Transfer** again to close it. The list scrolls when necessary.

The control appears only in a live multiplayer match with teammates. It hides with the minimized HUD, chat input, and mission movies. Checkmarks are disabled when no owned living units are selected. The synchronized command validates ownership and the recipient again when it executes. Transfers require another present player on the same playable side; enemies, spectators, and self-transfers are rejected. The recipient receives the game's standard "New Units Available" notification.

A transferred vehicle takes its passengers and towed gun. Donated units retain their level immediately. Subsequent recipient reinforcement level-ups raise a donated unit only when the new reinforcement level exceeds that unit's level. For example, donated level-3 heavies stay level 3 until the recipient reaches level 4; donated level-2 mediums received by a level-4 player stay level 2.

## Install and rebuild

The CMake game install target installs this archive into the game's `Data` directory. For manual installation, copy `unit_transfer.pak` into `Data` beside the existing archives. Every multiplayer client needs the updated executable and libraries as well as this package.

The archive contains only new resources under `UI/Game/Mission/UnitTransfer`. It reuses the stock menu panel, table rows, button states, HUD action frames, scrollbar, and checkmark artwork. Compact button descriptors resize native artwork while retaining its original state colors; they add no custom palette. It does not replace an existing mission screen or require a database schema change.

Run from the repository root with Python 3.9 or newer. The generator reads stock descriptors from `Versions/Current/Data`:

```sh
python OBK2_Examples/UnitTransferImpl/build_pak.py
python OBK2_Examples/UnitTransferImpl/build_pak.py --check
```

The generator uses fixed ZIP timestamps and UTF-16LE game text. Short text entries are stored uncompressed. `UnitTransferUI_test` loads every packaged descriptor through the game's database and checks native texture files, unchanged state colors, default text formats, action bindings, labels, and scroll geometry.

`UnitTransferCommand_test` checks both replay serialization and the separate multiplayer wire format, including command reconstruction by independent receivers. `UnitTransfer_test` checks side/slot restrictions, attached-unit traversal, retained levels, future level-ups, and save/load. Test targets must be built explicitly; the default game build does not rebuild them.

## Multiplayer checks

- Start a match with at least two teammates and an opponent. Check that only teammates appear and their name colors match the map.
- Select infantry, vehicles, and a transport carrying passengers or towing a gun. Donate from both host and joining clients and verify ownership, selection, and cargo on every client. Confirm that only the recipient receives "New Units Available".
- Verify the button and top close control close the panel; minimize the HUD and open chat to verify the control hides. Check several screen resolutions.
- Use enough teammates to require scrolling; check the last recipient is reachable by scrollbar and mouse wheel.
- Verify the button is absent in single player, replay playback, and a multiplayer match without teammates.
- Verify both donor-above-recipient and donor-below-recipient level cases, then replay the match to check the transfers reproduce.
