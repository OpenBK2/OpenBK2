
# AGENT LEARNINGS

This file is meant for AI agents only, both for read and write purposes.

- UI text fitting: glyph advances and padded line cells are not visible letter bounds. PT Sans's initial `J` can extend left of its origin. Try the smallest translation that fits the visible letters before removing runtime font enlargement, and use that same corrected origin when drawing placed captions. Correct horizontal overhang per line before combining paragraph bounds; different lines can overhang opposite edges. Keep intentional scroll viewport clipping out of font fitting. Round wrapping constraints inward and autosized bounds outward when converting between screen pixels and virtual coordinates.
- Font regressions should use the actual control, style, parent clip, and shipped font. Encyclopedia names occur in both the body-font list and the centered heading; testing only the heading misses list failures. Include 800x600, the 1024x768 reference size, widescreen and ultrawide sizes, and 4K.
- Broader glyph tests can use the six runtime sizing code pages from `NCodePages`. Check `CFace::HasGlyph` so replacement glyphs do not masquerade as coverage of unsupported characters, and exclude raw markup delimiters from plain-text samples. UI bound checks and independent bitmap coverage tests complement each other.
- Windows builds here use Visual Studio's `Launch-VsDevShell.ps1` and bundled CMake. If sandboxed Ninja stalls before compilation, stop only the processes started for that build and retry through the approval tool. The bundled formatter is under `VC/Tools/Llvm/x64/bin`; pass it to the hygiene wrapper with `CLANG_FORMAT`.
- Fail fast when loading the Visual Studio environment: use `$ErrorActionPreference = 'Stop'` and, if scripts are disabled, a process-scoped execution-policy bypass before `Launch-VsDevShell.ps1`. Otherwise PowerShell can continue into CMake without SDK library paths and report unrelated missing libraries such as `ole32.lib`. In Windows PowerShell, restore `Continue` before redirecting native build output and check `$LASTEXITCODE`; otherwise a CMake warning on stderr can terminate an otherwise valid build.
- A Windows `LNK1140` can come from accumulated linker PDB state rather than current source size. In October 2026, `AILogic.pdb` had grown to 6.34 GB; regenerating it produced a 178 MB PDB. Treat that cleanup as recovery, not proof that the underlying cause is fixed. Repeated single-object links reproduced growth with `/INCREMENTAL`, even with `/PDBTMCACHE:NO`; `/INCREMENTAL:NO` stayed stable. The Release presets use RelWithDebInfo, which inherits incremental linking from CMake unless explicitly overridden. Switching the flag does not compact an existing PDB, so old oversized files need one regeneration. Keep `/debug` for crash symbols. VS Code's later `Failed to prepare executable target ... undefined` message was secondary to this link failure.

## Bulk sed over a name used by two layers

Mistake: replacing `theGroupLogic.GenerateGroupNumber()` with the simulation variant across
AILogic with one `sed` also rewrote `CAILogic::GenerateGroupNumber`, which is the client's
path to the same function, and a commented-out line. Caught only by reading the diff.

Avoid it: when a function is called from both the client and the simulation, list the call
sites with `grep -n` first and edit them by line, or anchor the pattern to the caller's
context. Read `git diff` of every touched file before building.

## Replays: which build and which player

- A replay records the build id that made it (`szImageId`). Find the branch that build came
  from before trusting a crash or a divergence: a command class missing from the current
  checkout loads as null.
- `--replay=` always plays as player 0. Another viewpoint needs the console command
  `replay "<name>" <player>` from the main menu.
- `--setvar` only works as one token, `--setvar=name=value`.
