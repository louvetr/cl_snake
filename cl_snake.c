#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#define DELAY 500000

// playground size
#define pg_max_x 20
#define pg_max_y 15

enum snake_dir {
	DIR_UP,
	DIR_DOWN,
	DIR_LEFT,
	DIR_RIGHT,
};

#define c_empty ' '
#define c_snake 'O'
#define c_head '0'
#define c_border '#'
#define c_apple '@'
#define c_filler '.'

// Structure def

//////////////////////////////////////////////////////////////////////////////////
//         Double Linked List stuff
//////////////////////////////////////////////////////////////////////////////////

struct dll_node {
	unsigned int val;
	struct dll_node *next;
	struct dll_node *prev;
};

// Functions

struct dll_node *dll_insert_first(struct dll_node *sentinel, unsigned int val)
{
	struct dll_node *node;

	node = calloc(1, sizeof(*node));
	if (node == NULL) {
		printf("[%s] calloc failed\n", __func__);
		return NULL;
	}

	node->val = val;
	if (sentinel->next == NULL)
		node->next = sentinel;
	else
		node->next = sentinel->next;
	node->prev = sentinel;
	if (sentinel->prev == NULL)
		sentinel->prev = node;
	else
		sentinel->next->prev = node;
	sentinel->next = node;

	return node;
}

struct dll_node *dll_insert_last(struct dll_node *sentinel, unsigned int val)
{
	struct dll_node *node;

	node = calloc(1, sizeof(*node));
	if (node == NULL) {
		printf("[%s] calloc failed\n", __func__);
		return NULL;
	}

	node->val = val;
	node->next = sentinel;
	if (sentinel->prev == NULL)
		node->prev = sentinel;
	else
		node->prev = sentinel->prev;
	if (sentinel->next == NULL)
		sentinel->next = node;
	else
		sentinel->prev->next = node;
	sentinel->prev = node;

	return node;
}

void dll_delete_last(struct dll_node *sentinel)
{
	struct dll_node *node;

	node = sentinel->prev;

	if (node->prev == sentinel) {
		sentinel->prev = NULL;
		sentinel->next = NULL;
	} else {
		sentinel->prev = node->prev;
		node->prev->next = sentinel;
	}

	free(node);
}

void dll_print(struct dll_node *sentinel)
{
	struct dll_node *tmp = sentinel;

	printf("(S)");

	do {
		tmp = tmp->next;
		printf("->(%u)", tmp->val);
	} while (tmp->val != ~0);
	printf("\n");
}

struct dll_node *dll_get_last(struct dll_node *sentinel)
{
	return sentinel->prev;
}

// local variable
struct dll_node sentinel = {
	.val = ~0,
	.next = NULL,
	.prev = NULL,
};

struct dll_node *tmp_node;

//////////////////////////////////////////////////////////////////////////////////
//         Snake game stuff
//////////////////////////////////////////////////////////////////////////////////

static inline int rnd_in_range(int min, int max)
{
	return rand() % (max - min + 1) + min;
}

