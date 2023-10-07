#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#define H 15
#define W 25


#ifdef _WIN32
	#include <conio.h>
	#define print_chars printf


	typedef enum {
		KEY_UP = 38,
		KEY_DOWN = 40,
		KEY_LEFT = 37,
		KEY_RIGHT = 39
	} Directions;
	

	// Nie jestem pewien, czy pod windowsem trzeba coś przygotować
	void prepare() {

	}


	void clear() {

	}


	// Zaimplementuj to
	char get_signal() {
		return 0;
	}
#else
	#include <ncurses.h>
	#define print_chars printw
	#define get_signal getch
	

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


struct ThaCoord SEGMENTS[H * W];
struct ThaCoord APPLE_COORDS;
size_t LAST_SEGMENT_INDEX = 0;
size_t SCORE_COUNTER = 0;
int CURRENT_DIRECTION = 0;


void print_data() {
	for(size_t n = 0; n < H + 2; n++) {
		for(size_t m = 0; m < W + 2; m++) {
			if(n == 0 || n == H + 1 || m == 0 || m == W + 1) {
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
			no_space:
		}
		print_chars("\n");
	}
	print_chars("Score => %ld", SCORE_COUNTER);
}


void update_apple_coords() {
	size_t counter = 0;
	size_t free_coords_count = H * W - (LAST_SEGMENT_INDEX + 1);
	size_t random_num = rand() % free_coords_count;
	for(size_t n = 1; n <= H; n++) {
		for(size_t m = 1; m <= W; m++) {
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
			coords_not_free:
		}
	}
	updated:
}


void is_apple_eaten() {
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
		insert_new_segment:
		LAST_SEGMENT_INDEX += 1;
		SEGMENTS[LAST_SEGMENT_INDEX] = new_segment;
		SCORE_COUNTER++;
		update_apple_coords();
	}
}


size_t update_snake_coords(int signal) {
	if(signal == 'q') {
		return 0;
	}
	if((LAST_SEGMENT_INDEX == 0 || signal != CURRENT_DIRECTION) && signal != -1) {
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
	// Check if the first segment has the same coords as any other segment.
	for(size_t segment_index = 1; segment_index <= LAST_SEGMENT_INDEX; segment_index++) {
		if(
			SEGMENTS[0].x == SEGMENTS[segment_index].x &&
			SEGMENTS[0].y == SEGMENTS[segment_index].y
		) {
			return 0;
		}
	}
	// Check if the head hit the border.
	if(
		SEGMENTS[0].x == 0 ||
		SEGMENTS[0].x == W + 1 ||
		SEGMENTS[0].y == 0 ||
		SEGMENTS[0].y == H + 1
	) {
		return 0;
	}
	return 1;
}


int main(void) {
	prepare();
	srand(time(NULL));
	struct ThaCoord head_coords;
	head_coords.x = (W - 1) / 2 + 1;
	head_coords.y = (H - 1) / 2 + 1;
	SEGMENTS[LAST_SEGMENT_INDEX] = head_coords;
	APPLE_COORDS.x = 0;
	APPLE_COORDS.y = 0;
	print_data();
	int first_signal;
	do {
		first_signal = get_signal();
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
		timeout(550);
	#endif
	int signal = get_signal();
	while(
		update_snake_coords(signal)
	) {
		is_apple_eaten();
		clear();
		print_data();
		signal = get_signal();
	}
	game_over:
	clear();
	print_chars(
		"Game over.\nYour score: %ld\nPress any button to exit\n",
		SCORE_COUNTER
	);
	refresh();
	#ifndef _WIN32
		timeout(-1);
	#endif
	getchar();
	endwin();
	return 0;
}