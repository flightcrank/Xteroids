# Xteroids ☄️

A fast-paced, lightweight Asteroids clone written in pure C using **X11 (Xlib)**. No engines, no bloat—just math and pixels.


## Features
- **Pure C:** Zero external dependencies other than standard libraries and X11.
- **Software Rendered:** Custom pixel-pushing via `XImage`.
- **Lightweight:** The binary is tiny and optimized for performance.
- **Retro Physics:** Classic floaty space momentum.

## Prerequisites
To compile and run Xteroids, you need the X11 development libraries on Linux.

## To compile
```bash
gcc xteroids.c graphics.c -o xteroids -lX11 -lm
```

## Screenshots
<img src="https://i.imgur.com/yFlNzA6.png" width="800" alt="Xteroids Menu">
<img src="https://i.imgur.com/rHF5lEu.png" width="800" alt="Xteroids Gameplay">

