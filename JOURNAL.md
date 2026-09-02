# Development Journal

## Session 1 — Environment setup through first working menu
Date: [07/05/2026]

### Goal
Install devkitPro on Windows 11, prove the toolchain builds a runnable .nds, 
and get the shape of a launcher's menu working with hardcoded data.

### What I did

**Toolchain install.** devkitPro via graphical installer on Windows 11, 
`nds-dev` metapackage. MSYS2 is the build shell. Installed melonDS separately 
as the emulator (DS mode, HLE BIOS — no BIOS dumps needed until I want DSi 
mode later).

**PATH resolution took several rounds.** devkitPro's modern design uses 
opt-in per-platform activation scripts (`devkitarm.sh`, `ndsvars.sh`) in 
`/opt/devkitpro/`, not automatic PATH population. The base `devkit-env.sh` 
in `/etc/profile.d/` only sets `$DEVKITPRO` and adds `tools/bin`, not the 
compiler directory. Sourcing `devkitarm.sh` + `ndsvars.sh` is required to 
get `arm-none-eabi-gcc` on PATH.

**MSYS2 startup files.** Fresh MSYS2 shell reads `.bash_profile` / 
`.bash_login` / `.profile` (first found) as a login shell, and does NOT 
automatically read `.bashrc`. Bare `.bashrc` config was silently ignored. 
Fix: created `.bash_profile` with `if [ -f ~/.bashrc ]; then source ~/.bashrc; fi`, 
put the devkit source lines in `.bashrc`. Debugged with an 
`echo "BASHRC LOADED"` probe at the top of `.bashrc` to prove it wasn't running.

**Heredoc gotcha.** Multi-line append via `cat >> file << 'EOF' ... EOF` 
can dump the closing `EOF` into the file as literal content if the terminal 
paste mangles line endings. Broke `.bashrc` for a round. Prefer `nano`/`vim` 
for multi-line edits. Always `tail` the file after append to verify.

**Template weirdness.** `templates/combined` (default arm7+arm9) is currently 
broken due to symbol conflicts between libnds and calico during devkitPro's 
ongoing migration to calico as the base layer. Every error was "X redefined" 
or "conflicting types for Y" — that pattern means library conflict, not user 
code. `templates/arm9` (arm9 only, uses prebuilt default_arm7) works. Also 
the correct choice going forward — I won't be writing custom arm7 anyway.

**Hello world build.** Copied `templates/arm9` to `~/dev/nds/hello`, ran 
`make`, got a `hello.nds`, ran in melonDS. Bottom screen: "Hello World!". 
Top screen: white (uninitialized). Confirms toolchain end to end.

**Read the hello-world source.** Key patterns:

- `consoleDemoInit()` bundles graphics + VRAM + font + stdout redirect.
- `iprintf` (integer printf, no float support) is the print habit to build. 
  Avoids soft-float bloat. DS ARM9 has no FPU.
- Frame loop shape:

while(pmMainLoop()) {
swiWaitForVBlank();
scanKeys();
// check keysDown() for input
// update state
// draw
}

- `swiWaitForVBlank` locks the loop to 60Hz. No scheduler, no threads by 
  default. Whole program is one loop on one CPU.
- `keysDown()` returns a bitmask of just-pressed buttons. Standard C 
  `pressed & KEY_UP` check. `keysHeld()` is for currently-held; 
  `keysDownRepeat()` is for auto-repeat when held.

**Dev environment.** VS Code with integrated MSYS2 terminal configured via 
`terminal.integrated.profiles.windows` in User Settings JSON, using 
`--login -i` flags on bash so `.bash_profile` chain fires and devkit env 
is loaded automatically in the terminal. C/C++ extension for syntax and 
completion. IntelliSense doesn't know where libnds headers live yet 
(cosmetic issue, deferred).

**Repo setup.** GitHub repo, SSH key auth, `.gitignore` excludes build/, 
*.nds, *.elf, *.o, *.d, editor cruft. README and JOURNAL committed alongside 
source. First commit was just the arm9 template with scaffolding — proves 
the "resume from any state" property from day one.

**First launcher-shaped code: hardcoded menu.**

State: a `const char* items[]` array and an `int selection` index. 
That's the entire program state.

