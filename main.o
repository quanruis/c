/* main.c -- the screen. ncurses, no dependencies beyond that.
 *
 * rime is a game about being the only warm thing in a cold place.
 * you can't fight your way out and you can't dig; you can only spend
 * your own heat to melt a way through, and the vault wakes up when
 * you do.
 */

#include <curses.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sim.h"

#define NEEDW 44
#define NEEDH 24
#define LASTDEPTH 7

#define C_TITLE  1
#define C_WALL   2
#define C_FLOOR  3
#define C_ICE    4
#define C_WATER  5
#define C_RIME   6
#define C_ME     7
#define C_ASLEEP 8
#define C_AWAKE  9
#define C_FIRE   10
#define C_COLD   11
#define C_STAIR  12
#define C_VENT   13

static int usecol = 1;
static struct game G;

static void onint(int sig)
{
	(void)sig;
	endwin();
	_exit(1);
}

static void usage(const char *argv0)
{
	printf("usage: %s [--seed N] [--no-color] [--help]\n", argv0);
}

static int parse_args(int argc, char **argv, uint32_t *seed)
{
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--no-color"))
			usecol = 0;
		else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
			return 1;
		else if (!strcmp(argv[i], "--seed") && i + 1 < argc)
			*seed = (uint32_t)strtoul(argv[++i], NULL, 10);
		else if (!strncmp(argv[i], "--seed=", 7))
			*seed = (uint32_t)strtoul(argv[i] + 7, NULL, 10);
		else {
			fprintf(stderr, "unknown argument: %s\n", argv[i]);
			usage(argv[0]);
			return -1;
		}
	}
	return 0;
}

static void paint(void)
{
	if (!usecol || !has_colors()) {
		usecol = 0;
		return;
	}
	start_color();
	use_default_colors();
	init_pair(C_TITLE,  COLOR_CYAN,    -1);
	init_pair(C_WALL,   COLOR_BLUE,    -1);
	init_pair(C_FLOOR,  COLOR_WHITE,   -1);
	init_pair(C_ICE,    COLOR_CYAN,    -1);
	init_pair(C_WATER,  COLOR_BLUE,    -1);
	init_pair(C_RIME,   COLOR_WHITE,   -1);
	init_pair(C_ME,     COLOR_YELLOW,  -1);
	init_pair(C_ASLEEP, COLOR_GREEN,   -1);
	init_pair(C_AWAKE,  COLOR_RED,     -1);
	init_pair(C_FIRE,   COLOR_YELLOW,  -1);
	init_pair(C_COLD,   COLOR_MAGENTA, -1);
	init_pair(C_STAIR,  COLOR_YELLOW,  -1);
	init_pair(C_VENT,   COLOR_MAGENTA, -1);
}

static void put(int y, int x, int pair, int attr, const char *s)
{
	if (usecol)
		attron(COLOR_PAIR(pair) | attr);
	else if (attr)
		attron(attr);
	mvaddstr(y, x, s);
	if (usecol)
		attroff(COLOR_PAIR(pair) | attr);
	else if (attr)
		attroff(attr);
}

static void bar(int y, int x, int val, int max, int width, int pair)
{
	char cell[2];
	int n = val * width / max, i;

	cell[1] = 0;
	put(y, x, C_FLOOR, 0, "[");
	for (i = 0; i < width; i++) {
		cell[0] = i < n ? '#' : '.';
		put(y, x + 1 + i, i < n ? pair : C_WALL, i < n ? A_BOLD : 0, cell);
	}
	put(y, x + width + 1, C_FLOOR, 0, "]");
}

static char glyph(const struct game *g, int x, int y, int *pair, int *attr)
{
	int i;

	*attr = 0;
	*pair = C_FLOOR;

	for (i = 0; i < g->nflames; i++)
		if (g->flames[i].x == x && g->flames[i].y == y) {
			*pair = C_FIRE;
			*attr = A_BOLD;
			return '*';
		}
	for (i = 0; i < g->nbraz; i++)
		if (g->braz[i].x == x && g->braz[i].y == y && !g->braz[i].lit) {
			*pair = C_COLD;
			return '%';
		}
	for (i = 0; i < g->nvents; i++)
		if (g->vents[i].x == x && g->vents[i].y == y) {
			*pair = C_VENT;
			return '^';
		}
	for (i = 0; i < g->nmobs; i++)
		if (g->mobs[i].x == x && g->mobs[i].y == y) {
			*pair = g->mobs[i].awake ? C_AWAKE : C_ASLEEP;
			*attr = g->mobs[i].awake ? A_BOLD : 0;
			return g->mobs[i].awake ? 'W' : '&';
		}

	switch (g->map[y][x]) {
	case T_WALL:   *pair = C_WALL;  return '#';
	case T_ICE:    *pair = C_ICE;   return '=';
	case T_WATER:  *pair = C_WATER; return '~';
	case T_STAIRS: *pair = C_STAIR; *attr = A_BOLD; return '>';
	case T_RIME:
		{
			int ri = rime_at(g, x, y);
			int sides = ri >= 0 ? g->rimes[ri].sides : 0;

			*pair = C_RIME;
			if (sides >= 12)
				return 'H';
			if (sides >= 8)
				return 'Y';
			if (sides >= 4)
				return 'U';
			return 'n';
		}
	default:       *pair = C_FLOOR; return '.';
	}
}

