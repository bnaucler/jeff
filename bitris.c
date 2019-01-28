/*
 *
 * jeff - a minimalist tetris engine
 * B Nauclér (mail@bnaucler.se) 2018
 * MIT License (do whatever you want)
 *
 */

#include <stdio.h>
#include <ncurses.h>        // test impl
#include <stdlib.h>
#include <time.h>

#define PFH 20
#define PFW 10
#define NUMPCS 7
#define XSTART (PFW / 2) - 2
#define YSTART -2

#define DEBUG 1

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

const uint16_t smp[5] = {0, 40, 100, 300, 1200}; // score multipliers

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
    or a uint8_t * 10..?

    const uint16_t speed[] = 800, 720, 630, 550, 470, 380, 300, 220, 130, 100,
                             80, 80, 80, 70, 70, 70, 50, 50, 50, 30, 30, 30,
                             30, 30, 30, 30, 30, 30, 30, 20;
*/

static void newpiece(piece *p) {

    // NES style piece selection algo
    uint8_t np = rand() % NUMPCS + 1;
    if(np == NUMPCS || np == p->block) p->block = rand() % NUMPCS;
    else p->block = np;

    p->pos = 0;
    p->x = XSTART;
    p->y = YSTART;
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
    *lev = *lines / 10;

    return smp[newlines] * (*lev + 1);
}

// gets real x & y coordinates for block pieces
static int8_t setxy(const piece *p, const int8_t i, int8_t *x, int8_t *y) {

    if(!(blk[p->block][p->pos] & 1 << i)) return 1;
    *y = i / 4 + p->y;
    *x = i % 4 + p->x;
    
    return 0;
}

// checks for collisions
static uint8_t ccol(const uint16_t *pf, const piece *p) {

    int8_t x, y, i = 0;

    do {
        if(setxy(p, i, &x, &y)) continue;
        if(x < 0 || x > PFW - 1 || y > PFH - 1 || pf[y] & 1 << x) return 1;
    } while(++i < 16);

    return 0;
}

// updates pf by applying act to rows of p
static void plotpiece(uint16_t *pf, const piece *p, const uint8_t val) {

    int8_t x, y, i = 0;

    do {
        if(setxy(p, i, &x, &y)) continue;

        if(val) pf[y] |= 1 << x;
        else pf[y] &=  ~(1 << x);
    } while(++i < 16);
}

// draws the playing field - curses version
static void draw(const uint16_t *pf, piece *p, const uint8_t lev, const uint16_t lines, const uint16_t score) {

    erase();

    for (uint8_t y = 0; y < PFH; y++){
        for(uint8_t x = 0; x < PFW; x++)
            addch(pf[y] & 1 << x ? ACS_CKBOARD : '.');
        if(DEBUG) printw("   pf[%d] = %d\n", y, pf[y]);
        else addch('\n');
    } 

    printw("level: %d\tlines: %d\tscore: %d\n", lev, lines, score);
    if(DEBUG) printw("DEBUG: p->block = %d[%d], bval = %d, p->x = %d, p->y = %d\n",
            p->block, p->pos, blk[p->block][0], p->x, p->y);

    refresh();
}

// raw memory copy of piece s to d
static void cppiece(piece *d, const piece *s) {

    uint8_t sz = sizeof(d);
    char *pd = (char*) d;
    const char *ps = (char*) s;

    while(--sz) *pd++ = *ps++;
}


// Applies transformation to piece based on user input - curses impl
static uint8_t getinput(piece *p) {

    char c = getch();
    switch(c) {

        case 'q':
            return 1;
            break;

        case 'a':
            p->x--;
            break;

        case 's':
            p->x++;
            break;

        case 'k':
            p->pos++;
            if(p->pos > 3) p->pos = 0; 
            break;

        case 'j':
            p->pos--;
            if(p->pos < 0) p->pos = 3;
            break;
    }

    return 0;
}

int main(void) {

	srand(time(NULL));
    rand();

    uint16_t pf[PFH];

    uint16_t score = 0, lines = 0;
    uint8_t lev = 0;

    for(uint8_t i = 0; i < PFH; i++) pf[i] = 0;

    // curses impl
    initscr();
    raw();
    noecho();
    nodelay(stdscr, 1);
    timeout(200);

    piece p, n;
    newpiece(&p);

    for(;;) {
        plotpiece(pf, &p, 1);
        draw(pf, &p, lev, lines, score);
        plotpiece(pf, &p, 0);

        cppiece(&n, &p);
        n.y++; // drop

        if(ccol(pf, &n)) {
            plotpiece(pf, &p, 1);
            score += cline(pf, &lines, &lev, p.y);
            newpiece(&p);
            if(ccol(pf, &p)) break;

        } else {
            cppiece(&p, &n);
        }

        if(getinput(&n)) break;
        if(!ccol(pf, &n)) cppiece(&p, &n);
    }

    endwin(); // curses impl

    return 0;
}
