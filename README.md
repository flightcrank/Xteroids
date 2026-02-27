# Xteroids ☄️

A lightweight Asteroids clone written in pure C using **X11 (Xlib)** for Linux systems. 


## Features
- **Pure C:** Zero external dependencies other than standard libraries and X11 and single header libs for images and sound.
- **Software Rendered:** Xlib used for all gfx
- **Lightweight:** The binary is pretty small at just over 1mb with sound engine included
- **Retro Physics:** Classic floaty space momentum.

## Prerequisites
To compile and run Xteroids, you need the X11 development libraries on Linux.

## To compile

if you have make installed on your system, to compile just type

```bash
make
```

alternitivly just type the following command

```bash
gcc xteroids.c graphics.c -o xteroids -lX11 -lm -lpthread -ldl
```

then to run the program type

```bash
./xteroids
```

## Screenshots
<img src="https://i.imgur.com/KR9HZgz.png" width="800" alt="Xteroids Menu">
<img src="https://i.imgur.com/f9pmkJI.png" width="800" alt="Xteroids Gameplay">

