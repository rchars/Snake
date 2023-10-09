#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#define GAME_H 13
#define GAME_W 23


#ifdef _WIN32
	#include <conio.h>
	#include <windows.h>
	#define print_chars printf

	typedef enum {
		KEY_UP = 72,
		KEY_DOWN = 80,
		KEY_LEFT = 75,
		KEY_RIGHT = 77
	} Directions;
	

	HANDLE h_console;


	void prepare() {
		h_console = GetStdHandle(STD_OUTPUT_HANDLE);
	}


	void clear() {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		SMALL_RECT scroll_rect;
		COORD scroll_target;
		CHAR_INFO fill;
		if(!GetConsoleScreenBufferInfo(h_console, &csbi)) {
			return;
		}
		scroll_rect.Left = 0;
		scroll_rect.Top = 0;
		scroll_rect.Right = csbi.dwSize.X;
		scroll_rect.Bottom = csbi.dwSize.Y;
		scroll_target.X = 0;
		scroll_target.Y = (SHORT)(0 - csbi.dwSize.Y);
		fill.Char.UnicodeChar = TEXT(' ');
		fill.Attributes = csbi.wAttributes;
		ScrollConsoleScreenBuffer(h_console, &scroll_rect, NULL, scroll_target, &fill);
		csbi.dwCursorPosition.X = 0;
		csbi.dwCursorPosition.Y = 0;
		SetConsoleCursorPosition(h_console, csbi.dwCursorPosition);
	}


	int get_signal() {
		DWORD start = GetTickCount();
		int signal;
		while (1) {
			if(_kbhit()) {
				signal = _getch();
				if(
					signal == KEY_UP ||
					signal == KEY_DOWN ||
					signal == KEY_LEFT ||
					signal == KEY_RIGHT ||
					signal == 'q'
				) {
					return signal;
				}
				return -1;
			}
			if(GetTickCount() - start >= 525) {
				return -1;
			}
		}
	}
#else
	#include <ncurses.h>
	#define print_chars printw


	int get_signal() {
		int signal = getch();
		if(
			signal == KEY_UP ||
			signal == KEY_DOWN ||
			signal == KEY_LEFT ||
			signal == KEY_RIGHT ||
			signal == 'q'
		) {
			return signal;
		}
		return -1;	
	}


	void prepare() {
		initscr();
		raw();
		noecho();
		keypad(stdscr, TRUE);
		clear();
	}
#endif


struct ThaCoord {
	size_t x;
	size_t y;
};


struct ThaCoord SEGMENTS[GAME_H * GAME_W + 1];
struct ThaCoord APPLE_COORDS;
size_t LAST_SEGMENT_INDEX = 0;
size_t SCORE_COUNTER = 0;
int CURRENT_DIRECTION = 0;


void print_data() {
	for(size_t n = 0; n < GAME_H + 2; n++) {
		for(size_t m = 0; m < GAME_W + 2; m++) {
			if(n == 0 || n == GAME_H + 1 || m == 0 || m == GAME_W + 1) {
				print_chars("#");
				goto no_space;
			}
			if(SEGMENTS[0].x == m && SEGMENTS[0].y == n) {
				print_chars("M");
				goto no_space;
			}
			for(size_t k = 1; k <= LAST_SEGMENT_INDEX; k++) {
				if(SEGMENTS[k].x == m && SEGMENTS[k].y == n) {
					print_chars("0");
					goto no_space;
				}
			}
			if(
				n == APPLE_COORDS.y &&
				m == APPLE_COORDS.x
			) {
				print_chars("O");
				goto no_space;
			}
			print_chars(" ");
			no_space:;
		}
		print_chars("\n");
	}
	print_chars("Score => %zu, q => Quit", SCORE_COUNTER);
}


void update_apple_coords() {
	size_t counter = 0;
	size_t free_coords_count = GAME_H * GAME_W - (LAST_SEGMENT_INDEX + 1);
	size_t random_num = rand() % free_coords_count;
	for(size_t n = 1; n <= GAME_H; n++) {
		for(size_t m = 1; m <= GAME_W; m++) {
			for(size_t k = 0; k <= LAST_SEGMENT_INDEX; k++) {
				if(SEGMENTS[k].x == m && SEGMENTS[k].y == n) {
					goto coords_not_free;
				}
			}
			if(counter == random_num) {
				APPLE_COORDS.x = m;
				APPLE_COORDS.y = n;
				goto updated;
			}
			counter++;
			coords_not_free:;
		}
	}
	updated:;
}


