#include "jeff.h"
// #include <stdio.h>
#include <stdlib.h>

// tetromino definitions
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
   if NES accuracy is prioritized over resource requirements
*/

// sets falling speed based on level
uint16_t getspeed(uint8_t lev) { return 50 - ((lev + 1) * 2) + (800/(lev + 1)); }

// sets individual bit to val (1 / 0)
static void setbit(uint8_t *pf, const uint8_t x, const uint8_t y, const uint8_t val) {

    uint16_t bit = (y * PFX) + x; // macro bound var name

    if(val) pf[INDEX] |= 1 << OFFSET;
    else pf[INDEX] &=  ~(1 << OFFSET);
}

// returns 1 if bit at x & y is set, else 0
uint8_t getbit(const uint8_t *pf, const uint8_t x, const uint8_t y) {

    uint16_t bit = (y * PFX) + x; // macro bound var name

    return pf[INDEX] & 1 << OFFSET ? 1 : 0;
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

    for(uint8_t x = 0; x < PFX; x++) {
        if(!getbit(pf, x, y)) return 0;
    }

    return 1;
}

// copies line ysrc to ydst
static uint8_t cpline(uint8_t *pf, const uint8_t ydst, const uint8_t ysrc) {

    uint8_t bit, x, ret = 0;

    for(x = 0; x < PFX; x++){
        ret += bit = getbit(pf, x, ysrc);
        setbit(pf, x, ydst, bit);
    }

    return ret;
}

// drops all lines above y - pads with 0 if necessary
static uint8_t droplines(uint8_t *pf, const uint8_t y) {

    for(uint8_t i = y; i > 0; i--) {
        if(!cpline(pf, i, i - 1)) break;
    }

    return 1;
}

// checks for full lines at collision - returns score
static uint16_t cline(uint8_t *pf, uint16_t *lines, uint8_t *lev, const uint8_t py) {

    uint8_t y, newlines = 0;

    for(y = py; y < py + 4 && py < PFY; y++) {
        if(fulline(pf, y)) newlines += droplines(pf, y);
    }

    if(!newlines) return 0;

    *lines += newlines;
    *lev = *lines / 10;

    return smp[newlines] * (*lev + 1);
}

// gets real x & y coordinates for block pieces
static int8_t setxy(const piece *p, const int8_t i, int8_t *x, int8_t *y) {

    if(!(blk[p->block][p->pos] & 1 << i)) return 0;
    *y = i / 4 + p->y;
    *x = i % 4 + p->x;

    return 1;
}

// checks for collisions
static uint8_t ccol(const uint8_t *pf, const piece *p) {

    int8_t x, y, i = 0;

    do {
        if(!setxy(p, i, &x, &y)) continue;
        if(x < 0 || x > PFX - 1 || y > PFY - 1 || getbit(pf, x, y)) return 1;
    } while(++i < SHORT);

    return 0;
}

// updates pf by setting or unsetting bits from p
static void plotpiece(uint8_t *pf, const piece *p, const uint8_t val) {

    int8_t x, y, i = 0;

    do { if(setxy(p, i, &x, &y)) setbit(pf, x, y, val);
    } while(++i < SHORT);
}

// raw memory copy of piece s to d
static void cppiece(piece *d, const piece *s) {

    uint8_t sz = sizeof(d);
    char *pd = (char*) d;
    const char *ps = (char*) s;

    while(--sz) *pd++ = *ps++;
}

// sets random seed without time.h
static void setsrand() {

    char buf[3]; // should be random enough for everyone

    FILE *f = fopen("/dev/urandom", "r");
    fgets(buf, 2, f);
    srand(buf[0] * buf[1]);

    fclose(f);
    rand();
}

// calculates size and allocates memory for playing field
static uint8_t *mkpf() {

    uint16_t bit = PFX * PFY; // macro bound var name
    uint8_t sz = INDEX + (OFFSET ? 1 : 0);

    return calloc(sz, sizeof(uint8_t));
}

int main(void) {

	setsrand();

    uint8_t lev = 0;
    uint16_t score = 0, lines = 0;
    uint8_t *pf = mkpf();

    ncinit(lev);

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
    free(pf);

    return 0;
}
