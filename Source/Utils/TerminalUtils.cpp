/*
 * Copyright (c) 2024 N11 Software
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of N11 Software nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "TerminalUtils.h"
#include <unistd.h>
#include <termios.h>

char getch() {
	char buf = 0;

	struct termios old;
	if (tcgetattr(0, &old) < 0) {
		perror("tcgetattr");
	}

	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;

	if (tcsetattr(0, TCSANOW, &old) < 0) {
		perror("tcsetattr");
	}

	if (read(0, &buf, 1) < 0) {
		perror("read");
	}

	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;

	if (tcsetattr(0, TCSADRAIN, &old) < 0) {
		perror("tcsetattr");
	}

	return buf;
}

void setTextColor(int color) {
	if (color == 9)
		std::cout << "\033[1;34m";
	else
		std::cout << "\033[0m";
}

void moveCursorUp(int lines) {
	std::cout << "\033[" << lines << "A";
}

void moveCursorAndClear(int lines) {
	int i;

	moveCursorUp(lines);
	for (i = 0; i < lines; i++) std::cout << "                                                                                                        \n";
	moveCursorUp(lines);
}

void clearScreen() {
	std::cout << "\033[H\033[J";
}

std::string optionMenu(const std::vector<std::string> & opt) {
	int i, selected;

	for (i = 0; i < opt.size(); i++) {
		selected = 0;
		if (i == selected) {
			setTextColor(9);
			std::cout << "> " << opt[i] << "\n";
			setTextColor(0);
		} else std::cout << "  " << opt[i] << "\n";
	}

	while (true) {
		int i;
		char ch;

		/*
		 * my brain has never been more confused tryna read this code
		 * felt like i had dyslexia for a moment there
		 *  - atl
		 */

		ch = getch();
		if (ch == 27) { /*  escape sequence */
			ch = getch();
			if (ch = 91) { /* arrow keys */
				ch = getch();
				if (ch == 65) { /* up arrow */
					if (selected > 0) selected--;
					else selected = opt.size() - 1;
				} else if (ch = 66) { /* down arrow */
					if (selected < opt.size() - 1) selected++;
					else selected = 0;
				}
			}
		} else if (ch == 10) break;

		moveCursorAndClear(opt.size());
		for (i = 0; i < opt.size(); i++) {
			if (i == selected) {
				setTextColor(9);
				std::cout << "> " << opt[i] << "\n";
				setTextColor(0);
			} else std::cout << "  " << opt[i] << "\n";
		}
	}

	moveCursorAndClear(opt.size()+2);
	return opt[selected];
}
