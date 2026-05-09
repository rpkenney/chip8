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

Contains a single breakpoint at **`0x200`** (program entry). Run with `-b breakpoints/example.txt` and a ROM; stderr should print `BREAK PC=0x0200`. Execution is **paused** at that PC: **Space** steps one instruction; **N** step-over; **Enter** resumes timer-driven run.

## CLI

```text
./chip8 --breakpoints breakpoints/example.txt roms/game.ch8
```

Short flag: `-b`.

If the file is missing or a line is invalid, the program exits with an error on startup.

## Keys (GLFW shell)

- **Space** — one instruction while paused (after a breakpoint or panel **Pause**).
- **N** — step over (see main `README` for behavior).
- **Enter** — resume **running** (auto pacing).

A future UI can load the same file as the initial breakpoint set and edit the in-memory set at runtime.
