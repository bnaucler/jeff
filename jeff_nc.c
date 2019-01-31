#include "jeff.h"
#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>

// Initializes ncurses
void ncinit(const uint8_t lev) {

    initscr();
    raw();
    noecho();
    nodelay(stdscr, 1);
    timeout(getspeed(lev));
}

// draws the playing field - curses version
void draw(const uint8_t *pf, piece *p, const uint8_t lev, const uint16_t lines, const uint16_t score) {

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

// Applies transformation to piece based on user input - curses impl
uint8_t getinput(piece *p) {

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
