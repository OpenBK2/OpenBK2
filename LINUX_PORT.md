# Linux port - working notes

Replaying the portability commits from the old `linux` branch (last touched 2025-10-19)
onto current `netcode_bugfixes`, one commit at a time, verifying Windows stays healthy at
every step.

---

## Start here

**Read [docs/port/SESSION_STATE.md](docs/port/SESSION_STATE.md) first.** It is the resume
point: which commit each clone is on, what is green, what the working loop is, and which
error is next. It is short, and it is the only document that is allowed to go stale.

**These notes are committed as of 2026-08-26.** They were kept out of the branch by
`.git/info/exclude` while the work was on one machine. The renderer is the next thing to
look at and WSL cannot show it a real display, so they travel with the branch now.

**The replay is finished.** All 57 rows of the table in
[PORT_REPLAY.md](docs/port/PORT_REPLAY.md) are done: 55 applied, 2 absorbed into other
commits, none open.

Since then `linux2` has been rebased onto `netcode_bugfixes` and carries 230 commits past
it.

**The compile phase is over. Everything in the tree compiles on Linux**, and all 25
modules link, 3Dmotor among them, which the whole port was waiting on. D3D9 goes through
DXVK; the window, the cursor, the splash screen, the window icon and now the input devices
through SDL. **There are no platform blockers left**: `intrin.h`, D3D9, Win32 windowing and
DirectInput are all done.

**The game runs.** As of 2026-08-26 it starts, brings up DXVK on a real GPU, mounts the data
paks, loads the database, reads its profile and configs, builds its UI and steps frames in the
main loop. It then crashes releasing a screen, on the way back out of a menu; the resume point
has the AddressSanitizer report and what it names.

Two instruments exist for this and both are permanent: `OPENBK2_FILE_TRACE=1` and
`OPENBK2_CMD_TRACE=1` log what the game opens and which interface commands run, on both
platforms, and `scripts/port/diff-platform-trace.py` compares two such logs. That comparison is
what established the data path is finished: the two platforms load identical files in identical
order, so what is left is code.

**`Game` links.** A whole-tree `ninja -k 0` succeeds and produces an ELF 64-bit executable
with all 187 of its shared library dependencies resolved. That had never happened before
2026-08-26. The link went 107 undefined symbols, then 66, 63, 54, 0: the system FFmpeg
through pkg-config, `main` off Windows, three `CTerraGen` finders that were `inline` in a
`.cpp`, the D3DX shader effects stubbed because that path is unreachable on Windows too,
and finally Granny stubbed.

**It links against a Granny stub, not against Granny**, so this is a binary that builds
and starts rather than one that plays: all 54 entry points return null, zero or false and
log every call. **The next step is to run it** and read `granny_calls.log`, which says
which Granny functions were actually reached, in what order, and with what arguments.
That list, in that order, is the porting plan for Granny, which is its own project.

The classes the compile turned up, and the reasoning behind each link cluster, are written
into [PORT_ROADMAP.md](docs/port/PORT_ROADMAP.md) as they were found.

`linux2-verified` is stale - it points at the pre-rebase tip and is orphaned in both
clones. Nothing on the branch is unverified on Windows, which stays green at every commit,
but the marker no longer says so and has to be re-pointed by hand.

Two branches can be deleted once nothing is expected of them: `backup/pre-sleep-squash`
and `backup/pre-sleep-split`, both taken before rewriting history around the Sleep
commits on 2026-08-22.

## The documents

| document | what it is for | open it when |
|---|---|---|
| [docs/port/SESSION_STATE.md](docs/port/SESSION_STATE.md) | where work stopped: branch and commit state, the working loop, the standing instructions, the next error | picking the port back up after a break |
| [docs/port/LINUX_BUILD_TLDR.md](docs/port/LINUX_BUILD_TLDR.md) | the commands, in order, from a bare machine to a running `Game`: packages, DXVK, configure, data, launch | building on Linux for the first time, or on a new box |
| [docs/port/PORT_SETUP.md](docs/port/PORT_SETUP.md) | environment, prerequisites, building on Linux, the d3d9 stub, Windows/WSL sync, the verification budget | setting the machine up, or syncing the two clones |
| [docs/port/PORT_ROADMAP.md](docs/port/PORT_ROADMAP.md) | everything still to do, split into large items that need a decision and small items that are merely mechanical | looking for the next piece of work |
| [docs/port/DXVK.md](docs/port/DXVK.md) | building DXVK native, the packages it needs, wiring it in place of the DXSDK stub, and what it does not cover | starting on D3D9, windowing or input |
| [docs/port/PORT_REPLAY.md](docs/port/PORT_REPLAY.md) | the 57-commit table, all of it applied, and what each commit turned out to involve | asking what was already done, and why it was safe |
| [docs/port/PORT_FINDINGS.md](docs/port/PORT_FINDINGS.md) | why this codebase behaves as it does; reference, not tasks | something surprising happens and you want to know if it is known |
| [docs/port/GIT_WORKFLOW.md](docs/port/GIT_WORKFLOW.md) | making git fast here, the CRLF rules, rerere, the quoting traps | git or the shell is fighting you |
| [docs/port/BUILD_PERF.md](docs/port/BUILD_PERF.md) | compile and configure time on this tree | the build is slow enough to be worth fixing |
| [docs/port/CHEATSHEET.md](docs/port/CHEATSHEET.md) | the tooling installed for this port: tig, lazygit, delta, fzf, and the Windows terminal font fix | you want a faster way to do something you are doing by hand |

