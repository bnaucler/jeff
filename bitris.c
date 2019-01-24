/*
 *
 * bitris.c - a minimalist tetris implementation
 * B Nauclér (mail@bnaucler.se) 2018
 * MIT License (do whatever you want)
 *
 */

#include <stdio.h>
#include <ncurses.h>        // test impl
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define PFH 20
#define PFW 10
#define NUMPCS 7
#define XSTART (PFW / 2) - 2
#define YSTART -2

#define DEBUG 1

#define DEL 0
#define PLOT 1

#define ROWMX 1023

/* Reference material
 *
 * https://www.colinfahey.com/tetris/tetris.html
 * https://meatfighter.com/nintendotetrisai/
 *
 */


typedef struct {
    int8_t block, pos, x, y;
} piece;

const uint16_t blk[NUMPCS][4] = {

    {19968, 17984, 3648, 19520},  // T
    {36352, 25664, 3616, 17600},  // J
    {50688, 19584, 50688, 19584}, // Z
    {26112, 26112, 26112, 26112}, // O
    {27648, 35904, 27648, 35904}, // S
    {11776, 17504, 3712, 50240},  // L
    {3840, 17476, 3840, 17476},   // I
};

const uint16_t scoremp[5] = {0, 40, 100, 300, 1200}; // score multipliers

/* TODO: Implement algo to calculate the following drop speeds
   NES configuration:

  Level    Drop speed
         (frames/line)
     00            48 (0.8 s)
     01            43 (0.72s)
     02            38 (0.63s)
     03            33 (0.55s)
     04            28 (0.47s)
     05            23 (0.38s)
     06            18 (0.3 s)
     07            13 (0.22s)
     08             8 (0.13s)
     09             6 (0.1 s)
  10-12             5 (0.08s)
  13-15             4 (0.07s)
  16-18             3 (0.05s)
  19-28             2 (0.03s)
    29+             1 (0.02s)

    or this memory monster, in the worst case
    const uint16_t speed[] = 800, 720, 630, 550, 470, 380, 300, 220, 130, 100,
                             80, 80, 80, 70, 70, 70, 50, 50, 50, 30, 30, 30,
                             30, 30, 30, 30, 30, 30, 30, 20;
*/

static void newpiece(piece *p) {

    // NES style piece selection algo
    uint8_t npnum = rand() % NUMPCS + 1;
    if(npnum == NUMPCS || npnum == p->block) p->block = rand() % NUMPCS;
    else p->block = npnum;

    p->pos = 0;
    p->x = XSTART;
    p->y = YSTART;
}

// Applies left or right shift
static uint16_t mkshift(uint16_t val, int8_t shift) {

    return shift < 0 ? val << abs(shift) : val >> shift;
}

// clears full lines - returns score
static uint16_t cline(uint16_t *pf, uint16_t *lines, uint8_t *lev, uint8_t py) {

    uint8_t newlines = 0;

    for(int8_t y = py; y < PFH; y++) {
        if(pf[y] == ROWMX) {
            for(int8_t i = y; i > 0; i--) pf[i] = pf[i - 1];
            pf[0] = 0;
            newlines++;
        }
    }

    *lines += newlines;

    return scoremp[newlines] * (*lev + 1);
}

// set values to bitshift row -- TODO: can this be simplified? double shift
static void setrow(piece *p, uint8_t row, uint16_t *rval) {

    row -= p->y;
    *rval = blk[p->block][p->pos] & 15 << row * 4;
    *rval = mkshift(*rval, row * 4 - p->x);
}

// receives row value and shift - returns 1 for collision TODO: clean up
static uint8_t iscol(uint16_t *pf, uint16_t rval, uint16_t prval, int8_t y, int8_t rowx) {

    if(!rval || !pf[y + 1]) return 0;

    uint16_t pbx, pbrx;

    for(uint8_t x = rowx; x < rowx + 4; x++) {
        pbx = rval & 1 << x; // bit value for current line of piece
        pbrx = prval & 1 << x; // bit value for previous line of piece
        if(pbx != pbrx && pbx && pbx == (pf[y + 1] & 1 << x)) return 1;
    }

    return 0;
}

// checks for collisions -- TODO: seems glitchy
static uint8_t ccol(uint16_t *pf, piece *p) {

    // TODO: actual collision; piece should have 1 cycle to move
    //       modify to enable tuck / spin checks

    uint16_t rval, prval;

    for(int8_t y = p->y + 3; y >= p->y; y--) {
        prval = rval;
        setrow(p, y, &rval);

        if(y == PFH - 1 && rval) return 1; // screen bottom
        else if(iscol(pf, rval, prval, y, p->x)) return 1;
    }

    return 0;
}

// updates pf by applying act to rows of p
static void mvpiece(uint16_t *pf, piece *p, uint8_t act) {

    uint16_t rval;

    for(int8_t y = p->y; y < p->y + 4; y++) {
        setrow(p, y, &rval);

        if(act == DEL) pf[y] = pf[y] & ~rval;
        else pf[y] = pf[y] | rval; // assume PLOT
    }
}

// draws the playing field - curses version
static void draw(const uint16_t *pf, piece *p, uint16_t lines, uint16_t score) {

    erase();

    for(int y = 0; y < PFH; y++) {
        for(int x = 0; x < PFW; x++) {
            if(pf[y] & 1 << x) addch(ACS_CKBOARD);
            else addch('.');
        }

        if(DEBUG) printw("   pf[%d] = %d\n", y, pf[y]);
        else addch('\n');
    }

    printw("lines: %d\tscore: %d\n", lines, score);
    if(DEBUG) printw("DEBUG: p->block = %d[%d], bval = %d, p->x = %d, p->y = %d\n",
            p->block, p->pos, blk[p->block][0], p->x, p->y);

    refresh();
}

int main(void) {

	time_t t;
	srand((unsigned) time(&t));

    uint16_t pf[PFH];

    uint16_t score = 0, lines = 0;
    uint8_t lev = 0;

    for(int a = 0; a < PFH; a++) pf[a] = 0;

    // curses impl
    initscr();
    raw();
    noecho();
    timeout(200);

    piece p;
    newpiece(&p);

    // TODO: plot first in loop
    for(;;) {

        if(ccol(pf, &p)) {
            score += cline(pf, &lines, &lev, p.y);
            newpiece(&p);

        } else {
            mvpiece(pf, &p, DEL);
        }

        char c = getch();
        switch(c) {

            case 'q':
                return 0;

            case 'a':
                p.x--;
                if(p.x < -1) p.x = -1; // TODO: Check for piece boundry
                break;

            case 's':
                p.x++;
                if(p.x > PFW - 3) p.x = PFW - 3; // TODO: Check for piece boundry
                break;

            case 'j':
                p.pos++;
                if(p.pos > 3) p.pos = 0; // TODO: check for collision, enable tuck & spin
                break;

            case 'k':
                p.pos--;
                if(p.pos < 0) p.pos = 3; // TODO: check for collision, enable tuck & spin
                break;
        }

        p.y++; // drop
        mvpiece(pf, &p, PLOT);
        draw(pf, &p, lines, score); // p for debug
    }

    endwin(); // curses impl

    return 0;
}
