# jeff v0.1A
A minimalist tetris engine

## Written by
Björn Westerberg Nauclér (mail@bnaucler.se) 2019

Compiled and tested on:
* MacOS High Sierra

However; the code is portable and should work pretty much anywhere. Please raise an issue if you encounter compilation problems.

## Why jeff?
jeff is designed to operate with as little memory and CPU usage as possible, and might be suitable for low-powered devices such as an attiny.
The version in the repository currently requires ncurses, which does in itself consume some systems resources, but the project goal is to create an efficient engine. Future commits will separate the game- and rendering engines to further accentuate this fact.

## Dependencies
* ncurses (for current test version)

## Installation
```
sudo make all install
```

Unless otherwise specified, the binary will be installed in `/usr/bin`.

## Key bindings
`a` - Moves piece to the left
`s` - Moves piece to the right
`j` - Rotates piece right
`k` - Rotates piece left
`q` - Quit

Set `DEBUG` to `0` in jeff.c to remove debug output.

## Contributing
Submit an issue or send a pull request if you feel inclined to get involved.

## Disclaimer
This project is in alpha version. Feel free to explore (and edit!) the code base, but expect bugs.

## License
MIT (do whatever you want)