int main()
{
	// stuff
	int i, j, rnd_min, rnd_max, exit = 0;
	// screen info
	int max_x, max_y, screen_size, pg_size;
	// game info
	int head_pos, tail_pos, snake_lg = 4, apple_pos;
	enum snake_dir dir = DIR_RIGHT;

	char *screen = NULL, *pg = NULL;

	// seed for random
	srand(time(0));
	rnd_min = pg_max_x + 2;
	rnd_max = pg_max_x * pg_max_y - pg_max_x - 2;

	// NCurses setup
	setlocale(LC_ALL, ""); // Set locale for UTF-8 support
	initscr(); // Initialise NCurses screen
	noecho(); // Don't echo input to screen
	curs_set(0); // Don't show terminal cursor
	nodelay(stdscr, true); // Don't halt program while waiting for input
	cbreak(); // Make input characters immediately available to the program
	getmaxyx(stdscr, max_y, max_x);

	if (max_x < pg_max_x || max_y < pg_max_y) {
		printf("max_x = %d, max_y = %d", max_x, max_y);
		printf("ERROR: window size as to be at least 40x40.\n");
		goto exit;
	}

	// offset is where to display playground in screen to put it in center
	int offset =
		max_x / 2 - pg_max_x / 2 + (max_y / 2 - pg_max_y / 2) * max_x;

	// allocate screen
	screen_size = max_x * max_y + 1;
	screen = calloc(1, screen_size);
	if (screen == NULL) {
		printf("ERROR: calloc failed.\n");
		return -1;
	}
	// init screen
	for (i = 0; i < screen_size - 1; i++) {
		screen[i] = c_filler;
	}

	// allocate playground
	pg_size = pg_max_x * pg_max_y;
	pg = calloc('#', pg_size);
	if (pg == NULL) {
		printf("ERROR: calloc failed.\n");
		return -1;
	}
	// init playground
	for (i = 0; i < pg_size; i++) {
		if (i < pg_max_x || i > pg_size - pg_max_x ||
		    i % pg_max_x == 0 || i % pg_max_x == pg_max_x - 1)
			pg[i] = c_border;
		else
			pg[i] = c_empty;
	}

	// init snake position within pg
	head_pos = pg_max_x * pg_max_y / 2;
	tail_pos = pg_max_x * pg_max_y / 2 - snake_lg;
	for (i = tail_pos; i <= head_pos; i++) {
		pg[i] = c_head;
		dll_insert_first(&sentinel, i);
	}

	// init 1st apple position
	do {
		apple_pos = rnd_in_range(rnd_min, rnd_max);
	} while (pg[apple_pos] != c_empty);
	pg[apple_pos] = c_apple;

	// put playground into screen
	for (j = 0; j < pg_max_y; j++) {
		for (i = 0; i < pg_max_x; i++) {
			screen[i + j * max_x + offset] = pg[i + j * pg_max_x];
		}
	}

	// game loop
	while (!exit) {
		clear();
		// game timing =================================
		usleep(100000);

		// input =======================================
		int key = getch();
		switch (key) {
		case 'q': // left
			head_pos--;
			dir = DIR_LEFT;
			break;
		case 'd': // right
			head_pos++;
			dir = DIR_RIGHT;
			break;
		case 's': // down
			head_pos += pg_max_x;
			dir = DIR_DOWN;
			break;
		case 'z': // up
			head_pos -= pg_max_x;
			dir = DIR_UP;
			break;
		case 'o': // exit
			exit = 1;
			break;
		default: // no input, snake goes ahead
			switch (dir) {
			case DIR_LEFT:
				head_pos--;
				break;
			case DIR_RIGHT:
				head_pos++;
				break;
			case DIR_DOWN:
				head_pos += pg_max_x;
				break;
			case DIR_UP:
				head_pos -= pg_max_x;
				break;
			default:
				break;
			}
			break;
		}

		// game logic ==================================

		// action depending new head position
		switch (pg[head_pos]) {
		case c_border: // game over
		case c_snake: // game over
			exit = 1;
			break;
		case c_empty: // move on, slide the tail
			// pg[tail_pos] = c_empty;
			pg[head_pos] = c_head;
			dll_insert_first(&sentinel, head_pos);
			tmp_node = dll_get_last(&sentinel);
			pg[tmp_node->val] = c_empty;
			dll_delete_last(&sentinel);
			break;
		case c_apple: // apple eaten
			pg[head_pos] = c_head;
			dll_insert_first(&sentinel, head_pos);
			snake_lg++;
			do {
				apple_pos = rnd_in_range(rnd_min, rnd_max);
			} while (pg[apple_pos] != c_empty);
			pg[apple_pos] = c_apple;
			break;
		default: // shall not happen
			break;
		}

		// display =====================================

		// put playground into screen
		for (j = 0; j < pg_max_y; j++) {
			for (i = 0; i < pg_max_x; i++) {
				screen[i + j * max_x + offset] =
					pg[i + j * pg_max_x];
			}
		}

		mvaddstr(0, 0, screen);
		mvprintw(1, 3, "--Controls:--------");
		mvprintw(2, 3, "| 'z': move up    |");
		mvprintw(3, 3, "| 's': move down  |");
		mvprintw(4, 3, "| 'q': move left  |");
		mvprintw(5, 3, "| 'd': move right |");
		mvprintw(6, 3, "| 'o': quit game  |");
		mvprintw(7, 3, "-------------------");

		char score_str[21];
		sprintf(score_str, "%d pts. |", snake_lg-4);

		mvprintw(9, 3, "--Score:------------");
		mvprintw(10, 3, "| %19s", score_str);
		mvprintw(11, 3, "-------------------");
		refresh();
		usleep(100000);
	}

exit:
	endwin();
	free(screen);
	free(pg);

	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	printf("               GAME OVER !!!\n");
	printf("\n");
	printf("               SCORE = %d \n", snake_lg-4);
	printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

	// TODO: free Double Linked List elements

	return 0;
}