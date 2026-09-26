/* sim.c -- heat, phase changes and the things that move when it's warm.
 *
 * everything in here is deterministic for a given seed. the ui is a
 * thin shell on top, so the whole game can be driven from a test or
 * a pipe of keystrokes without a terminal.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "sim.h"

#define NABS(x) ((x) < 0 ? -(x) : (x))

uint32_t rnd_next(uint32_t *s)
{
	uint32_t x = *s;
	if (!x)
		x = 0x9e3779b9u;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*s = x;
	return x;
}

void say(struct game *g, const char *fmt, ...)
{
	va_list ap;
	char *slot;

	if (g->nmsg >= 6) {
		memmove(g->msgs[0], g->msgs[1], 5 * sizeof g->msgs[0]);
		g->nmsg = 5;
	}
	slot = g->msgs[g->nmsg++];
	va_start(ap, fmt);
	vsnprintf(slot, sizeof g->msgs[0], fmt, ap);
	va_end(ap);
}

int rime_at(const struct game *g, int x, int y)
{
	int i;

	for (i = 0; i < g->nrimes; i++)
		if (g->rimes[i].x == x && g->rimes[i].y == y)
			return i;
	return -1;
}

static int nopen(const struct game *g, int x, int y)
{
	static const int dx[4] = { 1, -1, 0, 0 };
	static const int dy[4] = { 0, 0, 1, -1 };
	int i, n = 0;

	for (i = 0; i < 4; i++)
		if (passable(g, x + dx[i], y + dy[i]))
			n++;
	return n;
}

/* what goes into the meltwater does not come out of it again, which
 * gives you a way to clear a room without spending heat on a fight:
 * shove them in and the vault keeps them. */
static void drown(struct game *g, int i)
{
	say(g, "one of the Wept goes under and the water takes it back");
	g->map[g->mobs[i].y][g->mobs[i].x] = T_ICE;
	g->heat[g->mobs[i].y][g->mobs[i].x] = H_FREEZE - 10;
	g->mobs[i] = g->mobs[g->nmobs - 1];
	g->nmobs--;
}

static int mob_at(const struct game *g, int x, int y)
{
	int i;

	for (i = 0; i < g->nmobs; i++)
		if (g->mobs[i].x == x && g->mobs[i].y == y)
			return i;
	return -1;
}

/* rime is the only thing on the map that blocks a corridor, and it
 * only ever goes away by being warmed. that's the whole game. */
int passable(const struct game *g, int x, int y)
{
	if (x < 0 || y < 0 || x >= MAPW || y >= MAPH)
		return 0;
	if (g->map[y][x] == T_WALL)
		return 0;
	if (g->map[y][x] == T_RIME && rime_at(g, x, y) >= 0)
		return 0;
	return 1;
}

static int conducts(uint8_t t)
{
	switch (t) {
	case T_WATER: return 16;
	case T_ICE:   return 9;
	case T_FLOOR: return 7;
	case T_STAIRS:return 7;
	case T_RIME:  return 5;
	default:      return 1;   /* wall */
	}
}

void apply_heat(struct game *g, int x, int y, int amt, int per)
{
	int ri;

	if (x < 0 || y < 0 || x >= MAPW || y >= MAPH)
		return;
	if (g->map[y][x] == T_WALL)
		return;

	if (g->heat[y][x] < H_MAX - amt)
		g->heat[y][x] = (uint8_t)(g->heat[y][x] + amt);
	else
		g->heat[y][x] = H_MAX;

	ri = rime_at(g, x, y);
	if (ri < 0 || per <= 0)
		return;
	if (g->rimes[ri].units <= 0)
		return;
	if (g->heat[y][x] < H_MELT)
		return;

	g->rimes[ri].units -= per;
	if (g->rimes[ri].units <= 0) {
		g->map[y][x] = T_WATER;
		g->heat[y][x] = H_MELT;
		/* it is water now, not a plug. drop the record so nothing
		 * keeps treating the tile as something that blocks a door. */
		g->rimes[ri] = g->rimes[g->nrimes - 1];
		g->nrimes--;
		say(g, "the rime lets go and runs as water");
	} else {
		int gone = 0, i, m;

		if (g->rimes[ri].units <= 27 && g->rimes[ri].sides == 15)
			gone = 1;
		if (g->rimes[ri].units <= 18 && g->rimes[ri].sides == 14)
			gone = 1;
		if (g->rimes[ri].units <= 9 && g->rimes[ri].sides == 12)
			gone = 1;
		if (gone) {
			m = g->rimes[ri].sides;
			for (i = 0; i < 4; i++)
				if (m & (1 << i)) {
					m &= ~(1 << i);
					break;
				}
			g->rimes[ri].sides = m;
			say(g, "a face of the rime cracks away");
		}
	}
}

