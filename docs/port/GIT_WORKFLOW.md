# Git and shell workflow

Making git fast on this repo, the CRLF rules, rerere, and the quoting traps that have cost the most time.

Part of the Linux port notes; see [LINUX_PORT.md](../../LINUX_PORT.md) for the index.

---

## Gotchas found the hard way

- **Line endings are a mess, and that is fine - do NOT add a `.gitattributes`.**
  An earlier note here claimed the tree was uniformly CRLF. It is not, and believing
  that produced a false "MIXED endings!" alarm on a file that was pure LF. Measured
  with `git ls-files --eol` on 3312 engine `.cpp`/`.h` files:

  | | files |
  |---|---|
  | all LF | 2220 |
  | internally mixed | 1004 |
  | all CRLF | 88 |

  Project `CMakeLists.txt` are the opposite: 35 all-CRLF, 1 mixed. So the file you
  are about to edit could be anything; **detect, never assume**:

  ```bash
  git ls-files --eol -- path/to/file     # i/ is the index, w/ the working tree
  ```

  In a script, take the ending from the file rather than hardcoding it:

  ```python
  NL = "\r\n" if "\r\n" in text else "\n"
  ```

  `.gitattributes` is tracked but 0 bytes, and `core.autocrlf=false` on both sides, so
  git is byte-faithful in both directions and the index matches the working tree exactly
  (2220/1004/88 either way). gcc, ninja and cmake all read any of it without complaint.

  Adding the reflexive `* text=auto` would renormalize every source file to LF on the next
  commit - a diff touching thousands of files that would collide with **every** remaining
  cherry-pick, and shred `git blame` continuity. Leave it alone.

  The real hazard is not git, it is *tools that rewrite lines*: `sed`/`awk`/`python` on Linux
  write back LF and silently turn a file mixed. Two consequences:
  - a `$`-anchored pattern matches nothing (this already bit us once on `ED_B2/CMakeLists.txt`)
  - a rewritten line shows up as changed even when the text is identical

  Prefer `git cherry-pick`/`git apply`, which are byte-faithful. When `sed` is unavoidable, use
  an unanchored pattern on a unique token, then check that the ending did not change. Compare
  against the index rather than testing for CRLF, since most of this tree is LF and a
  CRLF-shaped test cries wolf on every one of those 2220 files:

  ```bash
  f=path/to/file
  before=$(git ls-files --eol -- "$f" | awk '{print $1}')   # i/lf, i/crlf, i/mixed
  after=$(git  ls-files --eol -- "$f" | awk '{print $2}')   # w/...
  [ "${before#i/}" = "${after#w/}" ] && echo OK || echo "ending changed: $before -> $after"
  ```

  If the `^M` clutter in diffs is annoying: `git config core.whitespace cr-at-eol` stops git
  flagging the CR as a whitespace error. That is display-only and never touches file content.
  `git diff --ignore-cr-at-eol` helps read a diff if something does go mixed.
- **Case collisions are invisible on Windows.** As of this branch, 8 `CMakeLists.txt` entries
  disagreed with on-disk case; commit #1 fixes all 8 (the original fixed 6 and missed the two
  editor ones, which only show up with `-DBUILD_EDITOR=ON`).
- **`git rerere` is enabled** with `autoupdate`. A conflict resolved once is replayed
  automatically if a step has to be redone.
- **Don't `git push origin` from the WSL clone.** There is no `origin` there any more - it was
  renamed to `windows` precisely because it pointed at the local Windows repo, not GitHub.