Two rules keep these useful. A finding lives in exactly one file, and the
roadmap links to it rather than restating it. And the roadmap is the only file
that holds unchecked boxes; once something is done it moves to the replay
record.

---

## Conventions

Set 2026-08-20. The code rules hold until clang-format and clang-tidy can enforce them
mechanically; that is deferred because reformatting now would produce a diff far too large
to review alongside the replay.

### No `Co-Authored-By` trailers

A `commit-msg` hook strips them. Installed in **both** clones:

```
C:\projects\OpenBK2\.git\hooks\commit-msg
~/src/OpenBK2/.git/hooks/commit-msg
```

Hooks live in `.git/hooks`, which is not cloned or pushed, so **reinstall after any fresh
clone**. If they ever need to be shared, `git config core.hooksPath <dir>` pointing at a
tracked directory would do it, but that is a repo-wide decision for later.

The hook deliberately does not fire on cherry-picks that reuse an existing message, so
replayed upstream commits keep their text verbatim.

### No em-dashes or en-dashes

Plain hyphens everywhere: commit messages, code comments, these notes. The rest of the
repo is plain ASCII outside the original Russian comments, and the long dashes are visual
noise. 96 of them were removed from `LINUX_PORT.md` and `CHEATSHEET.md` when this rule was
set.

### Comments must carry information

No ASCII-art divider comments:

```cpp
// no
// ---------------------------------------------------------------- exhaustive

// no
// ================================================================ helpers
```

A row of punctuation tells the reader nothing that a section name, a function name or a
test name does not already say, and it costs a line every time. If a block genuinely needs
introducing, write a sentence saying what it is for and why - like the one above
`TEST(BitOps, ZeroCases)`, which explains that those inputs are the ones that differed
before the helpers were made bit-exact.

New code only. The original `// ****` banners scattered through files like `Misc/Tools.h`
stay as they are; rewriting them would churn the diff for no benefit.

### History must be self-contained

Set 2026-08-22. A commit message must not depend on anything that will not
outlive it. No "the old branch's version missed one", no "re-run rather than
cherry-picked", no comparison against the `linux` branch. That branch is going
to be deleted, and every sentence pointing at it becomes noise the moment it
is.

Write for a reader who has this commit and this tree and nothing else. State
what the patch does and what invariant holds, in terms they can check
themselves.

Where the provenance goes instead: [docs/port/PORT_REPLAY.md](docs/port/PORT_REPLAY.md).
That is the right home for "this reproduces the old commit except for
whitespace", "the old version missed a site", and anything else whose value is
in the comparison rather than in the change.

This extends the rule above from "not how it was produced" to "not what it was
produced from".

### Always brace conditionals

```cpp
// yes
if ( x )
{
    foo();
}

// no
if ( x )
    foo();
```

This is a safety rule, not a style one - see Apple's `goto fail`, where a duplicated
unbraced statement silently disabled TLS certificate validation.

- Applies to **new** code only. Existing braceless code is left alone until a formatter
  pass; there is a lot of it, and touching it now would collide with the replay.
- Allman brace placement, matching the surrounding files.
- Verbatim reference copies of old implementations are exempt, since being byte-identical
  is their whole purpose - e.g. the `original::` namespace in `3Dmotor/test/BitOps.cpp`,
  which is annotated to say so.

Check new code before committing:

```bash
grep -nE "\b(if|for|while)\s*\([^)]*\)\s*[A-Za-z_]" <files>   # inline form
grep -nE "^\s*(if|for|while)\s*\(.*\)\s*$" <files>            # next-line form
```