static void draw(void)
{
	char hud[96];
	int x, y, pair, attr, t;

	erase();

	put(0, 0, C_TITLE, A_BOLD, " R I M E ");
	put(0, 10, C_FLOOR, 0, "you are the only warm thing down here");

	for (y = 0; y < MAPH; y++) {
		for (x = 0; x < MAPW; x++) {
			char c = glyph(&G, x, y, &pair, &attr);

			if (usecol)
				attron(COLOR_PAIR(pair) | attr);
			else if (attr)
				attron(attr);
			mvaddch(1 + y, x, (chtype)c);
			if (usecol)
				attroff(COLOR_PAIR(pair) | attr);
			else if (attr)
				attroff(attr);
		}
	}

	if (usecol)
		attron(COLOR_PAIR(C_ME) | A_BOLD);
	mvaddch(1 + G.py, G.px, '@');
	if (usecol)
		attroff(COLOR_PAIR(C_ME) | A_BOLD);

	t = (int)G.heat[G.py][G.px] - 128;
	snprintf(hud, sizeof hud, "heat %3d", G.body);
	put(MAPH + 2, 0, C_FLOOR, 0, hud);
	bar(MAPH + 2, 8, G.body > 0 ? G.body : 0, BODY_MAX, 20,
	    G.body < 60 ? C_AWAKE : C_ME);
	snprintf(hud, sizeof hud, "floor %+d    depth %d    turn %ld    braziers %d",
		 t, G.depth, G.turn, G.braziers);
	put(MAPH + 2, 31, C_FLOOR, 0, hud);

	if (G.nmsg > 0)
		put(MAPH + 4, 0, C_FLOOR, 0, G.msgs[G.nmsg - 1]);
	if (G.nmsg > 1)
		put(MAPH + 5, 0, C_WALL, 0, G.msgs[G.nmsg - 2]);

	put(MAPH + 7, 0, C_WALL, 0,
	    "hjkl move  H huddle  b set brazier  p pick it up  f relight  , look  > down  ? help");
	refresh();
}

static void panel(const char *title, const char **lines, int n)
{
	int w = (int)strlen(title), i, x0, y0;

	for (i = 0; i < n; i++) {
		int l = (int)strlen(lines[i]);

		if (l > w)
			w = l;
	}
	w += 4;
	if (w > COLS)
		w = COLS;
	x0 = (COLS - w) / 2;
	y0 = (LINES - (n + 5)) / 2;
	if (x0 < 0)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (y0 + n + 5 > LINES)
		y0 = LINES - (n + 5);
	if (y0 < 0)
		y0 = 0;

	attron(A_REVERSE);
	for (i = 0; i < n + 5 && y0 + i < LINES; i++)
		mvprintw(y0 + i, x0, "%*s", w, "");
	attroff(A_REVERSE);

	attron(A_UNDERLINE);
	mvprintw(y0 + 1, x0 + 2, "%s", title);
	attroff(A_UNDERLINE);

	for (i = 0; i < n && y0 + 3 + i < LINES; i++)
		mvprintw(y0 + 3 + i, x0 + 2, "%.*s", w - 4, lines[i]);

	if (y0 + n + 4 < LINES)
		mvprintw(y0 + n + 4, x0 + 2, "-- any key --");
	refresh();
	getch();
}

