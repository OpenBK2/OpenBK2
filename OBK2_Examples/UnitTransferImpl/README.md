# Multiplayer unit transfer

`unit_transfer.pak` adds a **Transfer** button above the selected-unit portrait in the bottom HUD. The button, popover, rows, and controls use the game's existing textured artwork and standard text appearance. The popover lists present teammates in their match colors. Click a teammate's checkmark to transfer your selected units; click **Transfer** again to close it. Its native arrow points right when closed and left when open. Captions and player names center vertically using their measured font height. The panel fits the teammate rows up to its original maximum size, keeping its lower border aligned with the button. The scrollbar and its side space appear only when the list needs scrolling.

The control appears only in a live multiplayer match with teammates. It hides with the minimized HUD, chat input, and mission movies. Checkmarks are disabled when no owned living units are selected. The synchronized command validates ownership and the recipient again when it executes. Transfers require another present player on the same playable side; enemies, spectators, and self-transfers are rejected. The recipient receives the game's standard "New Units Available" notification. Clicking it focuses a surviving donated unit, or the saved donation position if none remains visible.

A transferred vehicle takes its passengers and towed gun. Donated units retain their level immediately. Subsequent recipient reinforcement level-ups raise a donated unit only when the new reinforcement level exceeds that unit's level. For example, donated level-3 heavies stay level 3 until the recipient reaches level 4; donated level-2 mediums received by a level-4 player stay level 2.

## Install and rebuild

This is an optional example GUI package, installed manually. Copy `unit_transfer.pak` into the game's `Data` directory beside the existing archives to enable these controls. CMake does not install it. Every multiplayer client needs the updated executable and libraries for the synchronized command.

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
- Transfer ground attack aircraft, fighters, and recon planes. Check the shortcut moves to the recipient, orders work through both direct selection and the shortcut, and the donor cannot issue further orders. Transfer them back and repeat without selecting another group first. Click the notification after units have moved and verify the camera follows a donated unit.
- Verify that **Transfer** opens and closes the panel and its direction arrow updates; minimize the HUD and open chat to verify the control hides. Check caption centering at several resolutions and HUD font scales.
- Use enough teammates to require scrolling; check the last recipient is reachable by scrollbar and mouse wheel.
- Verify the button is absent in single player, replay playback, and a multiplayer match without teammates.
- Verify both donor-above-recipient and donor-below-recipient level cases, then replay the match to check the transfers reproduce.