Frame shape: input → conditionally update selection → conditionally redraw. 
Only redraw when state changes, not every frame. Avoids flicker without 
having to think about it.

Drawing: ANSI escape sequences (`\x1b[y;xH`) for cursor positioning in the 
console. Can format `%d` into the position code itself, so row numbers can 
be computed. `\x1b[2J` clears the screen.

Input: UP/DOWN wrap selection at boundaries, A shows a "selected" message, 
START exits the loop.

**Concept locked:** this program is the state-machine shape of the full 
launcher. One state variable, input transitions, drawn output. Real 
launcher will have more state and a real data source (SD filesystem 
instead of hardcoded strings), and the terminal transition on A will 
be chainload instead of a printed message. Same shape at scale.

**Refactor.** Original `drawMenu` was three copy-pasted blocks, one per 
item. Refactored to a `for` loop from 0 to `itemcount - 1`, choosing a 
prefix (`"--> "` or `"    "`) based on `i == selection`, computing the 
row as `5 + i`. Proved generality by adding a fourth item — no changes 
to `drawMenu` needed.

### Traps / lessons for future me

- Compiler errors that all say "X redefined" or "conflicting types" 
  → library version mismatch, not my code. Check what's being included 
  and which packages own those headers.
- Shell config debugging: add an `echo` probe to prove which files are 
  being read on shell start. Login vs interactive shell is a real 
  distinction that shows up in Docker, SSH, cron.
- `sizeof(arr)/sizeof(arr[0])` for array size — ONLY works in the array's 
  declaration scope. Array decays to pointer when passed to a function 
  and `sizeof` silently returns pointer size. Pass count as a separate 
  argument to functions.
- "Make it work, then make it right" — writing the dumb copy-pasted 
  version of drawMenu first, then generalizing, was faster than trying 
  to write the clever version cold.

### Where I am now
- Toolchain solid, VS Code + MSYS2 workflow smooth, repo pushed to GitHub.
- Menu of hardcoded items with cursor movement, selection display, exit.
- Refactored drawMenu is generic in item count.

### Next session
Real filesystem access. Replace the hardcoded array with actual .nds files 
scanned from a directory. Concepts to learn: libfat mount, DLDI (dynamic 
device driver for SD/flashcart access), directory iteration via POSIX-like 
API (`opendir`/`readdir`), string filtering for .nds extensions. This is 
the "cliff" from earlier — first real system-programming step. Expect 
friction. Journal the friction thoroughly.

## Session 2–3 — Filesystem: rung 1 (mount)

**Outcome:** `fatInitDefault()` returns true, `isDSiMode()` returns 1, menu
renders. Rung 1 cleared — under no$gba, not melonDS.

**Root cause of the multi-session block: melonDS's DSi SD emulation.**
Settled by differential test — identical .nds, identical BIOS/NAND dumps,
identical SD contents. Works in no$gba, fails in melonDS. Not my code, not
my toolchain. Consistent with libnds 2.0's DSi SD/eMMC driver being rewritten
from scratch around new threading/interrupt paths; melonDS self-labels DSi
mode experimental. unlaunch reads the same card fine because it ships its own
simpler driver — which is why "unlaunch can see the SD" was misleading
evidence for so long.

**Method that worked:** layered isolation plus controls.
- Bisected downward: dumps boot DSi menu ✓ → console sees SD via camera app ✓
  → image has valid FAT16 MBR (type 0x06, LBA 63, 55AA) ✓ → unlaunch reads
  launcher.nds off it ✓ → fatInitDefault fails ✗
- Control #1: built devkitPro's own filesystem/libfat example. Failed
  identically → eliminated all of my code in one step.
- Control #2: second emulator. Passed → eliminated the toolchain, localized
  to melonDS.

Having *no reference for success* was the thing that made the first several
hours unproductive. Get a control early.

**Dead hypotheses (5), for the record(claude's fault, i don't this toolcahin man good for notes tho):**
1. DSi unitcode header flag — devkitARM builds are hybrid; the entry point is
   the variable, not the build.
2. Malformed SD image — unlaunch had already read it.
3. Wrong filesystem library / missing libdvm — **libfat-nds 2.x IS the
   libdvm-era library** under the retained package name. `-lfat` is correct.
   There is no separate libdvm package.