static void diffuse(struct game *g)
{
	static uint8_t nb[MAPH][MAPW];
	int x, y, i;

	memcpy(nb, g->heat, sizeof nb);
	for (y = 1; y < MAPH - 1; y++) {
		for (x = 1; x < MAPW - 1; x++) {
			int acc = 0, sum = 0;
			static const int dx[4] = { 1, -1, 0, 0 };
			static const int dy[4] = { 0, 0, 1, -1 };

			for (i = 0; i < 4; i++) {
				int tx = x + dx[i], ty = y + dy[i];
				int k = conducts(g->map[y][x]);
				int kt = conducts(g->map[ty][tx]);
				int w = k < kt ? k : kt;
				int d = (int)g->heat[ty][tx] - (int)g->heat[y][x];

				acc += w * d;
				sum += w;
			}
			if (sum)
				nb[y][x] = (uint8_t)((int)g->heat[y][x] + acc / (sum * 8));
		}
	}

	for (y = 1; y < MAPH - 1; y++) {
		for (x = 1; x < MAPW - 1; x++) {
			int v = nb[y][x];

			if (g->map[y][x] == T_WALL)
				continue;
			v -= FLOOR_COOL;
			if (v < AMBIENT)
				v = AMBIENT;
			if (v > H_MAX)
				v = H_MAX;
			g->heat[y][x] = (uint8_t)v;
		}
	}
}

static void phases(struct game *g)
{
	int x, y;

	for (y = 1; y < MAPH - 1; y++) {
		for (x = 1; x < MAPW - 1; x++) {
			if (g->map[y][x] == T_ICE && g->heat[y][x] >= H_MELT) {
				g->map[y][x] = T_WATER;
				g->heat[y][x] = H_MELT - 6;
			} else if (g->map[y][x] == T_WATER && g->heat[y][x] <= H_FREEZE) {
				g->map[y][x] = T_ICE;
				g->heat[y][x] = H_FREEZE - 8;
			}
		}
	}
}

static void flames_tick(struct game *g)
{
	int i;

	for (i = g->nflames - 1; i >= 0; i--) {
		if (--g->flames[i].life > 0)
			continue;
		if (g->flames[i].brazier >= 0) {
			g->braz[g->flames[i].brazier].lit = 0;
			say(g, "a relit brazier gutters out");
		} else {
			say(g, "a brazier gutters out");
		}
		g->flames[i] = g->flames[g->nflames - 1];
		g->nflames--;
	}
}

/* rime is not warmed by the air around it, it is warmed by something
 * warm pressing on it. a flame reaches one tile out; so does a body
 * curled up against it. */
static void radiate(struct game *g, int x, int y, int amt, int per)
{
	static const int dx[4] = { 1, -1, 0, 0 };
	static const int dy[4] = { 0, 0, 1, -1 };
	int i;

	for (i = 0; i < 4; i++)
		apply_heat(g, x + dx[i], y + dy[i], amt, per);
}

static void sources(struct game *g)
{
	int i;

	if (g->huddling) {
		apply_heat(g, g->px, g->py, WARM_HUDDLE, RIME_PERSTEP);
		radiate(g, g->px, g->py, WARM_HUDDLE * 2 / 3, RIME_PERSTEP);
	} else {
		apply_heat(g, g->px, g->py, WARM_OUT, 0);
	}

	for (i = 0; i < g->nflames; i++) {
		apply_heat(g, g->flames[i].x, g->flames[i].y, FLAME_OUT, RIME_PERFLAME);
		radiate(g, g->flames[i].x, g->flames[i].y, FLAME_OUT, RIME_PERFLAME);
	}
	for (i = 0; i < g->nvents; i++) {
		apply_heat(g, g->vents[i].x, g->vents[i].y, VENT_OUT, 0);
		radiate(g, g->vents[i].x, g->vents[i].y, VENT_OUT / 2, 0);
	}
}

