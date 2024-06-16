#include <ncurses.h>

#include "nmake.h"

void mconf_init(void) {
	initscr();
	printw("Hello World !!!");
	refresh();
	getch();
	endwin();
}