4. "Not on the v2 stack" — read the absence of libdvm.a backwards.
5. Dirty install requiring reinstall — did it, unnecessary, tree was coherent.

**Environment facts worth keeping:**
- no$gba uses fixed filenames in the exe's directory and silently ignores
  anything else: `BIOSDSI9.ROM`, `BIOSDSI7.ROM`, `DSI-1.MMC`. Wrong names →
  no BIOS loaded → CPU executes garbage → "undefined opcode."
- SD image must be `DSi-1.sd`, shipped blank inside `DSI-SD.ZIP`. No folder
  sync — mount with OSFMount (or write into it with mtools) to add files.
- Stale `.d` files in build/ record absolute source paths and survive layout
  changes. Symptom: "No rule to make target <wrong path>". Fix: `make clean`.

**Design decision holding up:** no hardcoded drive prefixes. Lets me develop
against whatever mounts and deploy to `sd:/` on hardware unchanged.

**Cost:** ~2 sessions, zero launcher code. Claude put some bs here. I HATED this project for like 2 weeks because of ts. We're good now though.

**Next:** rung 2 — opendir/readdir/closedir, iprintf every entry unfiltered.
Then rung 3 (filter . / .. / non-.nds), rung 4 (fixed-size buffer, wire to
drawMenu). Scrolling and chainload deferred.

## Session 4 — Filesystem: rungs 2–4 (enumerate, filter, wire)

**Outcome:** menu now populates from the live SD image — real `.nds` files and
directories, cursor working. All four rungs of the enumeration ladder cleared
under no$gba. First fully data-driven build.

**Array groundwork first.** Switching the hardcoded `const char* items[]` to a
runtime data source broke immediately: declared `char* items[]` with an empty
initializer, compiler read it as `char*[0]`, every index tripped
`-Warray-bounds`. A C array's size is fixed at declaration — can't grow an empty
one at runtime. Fix: fixed capacity `char* items[MAX_ITEMS]` (32) plus
`int itemcount = 0` for how many slots are actually filled, loop bound
`i < itemcount`. This is the same fixed-buffer-over-malloc call I locked earlier
— bounded failure modes, no allocator on hardware with no memory protection.

### Rung 2 — raw enumeration

opendir/readdir/closedir, copy each entry into owned storage, print unfiltered.
Listing matched the test corpus exactly, count confirmed by hand.

**The core misconception, three bugs deep: `readdir` is a consuming call, not a
peek.** Each call advances the stream.

- Called `readdir` in both the `while` condition and the `if` body → consumed
  two entries per iteration, silently dropped every other file. Fixed with
  assign-in-condition: `while ((entry = readdir(dir)) != NULL && itemcount < MAX_ITEMS)`.
  Appears exactly once now.
- `items[i] = entry->d_name` stored a pointer into readdir's single internal
  buffer, overwritten every call → every slot aliased the same address, list
  showed the last name repeated. The `dirent*` is **borrowed, not owned.** Fix:
  `char item_storage[MAX_ITEMS][256]`, `strncpy` the characters in.
- `strncpy` doesn't null-terminate when the source fills the full width →
  explicit `item_storage[i][255] = '\0'` is load-bearing. Caps worst case at
  truncation, not over-read.

**Detour: getcwd.** Appeared unavailable, adding `unistd.h` threw an error that
vanished on recompile. `make clean && make` proved a clean from-scratch build →
stale build state, not a missing symbol (same `.d`-file class of ghost as
Session 2–3). `getcwd` returns `/` — no loader-supplied cwd under no$gba, `.` is
the default device root. Ultimately diagnostic sugar; didn't need it.

**Detour: fatInitDefault failing again — but NOT melonDS this time.** Distinct
fault, self-inflicted. Nothing added since rung 1 runs before the mount, so the
code wasn't the variable — the image was, after I'd written to it via OSFMount.
**Root cause: wrong filesystem on the image** (not FAT12/16/32). Reformatted,
mount succeeded. `opendir` returning `0x0` was a downstream symptom of the dead
mount, not a second bug — one root cause, two error messages. The `%p` print
pointed at init vs. logic correctly and did its job.

**Meta-lesson that cost time:** reasoned about a blank screen instead of reading
what was on it — a partial status line got misread as "nothing printed." Print
explicit state; never infer from absence.

### Rung 3 — filter to .nds + directory display