static void mob_turn(struct game *g)
{
	static int dist[MAPH][MAPW];
	static int qx[MAPH * MAPW], qy[MAPH * MAPW];
	static const int dx[4] = { 1, -1, 0, 0 };
	static const int dy[4] = { 0, 0, 1, -1 };
	int head = 0, tail = 0, i, k;

	memset(dist, 0x7f, sizeof dist);
	dist[g->py][g->px] = 0;
	qx[tail] = g->px;
	qy[tail] = g->py;
	tail++;

	while (head < tail) {
		int x = qx[head], y = qy[head];
		head++;
		for (k = 0; k < 4; k++) {
			int nx = x + dx[k], ny = y + dy[k];

			if (nx < 0 || ny < 0 || nx >= MAPW || ny >= MAPH)
				continue;
			if (!passable(g, nx, ny))
				continue;
			if (dist[ny][nx] <= dist[y][x] + 1)
				continue;
			dist[ny][nx] = dist[y][x] + 1;
			qx[tail] = nx;
			qy[tail] = ny;
			tail++;
		}
	}

	for (i = 0; i < g->nmobs; i++)
		if (!g->mobs[i].awake && g->heat[g->mobs[i].y][g->mobs[i].x] >= H_WAKE) {
			g->mobs[i].awake = 1;
			g->mobs[i].warm = WARM_MOB;
			say(g, "somewhere nearby, something stops being a statue");
		}

	for (i = 0; i < g->nmobs; i++) {
		struct mob *m = &g->mobs[i];
		int best = dist[m->y][m->x], bx = m->x, by = m->y, ties = 0, k2;

		if (!m->awake)
			continue;
		if (m->stagger > 0) {
			/* it is still finding its feet after the shove, which
			 * is the only opening you are ever going to get */
			m->stagger--;
			continue;
		}

		/* what woke them is heat they took in, and it runs out. keep
		 * them near something warm and they keep coming; run and they
		 * go stiff in the dark. a vault you set on fire is a vault
		 * full of them, and that is the trade you are making.
		 *
		 * this has to happen even when one is standing on you, or it
		 * would hold you and feed for ever. */
		m->warm -= (g->heat[m->y][m->x] >= H_FREEZE) ? 1 : 3;
		if (m->warm <= 0) {
			m->warm = 0;
			m->awake = 0;
			/* if it gives out in a doorway it would stand there
			 * for ever and wall the level off, so a narrow place
			 * takes it back instead. rooms keep their statues. */
			if (nopen(g, m->x, m->y) <= 2) {
				say(g, "one of the Wept gives out in the narrow and slumps away");
				g->mobs[i] = g->mobs[g->nmobs - 1];
				g->nmobs--;
				i--;
			} else {
				say(g, "one of the Wept cools and stiffens where it stands");
			}
			continue;
		}

		if (m->x == g->px && m->y == g->py) {
			g->body -= 38 + (int)(rnd_next(&g->rnd) % 12);
			say(g, "the Wept has you, and you are so much warmer");
			continue;
		}
		for (k2 = 0; k2 < 4; k2++) {
			int nx = m->x + dx[k2], ny = m->y + dy[k2], d;

			if (!passable(g, nx, ny))
				continue;
			if (mob_at(g, nx, ny) >= 0)
				continue;
			d = dist[ny][nx];
			if (d < best) {
				best = d;
				bx = nx;
				by = ny;
				ties = 1;
			} else if (d == best && ties && (rnd_next(&g->rnd) & 1)) {
				bx = nx;
				by = ny;
			}
		}
		if (bx == g->px && by == g->py) {
			m->x = bx;
			m->y = by;
			g->body -= 38 + (int)(rnd_next(&g->rnd) % 12);
			say(g, "the Wept has you, and you are so much warmer");
			continue;
		}
		if (bx != m->x || by != m->y) {
			m->x = bx;
			m->y = by;
			if (g->map[by][bx] == T_WATER) {
				drown(g, i);
				i--;
			}
		}

	}

}

/* you only ever get warmer by staying still next to something that
 * is already burning. standing in the flame itself does nothing for
 * you, which stops a fire from being a tap you can drink from. */
static int body_drain(struct game *g)
{
	int i, drain;

	switch (g->map[g->py][g->px]) {
	case T_WATER: drain = 4; break;
	case T_ICE:   drain = 3; break;
	default:      drain = 2; break;
	}
	if (!g->huddling)
		return drain;

	for (i = 0; i < g->nflames; i++)
		if (NABS(g->flames[i].x - g->px) <= 1 && NABS(g->flames[i].y - g->py) <= 1)
			return -6;
	for (i = 0; i < g->nvents; i++)
		if (NABS(g->vents[i].x - g->px) <= 1 && NABS(g->vents[i].y - g->py) <= 1)
			return -1;
	return drain;
}

