# Development Journal

## Session 1 — Environment setup through first working menu
Date: [fill in]

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