# Breakpoint lists (local)

Files in this directory are ignored by Git so you can keep machine-specific lists. This repo tracks `README.md` and `example.txt` (smoke test); add your own `.txt` files locally.

## Format (`.txt` or any text file)

- **One program counter per line** — where execution should **stop before** that instruction runs.
- **Hex** with `0x` or `0X` prefix (e.g. `0x200`, `0x202`).
- **Decimal** without prefix (e.g. `512` is the same as `0x200`).
- **Blank lines** are ignored.
- **Comments:** anything from `#` to end of line is ignored (the `#` may be preceded by spaces).

Values must fit in 16 bits (`0`–`65535`).

## Checked-in sample (`example.txt`)

Contains a single breakpoint at **`0x200`** (program entry). Run without `--step`, confirm stderr prints `BREAK PC=0x0200`. You are then in **manual** pacing: **Space** steps one instruction; **Enter** resumes timer-driven run.

## CLI

```text
./chip8 --breakpoints breakpoints/example.txt roms/game.ch8
```

Short flag: `-b`.

If the file is missing or a line is invalid, the program exits with an error on startup.

## Keys (current shell)

- **Space** — one instruction when pacing is manual (after a breakpoint, or with `--step`).
- **Enter** — resume **auto** pacing after a breakpoint. Ignored if you started with `--step` (stays manual-only).

A future UI can load the same file as the initial breakpoint set and edit the in-memory set at runtime.