Filter runs inside the loop on `entry->d_name` directly, `continue` before any
copy. Only `.nds` listed, directories shown with a `/` marker.

- `entry->d_name[0] == "."` wouldn't compile — wasted a while on it. Root cause
  was quoting: `'.'` is a char constant (type `int`), `"."` is a string literal
  decaying to `char*`. Char-vs-pointer. Not an indexing problem.
- `strrchr` returns NULL on a dotless name; first version fed that straight to
  `strcasecmp`. `README` would've dereferenced NULL — no MMU, so no clean fault,
  just garbage or lockup. Guarded, reject on NULL.
- The `.`/`..` first-char skip looked redundant with the extension filter, but
  went load-bearing again once directories were accepted — `.` and `..` are
  directories and would pass the new `DT_DIR` branch otherwise.
- First directory version nested `DT_DIR` inside the "no dot" branch → only
  dotless folder names accepted, `backup.old` rejected. Restructured: test
  `DT_DIR` first, accept unconditionally, fall through to extension logic.
  Directory-ness is a separate classification, not a sub-case of the dotless path.
- Confirmed `d_type` in-toolchain by grepping `sys/dirent.h`
  (`unsigned char d_type;`, `#define DT_DIR 4`). Header presence proves
  declaration only; confirmed calico *populates* it empirically when the corpus
  subdir rendered with its marker. No `stat` fallback needed. `DT_DIR == 4` →
  BSD/Linux-conventional values; compare the named constant, never the literal.

**Also resolved:** the missing top rows from earlier were text wrapping +
console scrolling — long names wrap to a second row, pushing earlier output off
the top. Not cursor addressing.

### Rung 4 — wire to menu

Mostly landed back in rung 2 (the storage copy + `items[]` wiring). Remaining
work was initializing `item_type[itemcount] = entry->d_type` on **both** accept
paths, not just the directory one. The file path had been leaning on global
zero-init, which holds only until re-enumeration reuses a slot — a latent bug
primed to fire the moment directory navigation lands. Every claimed slot is now
fully initialized by the iteration that claims it.

### Traps / lessons for future me

- `readdir` advances the stream. Call it once per iteration. Its returned
  pointer is borrowed and reused — copy out, never store the pointer.
- `strncpy` doesn't terminate on a full-width copy. Terminate manually or it's a
  latent over-read.
- Char constant `'x'` vs string literal `"x"` — different types (`int` vs
  `char*`). Single vs double quotes is a type decision, not a style one.
- `strrchr`/`strchr` return NULL on no-match — guard before dereferencing. On a
  no-MMU target a NULL deref doesn't fault cleanly.
- A header defining a field proves declaration, not population. Verify the value
  is actually filled before depending on it.
- Bitwise `&` vs logical `&&` in conditions: `pressed & KEY_UP & itemcount > 0`
  parses as a bitwise AND against 1 (relational binds tighter). `KEY_UP` is
  `BIT(6)` so it silently never fired; `KEY_A` is `BIT(0)` so A worked by
  accident of bit position. Masking is `&`, logic is `&&`.
- Write-path bounds without matching read-path bounds is a recurring shape here
  — the empty-directory `items[-1]` reach and the uninitialized `item_type` slot
  were both this. Guard reads, not just writes.

### Where I am now

- Fully data-driven menu off the SD image: `.nds` files and directories, `/`
  marker on dirs, wraparound cursor, empty-directory and truncation messages.
- Deliberate test corpus on `DSi-1.sd` (mixed-case ext, no-dot, multi-dot,
  near-miss `.ndsx`, dotfile, subdir, overlong name) — reusable regression set.
- Known open: copy block duplicated across both accept branches. Prefix
  decoration eats 2 of 32 columns.(remember for scrolling session) `5 + i` row math runs off a 24-row screen
  ~i=18. Directories display but aren't enterable.

### Next session

Two candidates, and they interact — pick the order deliberately. **Scrolling**
(deferred since day one): a viewport over `items[]` so long lists and wrapped
names stop eating rows; width-truncation belongs in the same pass. **Directory
navigation:** current-path tracking, re-enumerate on select, reset `selection`
to 0, and a surgical dot filter (reject `.`, keep `..`) so `..` walks back up —
not a small addition. After both: **chainload** — the actual point of the launcher.