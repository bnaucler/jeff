/*
 *      jeff - a minimalist tetris engine
 *      B Nauclér (mail@bnaucler.se) 2018
 *      MIT License (do whatever you want)
 */

/* Reference material
 *
 * https://www.colinfahey.com/tetris/tetris.html
 * https://meatfighter.com/nintendotetrisai/
 *
 */

#ifndef JEFF_HEAD
#define JEFF_HEAD

#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>

#define DEBUG 1

#define PFY 20
#define PFX 10
#define NUMPCS 7
#define XSTART (PFX / 2) - 2
#define YSTART -2

#define BYTE 8
#define SHORT 16

#define INDEX bit / BYTE
#define OFFSET bit % BYTE

typedef struct {
    int8_t block, pos, x, y;
} piece;

const uint16_t blk[NUMPCS][4];

// forward declatations: jeff.c
extern uint16_t getspeed(uint8_t lev);
extern uint8_t getbit(const uint8_t *pf, const uint8_t x, const uint8_t y);

// forward declatations: jeff_nc.c
extern void ncinit(const uint8_t lev);
extern void draw(const uint8_t *pf, piece *p, const uint8_t lev, const uint16_t lines, const uint16_t score);
extern uint8_t getinput(piece *p);

#endif
