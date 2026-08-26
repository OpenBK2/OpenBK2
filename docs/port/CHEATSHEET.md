# Cheat sheet - the tools installed for this port

Untracked. Part of the Linux port notes; see [../../LINUX_PORT.md](../../LINUX_PORT.md)
for the index. The tools themselves are installed in WSL.

**Already wired up for you.** `~/.bashrc` has a marked block (`BK2 dev ergonomics`) - delete it to
revert. Git global config now uses delta as pager and `zdiff3` conflict markers.
Open a **new** shell before anything below works.

---

## tig - the one that matters most for this port

Reviewing 44 commits one at a time is exactly what tig is for.

| key | does |
|---|---|
| `tig` | commit log, newest first |
| `tig <sha>` | open at one commit |
| `tig linux2-verified..linux2` | **your unverified surface** |
| `Enter` | open the diff for the highlighted commit |
| `j` / `k` | down / up |
| `Tab` | switch between log and diff pane |
| `/` | search, `n` next match |
| `q` | back one view, `Q` quit everything |
| `tig blame <file>` | who touched what |
| `tig status` | stage/unstage interactively |

Reviewing the next commit before applying it:

```bash
tig 177843659          # #6, the 903-file monster
```

## lazygit - conflict resolution

Aliased to `lg`. Best use here is cherry-pick conflicts.

| key | does |
|---|---|
| `lg` | launch |
| `1`..`5` | jump to panel (1 status, 2 files, 3 branches, 4 commits, 5 stash) |
| `space` | stage / unstage the selected file or hunk |
| `Enter` on a conflicted file | open the conflict resolver |
| `c` | commit |
| `P` / `p` | push / pull |
| `x` | context menu - shows every key valid right now |
| `?` | help |
| `q` | quit |

During a conflict, `Enter` on the file, then pick the side per hunk. Faster than editing markers by
hand - and rerere memorises whatever you choose.

## delta - better diffs

Now the pager for `git diff` / `git show` / `git log -p`. Nothing to run.

| key (inside the pager) | does |
|---|---|
| `n` / `N` | next / previous file in the diff |
| `space` / `b` | page down / up |
| `q` | quit |

Side-by-side for a wide screen:

```bash
git -c delta.side-by-side=true show <sha>
```

If CRLF `^M` clutter ever annoys you: `git diff --ignore-cr-at-eol`.

## fzf - fuzzy finder

| key | does |
|---|---|
| `Ctrl-R` | **fuzzy search shell history** - the one you will use constantly |
| `Ctrl-T` | insert a file path into the current command |
| `Alt-C` | cd into a subdirectory |

Combines well with git:

```bash
git switch $(git branch --format='%(refname:short)' | fzf)
git show   $(git log --oneline -200 | fzf | cut -d' ' -f1)
```

## ripgrep / fd / bat

`rg` is the fast grep, `fd` the fast find, `bat` a syntax-highlighting `cat`.
(Ubuntu ships them as `batcat`/`fdfind`; aliases are set.)

```bash
rg 'GetTickCount' Versions/Temporary/Engine/Sources   # ~10x faster than grep -r
rg -t cpp '_controlfp'                                # only C++ files
rg -l 'boost/predef'                                  # filenames only
fd -e cpp stdafx                                      # find files by name
bat Versions/.../CMakeLists.txt                       # cat with highlighting
```

`rg` respects `.gitignore` by default - use `-u` to include ignored files.

## starship - the prompt

Shows branch + dirty state automatically. Configured with the **no-nerd-font** preset because only
plain Cascadia Code is installed; if the prompt ever looks like boxes, that is a font problem, not
a starship problem.

Config: `~/.config/starship.toml`. If it feels sluggish in this repo, the `git_status` module is
why - see §7 of `LINUX_PORT.md`.

---

# Windows side: fixing the tiny/blurry font problem

## The cause

**Git Bash's blurriness is MinTTY, not Windows.** MinTTY is not per-monitor DPI aware, so on a
high-DPI screen Windows bitmap-scales it - hence blurred, tiny text. `git gui` and `gitk` are worse:
they are Tcl/Tk apps from the 90s, not DPI aware at all, and cannot really be fixed.

## The fix - run bash inside Windows Terminal

Windows Terminal is DPI-aware, GPU-accelerated, and has proper Unicode support. You already have it,
but it has **no Git Bash profile**. Add one: `Ctrl+,` -> *Add a new profile* -> *New empty profile*,
then set

- **Name**: `Git Bash`
- **Command line**: `"C:\Program Files\Git\bin\bash.exe" -li`
- **Starting directory**: `C:\projects\OpenBK2`
- **Icon**: `C:\Program Files\Git\mingw64\share\git\git-for-windows.ico`

Or paste into the `profiles.list` array in settings.json:

```json
{
    "name": "Git Bash",
    "commandline": "\"C:\\Program Files\\Git\\bin\\bash.exe\" -li",
    "startingDirectory": "C:\\projects\\OpenBK2",
    "icon": "C:\\Program Files\\Git\\mingw64\\share\\git\\git-for-windows.ico"
}
```

Then set a decent font for all profiles: `Ctrl+,` -> *Defaults* -> *Appearance* -> *Font face* ->
**Cascadia Mono** (installed) at 11-12pt. That alone fixes most of the "tiny and blurry" complaint.