- For the four mega-sweeps (#6, #15, #25, #38 - 1966 of the ~2400 total file-touches), do not
  resolve conflicts by hand. They are regular substitutions; re-run the transformation on the
  current tree instead of reconciling year-old hunks.


## Shell / git ergonomics

### Make git fast first - everything else is downstream

This repo has ~237k tracked files, so any git-aware prompt runs an expensive `git status` on
every single prompt. Already applied to both repos:

```bash
git config feature.manyFiles true      # index v4 + untrackedCache + skipHash
git config core.untrackedCache true
git config core.fsmonitor true         # WINDOWS ONLY - see below
```

Measured `git status --porcelain`:

| | before | after |
|---|---|---|
| WSL / ext4 | 5.1s cold, 0.385s warm | **0.15s** |
| Windows / NTFS | 0.86s | 0.65s, then **0.16s** once `core.fsmonitor` is on |

`core.fsmonitor` is Windows-only in practice - on Linux git 2.53 answers
`fatal: fsmonitor--daemon not supported on this platform`. Don't chase it there.
To revert any of this: `git config --unset feature.manyFiles` (etc).

### Prompt

`starship` is in apt (1.22.1) and is the recommendation, because one binary and one
`~/.config/starship.toml` covers WSL bash, Windows PowerShell **and** Git Bash - the same prompt
everywhere instead of three configs. Add to `~/.bashrc`:

```bash
eval "$(starship init bash)"
```

If the prompt ever feels sticky in this repo, the `git_status` module is the reason (it is what
runs the expensive call). Either cap it or turn it off in `starship.toml`:

```toml
command_timeout = 1000
[git_status]
disabled = false      # set true if 0.15s per prompt still annoys you
```

Zero-install alternative: git already ships `git-prompt.sh` with `__git_ps1`. Lighter, uglier,
no extra dependency. Set `GIT_PS1_SHOWDIRTYSTATE=` (empty) to skip the costly part.

### zsh / oh-my-zsh - honest take

zsh itself is a real improvement (completion, globbing). **oh-my-zsh is not worth it here**: the
framework adds meaningful startup cost, its git plugin is ~150 aliases nobody memorises, and its
themes run `git status` on every prompt - precisely the expensive call in a 237k-file repo.

If you want zsh, prefer plain zsh + `zsh-autosuggestions` + `zsh-syntax-highlighting` + starship,
and skip the framework. But note that switching shells is orthogonal to "show me the branch" -
starship on bash gets you that today with no migration.

### Terminal

Windows Terminal is already installed (appx 1.24.x). **Skip terminator**: it is an X11 app, so
under WSLg it adds a compositor hop and loses GPU-accelerated text rendering. Windows Terminal
hosts both WSL and Windows shells and is the better choice for both.

### Worth more than prompt cosmetics, for *this* task

```bash
sudo apt install -y tig lazygit git-delta fzf starship bat ripgrep fd-find
```

- **`tig`** - commit browser. For walking the 44 commits one at a time this beats a fancy prompt
  by a wide margin: `tig log`, `tig show <sha>`, `tig blame`.
- **`lazygit`** - TUI; genuinely good at resolving cherry-pick conflicts hunk by hunk.
- **`git-delta`** - much better diff rendering. Wire it up:
  ```bash
  git config --global core.pager delta
  git config --global interactive.diffFilter 'delta --color-only'
  git config --global delta.navigate true
  git config --global delta.line-numbers true
  ```
- **`fzf`** - Ctrl-R history search.
- On Ubuntu the binaries are `batcat` and `fdfind`, not `bat`/`fd`.

**Do this one regardless of tooling**, it pays off across the remaining 43 cherry-picks:

```bash
git config --global merge.conflictstyle zdiff3
```

`zdiff3` shows the common ancestor in conflict markers, so you can see what the original code was
rather than just the two sides. On mechanical sweeps that is often the difference between
understanding a conflict and guessing at it.

### Windows side

Currently missing: `pwsh` (PowerShell 7), `winget`, `starship`, `oh-my-posh`. Only Windows
PowerShell 5.1 is present. (`winget` absent on Win11 is unusual - it may just be PATH in a
non-interactive context; check in a real session.)

Recommended: PowerShell 7 + starship (same `starship.toml` as WSL) + PSReadLine predictive
history. `posh-git` is the traditional answer but it runs `git status` per prompt and starship
does the job better. Git Bash stays useful for POSIX one-liners; PowerShell 7 is the better daily
driver for the cmake/ninja work.


## git rerere - "reuse recorded resolution"

**What it is.** A conflict memo pad. When you resolve a conflict by hand, git fingerprints the
conflict and stores your resolution in `.git/rr-cache/`. If it ever hits *the same conflict again*,
it replays your resolution automatically.

**Setting it up is two config lines** - already done in the WSL clone, nothing else to learn:

```bash
git config rerere.enabled true      # record resolutions, and replay them when seen again
git config rerere.autoupdate true   # also `git add` the replayed file, so you can just commit
```

**How you use it: you don't.** It is entirely passive. Resolve conflicts the way you always have;
it watches and remembers. The only visible sign is a line in git's output:

```
CONFLICT (content): Merge conflict in f.txt
Staged 'f.txt' using previous resolution.        <- rerere did the work
```

Without `autoupdate` that line reads `Resolved '...'` and the file is left unstaged (`UU`); with it
the file is staged (`M`) and you just commit.

**Why it matters for this port specifically.** Cherry-picks get thrown away and redone constantly:
you abort one, the tier-1 build shows the resolution was wrong, or `linux2` later gets rebased onto
a newer `netcode_bugfixes`. Every one of those replays the same conflicts. On the four mega-sweeps
(#6, #15, #25, #38) that is the difference between resolving hundreds of hunks once and resolving
them three times.

**Useful commands:**

| command | what it tells you |
|---|---|
| `git rerere status` | which files rerere is currently tracking a conflict for |
| `git rerere remaining` | what *you* still have to resolve by hand (empty = rerere got it all) |
| `git rerere diff` | how your in-progress resolution differs from the recorded conflict |
| `git rerere forget <path>` | **drop a recorded resolution** - see below |

**The one real hazard.** If you record a *wrong* resolution, rerere will keep silently reapplying
it, and it looks like git resolved things correctly. If a conflict keeps coming back wrong:

```bash
git rerere forget path/to/file      # during an active conflict
```

Nuclear option: `rm -rf .git/rr-cache` - you only lose memoised resolutions, never real commits.

**It is per-repository.** `.git/rr-cache` lives in the WSL clone and is not pushed. The Windows
repo has its own (empty) cache. If you ever want to carry resolutions across, copy the directory.


