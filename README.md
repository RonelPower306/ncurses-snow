# ncurses-snow
A simple C Ncurses program that simulates a snowy environment.

![Snow with frame](images/snow_w_frame.png)

## What is this?
This project is a terminal-based program that simulates a **snowy enviroment** (including **stacking snow**) using *Ncurses* and *UNICODE characters*.

I was introduced to Ncurses by [Daniel Hirsch](https://www.youtube.com/@HirschDaniel)'s [Coding an ncurses Animation in C](https://youtu.be/QygMBlV_USk) video, which I recommend to those who have never worked with this library.

## How does it work?

The program generates **N flakes** with a random positions and vertical velocities. Then a loop updates each flake's position while simulating wind flow *(using a sin function)*. 

When the flakes hit the ground they leave a *1/8 UNICODE block*. 

For each flake that hits the same spot, the snow column gets higher and higher until it reaches a limit *(defined by the user)*. When this limit is reached, all columns are lowered to prevent overflow and avoid filling the entire screen with white blocks.

## Compiling

This projects requires the **Ncurses** library (more specifically **Ncursesw**) and **GCC** to compile.

- On **Debian/Ubuntu Linux**, the command should be `sudo apt-get install libncurses5-dev libncursesw5-dev`
- On **Arch Linux**, use `yay -S ncurses`

Then compile with: `gcc -Wall -o snow snow.c -DNCURSES_WIDECHAR=1 -lncursesw -lm`

**NOTE**: Compilation and execution of this program have only been tested on an **Arch Linux** system. The procedure may vary depending on your personal setup.

## Usage

The program can be closed at any time by pressing the **Q** (lowercase) key on your keyboard.

The animation will automatically resize based on the current terminal width and height.

The program can be run with the following arguments:

- **--help** (-h): displays all available arguments
- **--no-frame** (-f): disables the frame around the animation (the animation will take the full width and height of the terminal)
- **--flakes-count \<flakes count\>** (-c \<flakes count\>): runs the program with the specified flakes count.