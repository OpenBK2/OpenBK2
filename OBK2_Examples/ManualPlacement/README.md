# Manual starting positions

Copy `manual_placement.pak` into the game's `Data` directory on each player's computer. Use the updated game on each computer.

In a custom LAN or online game room, the host can click the dice in the lower-left corner of the minimap to roll starting positions. Everyone sees the assignments. The host can drag a player flag onto another flag to swap their starts, or onto a grey flag to move the player to an empty start. Clicking the dice again restores the room's original placement setting. Each new roll generates a fresh assignment.

Co-op swaps are limited to human starts. Scripted AI keeps its assigned positions. On maps with fixed nations, players can move between starts assigned to their nation.

The archive uses the existing nation-selection dice, loading-screen team artwork and names, and neutral minimap flag. To rebuild it, run `python OBK2_Examples/ManualPlacement/build_pak.py` from the repository root with Python 3.9 or later.
