# nds-launcher

A homebrew chainloader with UI for the Nintendo DSi.
Loads .nds files from SD card and hands off execution.

## Status
Early development. Learning project — see JOURNAL.md for context.

## Building
Requires devkitPro with the nds-dev metapackage installed.

    make

Produces `launcher.nds`, runnable in melonDS or on hardware with unlaunch.

## Scope
Chainloader with file browser UI. Not firmware, not a ROM engine.
Chainloading of retail commercial ROMs would require nds-bootstrap
(separate project) — this launcher would invoke it, not replace it.