size_t is_apple_eaten() {
	if(
		SEGMENTS[0].x == APPLE_COORDS.x &&
		SEGMENTS[0].y == APPLE_COORDS.y
	) {
		struct ThaCoord new_segment;
		if(LAST_SEGMENT_INDEX == 0) {
			switch(CURRENT_DIRECTION) {
				case KEY_UP:
					new_segment.x = SEGMENTS[0].x;
					new_segment.y = SEGMENTS[0].y + 1;
					goto insert_new_segment;
				case KEY_DOWN:
					new_segment.x = SEGMENTS[0].x;
					new_segment.y = SEGMENTS[0].y - 1;
					goto insert_new_segment;
				case KEY_LEFT:
					new_segment.x = SEGMENTS[0].x + 1;
					new_segment.y = SEGMENTS[0].y;
					goto insert_new_segment;
				case KEY_RIGHT:
					new_segment.x = SEGMENTS[0].x - 1;
					new_segment.y = SEGMENTS[0].y;
					goto insert_new_segment;
			}
		}
		else {
			size_t x_diff = SEGMENTS[LAST_SEGMENT_INDEX - 1].x - SEGMENTS[LAST_SEGMENT_INDEX].x;
			if(x_diff != 0) {
				new_segment.x = SEGMENTS[LAST_SEGMENT_INDEX].x - x_diff;
				new_segment.y = SEGMENTS[LAST_SEGMENT_INDEX].y;
				goto insert_new_segment;
			}
			size_t y_diff = SEGMENTS[LAST_SEGMENT_INDEX - 1].y - SEGMENTS[LAST_SEGMENT_INDEX].y;
			if(y_diff != 0) {
				new_segment.x = SEGMENTS[LAST_SEGMENT_INDEX].x;
				new_segment.y = SEGMENTS[LAST_SEGMENT_INDEX].y - y_diff;
				goto insert_new_segment;
			}
		}
		insert_new_segment:;
		SCORE_COUNTER++;
		if(
			SCORE_COUNTER == GAME_H * GAME_W - 1 &&
			(
				new_segment.x == 0 ||
				new_segment.x == GAME_W ||
				new_segment.y == 0 ||
				new_segment.y == GAME_H
			)
		) {
			goto last_apple;
		}
		if(
			SCORE_COUNTER == GAME_H * GAME_W ||
			SCORE_COUNTER == GAME_H * GAME_W - 1 
		) {
			return -1;
		}
		LAST_SEGMENT_INDEX += 1;
		SEGMENTS[LAST_SEGMENT_INDEX] = new_segment;
		last_apple:;
		update_apple_coords();
	}
	return 0;
}


size_t update_snake_coords(int signal) {
	if(signal == 'q') {
		return 0;
	}
	if(signal != -1) {
		CURRENT_DIRECTION = signal;
	}
	for(size_t segment_index = LAST_SEGMENT_INDEX; segment_index > 0; segment_index--) {
		SEGMENTS[segment_index].x = SEGMENTS[segment_index - 1].x;
		SEGMENTS[segment_index].y = SEGMENTS[segment_index - 1].y;
	}
	switch(CURRENT_DIRECTION) {
		case KEY_UP:
			SEGMENTS[0].y -= 1;
			break;
		case KEY_DOWN:
			SEGMENTS[0].y += 1;
			break;
		case KEY_LEFT:
			SEGMENTS[0].x -= 1;
			break;
		case KEY_RIGHT:
			SEGMENTS[0].x += 1;
			break;
	}
	for(size_t segment_index = 1; segment_index <= LAST_SEGMENT_INDEX; segment_index++) {
		if(
			SEGMENTS[0].x == SEGMENTS[segment_index].x &&
			SEGMENTS[0].y == SEGMENTS[segment_index].y
		) {
			return 0;
		}
	}
	if(
		SEGMENTS[0].x == 0 ||
		SEGMENTS[0].x == GAME_W + 1 ||
		SEGMENTS[0].y == 0 ||
		SEGMENTS[0].y == GAME_H + 1
	) {
		return 0;
	}
	return 1;
}


int main(void) {
	prepare();
	srand(time(NULL));
	struct ThaCoord head_coords;
	head_coords.x = (GAME_W - 1) / 2 + 1;
	head_coords.y = (GAME_H - 1) / 2 + 1;
	SEGMENTS[LAST_SEGMENT_INDEX] = head_coords;
	APPLE_COORDS.x = 0;
	APPLE_COORDS.y = 0;
	print_data();
	int first_signal;
	do {
		first_signal = getch();
		if(first_signal == 'q') {
			goto game_over;
		}
	}
	while(
		first_signal != KEY_UP &&
		first_signal != KEY_DOWN &&
		first_signal != KEY_LEFT &&
		first_signal != KEY_RIGHT
	);
	update_snake_coords(first_signal);
	update_apple_coords();
	clear();
	print_data();
	#ifndef _WIN32
		timeout(525);
	#endif
	size_t winner_winner_apple_dinner = 0;
	int signal = get_signal();
	while(
		update_snake_coords(signal) &&
		(winner_winner_apple_dinner = is_apple_eaten()) == 0
	) {
		clear();
		print_data();
		signal = get_signal();
	}
	game_over:;
	clear();
	if(winner_winner_apple_dinner) {
		print_chars(
			"WINNER WINNER APPLE DINNER\nYour score: %zu\nPress any button to exit\n",
			SCORE_COUNTER
		);
	}
	else {
		print_chars(
			"Game over.\nYour score: %zu\nPress any button to exit\n",
			SCORE_COUNTER
		);
	}
	#ifndef _WIN32
		refresh();
		timeout(-1);
	#endif
	getchar();
	#ifndef _WIN32
		endwin();
	#endif
	return 0;
}