PowerShell 7 should also appear as its own profile after a Terminal restart.

## Installing fonts (Cascadia / Nerd / JetBrains)

### Rule zero: install them on WINDOWS, not in WSL

Windows Terminal and VS Code are Windows apps and render with Windows-installed fonts. Fonts
installed inside WSL do nothing for them. WSL-side fonts only matter for GUI Linux apps under
WSLg (`~/.local/share/fonts` + `fc-cache -fv`), which is not your terminal.

### Never clone the nerd-fonts repo

It is ~10GB because it stores patched binaries for every family, and its build scripts exist for
people *creating* patched fonts. Releases publish per-font zips of a few MB. Use those.

### Route 1 - winget (easiest, but slim pickings)

Only two real programming fonts exist in winget's default source:

```powershell
winget install DEVCOM.JetBrainsMonoNerdFont     # JetBrainsMono Nerd Font 3.3.0
winget install SourceFoundry.HackFonts          # Hack (no Nerd glyphs)
```

There is **no Cascadia package** and no other Nerd Fonts in winget. Everything else is manual.

### Route 2 - manual download (everything else)

| font | where | resulting family name |
|---|---|---|
| Cascadia Code / Mono | github.com/microsoft/cascadia-code releases | `Cascadia Code`, `Cascadia Mono` |
| Cascadia **with** Nerd glyphs, official | same zip, the `*NF.ttf` files | `Cascadia Code NF`, `Cascadia Mono NF` |
| Cascadia patched by Nerd Fonts | github.com/ryanoasis/nerd-fonts releases -> `CascadiaCode.zip` | **`CaskaydiaCove Nerd Font`** |
| JetBrains Mono, Fira Code, Meslo, Hack, ... | ryanoasis/nerd-fonts releases, one zip per family | `<Name> Nerd Font` |

**Naming gotcha that wastes everyone's time:** the Nerd-Fonts-patched Cascadia is called
**CaskaydiaCove**, not Cascadia. If you search the font picker for "Cascadia Nerd" you will find
nothing. Microsoft's own NF build *is* called `Cascadia Code NF` - two different fonts, both real.

**Variant gotcha:** each Nerd Font ships three flavours.

- `... Nerd Font Mono` - all glyphs forced single-width. **Use this for terminals.**
- `... Nerd Font` - some glyphs double-width; can misalign box drawing in a terminal
- `... Nerd Font Propo` - proportional, for documents, not terminals

### Installing a downloaded font

Extract the zip, select the `.ttf` files, then either:

- **right-click -> Install** - per-user, no admin, lands in `%LOCALAPPDATA%\Microsoft\Windows\Fonts`
- **right-click -> Install for all users** - needs admin, lands in `C:\Windows\Fonts`
- or drag them onto *Settings -> Personalization -> Fonts*

Per-user is fine and avoids the UAC prompt.

Check what is installed:

```powershell
Get-ChildItem C:\Windows\Fonts, "$env:LOCALAPPDATA\Microsoft\Windows\Fonts" |
  Where-Object { $_.Name -match 'Cascadia|Caskaydia|JetBrains|Nerd|Hack|Fira' } |
  Select-Object -ExpandProperty Name -Unique
```

### Pointing things at the new font

- **Windows Terminal**: `Ctrl+,` -> *Defaults* -> *Appearance* -> *Font face*. Needs the exact
  family name from the table above. Restart Terminal if it does not appear.
- **VS Code** (`settings.json`):
  ```json
  "editor.fontFamily": "'JetBrainsMono Nerd Font Mono', Consolas, monospace",
  "terminal.integrated.fontFamily": "'JetBrainsMono Nerd Font Mono'"
  ```

### Do you need any of this?

**No.** `~/.config/starship.toml` is configured so the prompt contains zero Private Use Area
characters - it renders correctly on plain Cascadia Mono, which you already have. A Nerd Font
only buys decorative glyphs. If you install one, you can then switch to the richer preset:

```bash
starship preset nerd-font-symbols -o ~/.config/starship.toml
```

...which will overwrite the file, so re-add the `[cmake] disabled = true` and `[git_branch]`
tweaks afterwards.

## GUI clients, ranked for this workflow

| client | cost | verdict |
|---|---|---|
| **VS Code** (already installed) | free | **Start here.** With the *WSL* extension it opens `~/src/OpenBK2` natively - correct fonts, correct Unicode, and it edits the Linux side without going through `/mnt/c`. Add *GitLens* for blame/history. |
| **Sublime Merge** | paid | Fastest native client. Excellent 3-way merge, handles this repo's size without stalling. |
| **Fork** | paid | Best interactive-rebase and cherry-pick UI of the lot. |
| **Git Extensions** | free | Windows-native, powerful, dated UI. |
| **GitHub Desktop** | free | Too simplistic for cherry-pick/rebase work. |
| **Sourcetree** | free | Struggles on repos this size. |
| `git gui` / `gitk` | free | What you have now. Tcl/Tk, not DPI aware. Retire them. |

**lazygit also runs natively on Windows** (`winget install JesseDuffield.lazygit`) if you would
rather have one interface on both sides than learn a GUI.

## Recommendation if you only do one thing

Install the **WSL extension** in VS Code and open the Linux clone with it. One editor, correct
rendering, and it sidesteps the whole `/mnt/c` performance problem - you edit on ext4 directly.