void sim_turn(struct game *g)
{
	sources(g);
	diffuse(g);
	phases(g);
	mob_turn(g);
	flames_tick(g);

	/* the clamp sits down here on purpose. heat you spent on a brazier
	 * or a relight has to stay spent, so nothing may top you back up. */
	g->body -= body_drain(g);
	if (g->body > BODY_MAX)
		g->body = BODY_MAX;
	g->turn++;

	if (g->body <= 0) {
		g->body = 0;
		g->dead = 1;
		snprintf(g->epitaph, sizeof g->epitaph, "the cold got in on turn %ld", g->turn);
	}
	if (g->body < -60) {
		g->dead = 1;
		snprintf(g->epitaph, sizeof g->epitaph,
			 "one of the Wept carried your warmth off, depth %d", g->depth);
	}
}

static void try_move(struct game *g, int dx, int dy)
{
	int nx = g->px + dx, ny = g->py + dy;
	int mi, shoved = 0;

	if (!passable(g, nx, ny)) {
		if (g->map[ny][nx] == T_RIME && rime_at(g, nx, ny) >= 0)
			say(g, "rime, waist deep in the doorway. it will take heat");
		return;
	}

	mi = mob_at(g, nx, ny);
	if (mi >= 0) {
		int beyond = passable(g, nx + dx, ny + dy) &&
			     mob_at(g, nx + dx, ny + dy) < 0;

		/* they walk as fast as you do, so running is not a plan.
		 * shoving one is: it costs you, it knocks some of the
		 * borrowed warmth back out of it, and it leaves the thing
		 * reeling for a turn. against a wall you trade places with
		 * it instead, which is the whole reason it always works. */
		if (g->mobs[mi].awake) {
			g->body -= 15;
			g->mobs[mi].warm -= 8;
			g->mobs[mi].stagger = 2;
			shoved = 1;
			if (g->mobs[mi].warm <= 0) {
				g->mobs[mi].warm = 0;
				g->mobs[mi].awake = 0;
				say(g, "you shove it hard and the last of its warmth goes");
			} else {
				say(g, "you shove it off you and it costs you dearly");
			}
		} else if (!beyond) {
			say(g, "the frozen figure will not budge");
			return;
		}

		if (beyond) {
			g->mobs[mi].x = nx + dx;
			g->mobs[mi].y = ny + dy;
		} else {
			/* nowhere to send it, so it ends up where you were */
			g->mobs[mi].x = g->px;
			g->mobs[mi].y = g->py;
			say(g, "you put your shoulder into it and get past");
		}
		if (g->map[g->mobs[mi].y][g->mobs[mi].x] == T_WATER)
			drown(g, mi);
		else if (!shoved)
			say(g, "you shoulder the frozen figure along");
	}

	g->px = nx;
	g->py = ny;

	if (g->map[ny][nx] == T_WATER)
		g->body -= 3;
	else if (g->map[ny][nx] == T_ICE)
		g->body -= 1;
}

static void place_brazier(struct game *g)
{
	int t = g->map[g->py][g->px];

	if (g->braziers <= 0) {
		say(g, "no braziers left to set down");
		return;
	}
	if (t != T_FLOOR && t != T_ICE && t != T_WATER && t != T_STAIRS) {
		say(g, "nothing to set it on here");
		return;
	}
	g->braziers--;
	g->body -= 20;
	g->flames[g->nflames].x = g->px;
	g->flames[g->nflames].y = g->py;
	g->flames[g->nflames].life = FLAME_LIFE;
	g->flames[g->nflames].brazier = -1;
	g->nflames++;
	say(g, "you set a brazier down and it catches");
}

/* a brazier you set down is yours to pick back up, which is the only
 * way to move a fire. it costs you a little both ways. */
static void pickup(struct game *g)
{
	int i;

	for (i = 0; i < g->nflames; i++) {
		if (g->flames[i].x != g->px || g->flames[i].y != g->py)
			continue;
		if (g->flames[i].brazier >= 0) {
			say(g, "that one belongs to the vault, it will not come");
			return;
		}
		g->body -= 5;
		g->braziers++;
		