static void show_help(void)
{
	static const char *l[] = {
		"you are warm. the vault is not. that is the whole problem.",
		"",
		" hjkl / arrows  walk         H  huddle: stay put, warm the floor",
		" b  set a brazier down (20)  p  pick a brazier of yours up (5)",
		" f  relight a cold brazier beside you (25)     ,  look    >  down",
		"",
		" .  cold floor   =  ice      ~  meltwater      >  the stair down",
		" H  rime, solid  Y U n  the same rime, melting away",
		" &  one of the Wept, frozen          W  one of the Wept, awake",
		" %  cold brazier  *  burning         ^  a vent, warm for ever",
		"",
		"rime cannot be broken or walked through, only melted, and the only",
		"heat down here is yours. fire takes a few turns, huddling thirty,",
		"and both spend the thing keeping you alive.",
		"",
		"warmth wakes the Wept and they walk as fast as you, so running is",
		"not a plan. walk into one to shove it: it costs heat and leaves it",
		"reeling. shove them into meltwater and the vault keeps them.",
	};

	panel("how it works", l, (int)(sizeof l / sizeof l[0]));
}

static void show_end(int escaped)
{
	const char *l[7];
	char a[80], b[80], c[80];
	int n = 0;

	snprintf(a, sizeof a, "%s",
		 escaped ? "daylight, of all things" :
		 G.dead ? G.epitaph : "you found the stair down");
	snprintf(b, sizeof b, "depth reached: %d", G.depth);
	snprintf(c, sizeof c, "turns taken:   %ld", G.turn);
	l[n++] = a;
	l[n++] = "";
	l[n++] = b;
	l[n++] = c;
	l[n++] = "";
	l[n++] = escaped ? "you came up with nothing and you came up warm." :
		 G.dead ? "the vault keeps what it is given." : "and down again.";

	panel(escaped ? "out" : G.dead ? "cold" : "down", l, n);
}

static int keymap(int ch)
{
	switch (ch) {
	case 'h': case '4': case KEY_LEFT:  return A_W;
	case 'j': case '2': case KEY_DOWN:  return A_S;
	case 'k': case '8': case KEY_UP:    return A_N;
	case 'l': case '6': case KEY_RIGHT: return A_E;
	case ' ': case '.': case '5':       return A_WAIT;
	case 'H':                           return A_HUDDLE;
	case 'b': case 'B':                 return A_BRAZIER;
	case 'p': case 'P':                 return A_PICKUP;
	case 'f': case 'F':                 return A_RELIGHT;
	case ',': case ';':                 return A_EXAMINE;
	case '>':                           return A_DESCEND;
	case '?':                           return -2;
	case 'q': case 'Q':                 return A_QUIT;
	default:                            return A_NONE;
	}
}

int main(int argc, char **argv)
{
	uint32_t seed;
	int ch, act, escaped = 0;
	int r;

	setlocale(LC_ALL, "");
	seed = (uint32_t)time(NULL) ^ ((uint32_t)getpid() << 16);
	r = parse_args(argc, argv, &seed);
	if (r) {
		if (r > 0)
			usage(argv[0]);
		return r > 0 ? 0 : 2;
	}

	memset(&G, 0, sizeof G);
	G.body = BODY_START;
	G.braziers = 4;
	if (!gen_level(&G, 1, seed)) {
		fprintf(stderr, "could not build a vault, try another seed\n");
		return 1;
	}

	initscr();
	signal(SIGINT, onint);
	cbreak();
	noecho();
	nonl();
	keypad(stdscr, TRUE);
	paint();

	if (COLS < NEEDW || LINES < NEEDH) {
		endwin();
		fprintf(stderr, "rime wants a terminal of at least %dx%d, this one is %dx%d\n",
			NEEDW, NEEDH, COLS, LINES);
		return 1;
	}

	say(&G, "depth 1. find the stair, and expect it to be blocked");
	say(&G, "press ? for how it works");

	for (;;) {
		draw();

		ch = getch();
		if (ch == ERR)
			break;

		act = keymap(ch);
		if (act == -2) {
			show_help();
			continue;
		}
		if (act == A_NONE)
			continue;

		if (act == A_QUIT) {
			G.dead = 1;
			snprintf(G.epitaph, sizeof G.epitaph,
				 "you sat down on depth %d and stopped moving", G.depth);
			break;
		}

		do_turn(&G, act);

		if (G.over) {
			G.over = 0;
			if (G.depth >= LASTDEPTH) {
				escaped = 1;
				break;
			}
			if (!gen_level(&G, G.depth + 1, seed + (uint32_t)G.depth * 104729u)) {
				G.dead = 1;
				snprintf(G.epitaph, sizeof G.epitaph,
					 "the vault below would not build itself");
				break;
			}
			say(&G, "depth %d. colder than the last", G.depth);
			say(&G, "you kept %d heat and %d braziers", G.body, G.braziers);
			continue;
		}
		if (G.dead)
			break;
	}

	show_end(escaped);
	endwin();
	printf("%s on depth %d after %ld turns\n",
	       escaped ? "out" : G.dead ? "cold" : "down",
	       G.depth, G.turn);
	return 0;
}
