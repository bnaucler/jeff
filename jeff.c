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

/*
   rough approximation of falling speeds through hyperbolic regression
   can be replaced with:
   const uint16_t speed[] = {800, 720, 630, 550, 470, 380, 300, 220, 130, 100,
                             80, 80, 80, 70, 70, 70, 50, 50, 50, 30, 30, 30, 30,
                             30, 30, 30, 30, 30, 30, 20};
   if NES accuracy is prioritized over resource use
*/
static uint16_t getspeed(uint8_t lev) { return 50 - ((lev + 1) * 2) + (800/(lev + 1)); }

// calculates number of bytes needed to create grid
static uint8_t getpfsize() {

    uint16_t bit = PFX * PFY; // macro bound var name

    return INDEX + (OFFSET ? 1 : 0);
}

// sets individual bit to val (1 / 0)
static void setbit(uint8_t *grid, const uint8_t x, const uint8_t y, const uint8_t val) {

    uint16_t bit = (y * PFX) + x; // macro bound var name

    if(val) grid[INDEX] |= 1 << OFFSET;
    else grid[INDEX] &=  ~(1 << OFFSET);
}

// returns 1 if bit at x & y is set, else 0
static uint8_t getbit(const uint8_t *grid, const uint8_t x, const uint8_t y) {

    uint16_t bit = (y * PFX) + x; // macro bound var name

    return grid[INDEX] & 1 << OFFSET ? 1 : 0;
}

// resets piece structure
static void newpiece(piece *p) {

    // NES style piece selection algo
    uint8_t np = rand() % NUMPCS + 1;
    if(np == NUMPCS || np == p->block) p->block = rand() % NUMPCS;
    else p->block = np;

    p->pos = 0;
    p->x = XSTART;
    p->y = YSTART;
}

// returns 1 if line y is full
static uint8_t fulline(const uint8_t *pf, const uint16_t y) {

    // TODO: another (macro defined) ret val for totally empty? - use to stop cpline loop

    for(uint8_t x = 0; x < PFX; x++) {
        if(!getbit(pf, x, y)) return 0;
    }

    return 1;
}

// copies line ysrc to ydst
static void cpline(uint8_t *pf, const uint8_t ydst, const uint8_t ysrc) {

    // TODO: operate on full type when possible, else getbit() & setbit() which feels sloooow
    //       perhaps func for full line check first, to enable early stop?

    uint8_t bit, x;

    for(x = 0; x < PFX; x++){
        bit = getbit(pf, x, ysrc);
        setbit(pf, x, ydst, bit);
    }
}

// clears full lines - returns score
static uint16_t cline(uint8_t *pf, uint16_t *lines, uint8_t *lev, const uint8_t py) {

    uint8_t x, y, i, newlines = 0;

    for(y = py; y < py + 4 && py < PFY; y++) {
        if(fulline(pf, y)) {
            for(i = y; i > 0; i--) cpline(pf, i, i - 1);
            for(x = 0; x < PFX; x++) setbit(pf, x, 0, 0);
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
static uint8_t ccol(const uint8_t *pf, const piece *p) {

    int8_t x, y, i = 0;

    do {
        if(setxy(p, i, &x, &y)) continue;
        if(x < 0 || x > PFX - 1 || y > PFY - 1 || getbit(pf, x, y)) return 1;
    } while(++i < SHORT);

    return 0;
}

// updates pf by applying act to rows of p
static void plotpiece(uint8_t *pf, const piece *p, const uint8_t val) {

    int8_t x, y, i = 0;

    do { if(!setxy(p, i, &x, &y)) setbit(pf, x, y, val);
    } while(++i < SHORT);
}

// draws the playing field - curses version
static void draw(const uint8_t *pf, piece *p, const uint8_t lev, const uint16_t lines, const uint16_t score) {

    erase();

    uint8_t x, y;

    for(y = 0; y < PFY; y++){
        for(x = 0; x < PFX; x++) addch(getbit(pf, x, y) ? ACS_CKBOARD : '.');
        addch('\n');
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

// sets random seed without time.h
static void setsrand() {

    char buf[3]; // should be random enough for everyone

    FILE *f = fopen("/dev/urandom", "r");
    fgets(buf, 2, f);
    srand(buf[0] + buf[1]);

    fclose(f);
}

int main(void) {

	setsrand();

    // TODO: factory function? will remove need for sz
    uint8_t sz = getpfsize();
    uint8_t pf[sz];
    for(uint8_t i = 0; i < sz; i++) pf[i] = 0;

    uint8_t lev = 0;
    uint16_t score = 0, lines = 0;

    // curses impl
    initscr();
    raw();
    noecho();
    nodelay(stdscr, 1);
    timeout(getspeed(lev));

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
            timeout(getspeed(lev));
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
