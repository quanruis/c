/* gen.c -- the vault is a set of cold rooms joined by cold corridors,
 * and every level has exactly one thing between you and the stair:
 * a plug of rime you cannot walk through and cannot break. you have
 * to melt it, which means building a little fire and standing in it.
 *
 * the generator guarantees the plug is a real choke point, so there is
 * always a route, and there is always a cold brazier within reach of
 * the plug so a level is never unwinnable because you spent badly.
 */

#include <string.h>
#include "sim.h"

#define MAXROOM 12
#define NABS(x) ((x) < 0 ? -(x) : (x))

/* where you can put your feet and set a brazier down */
static int standable(uint8_t t)
{
	return t == T_FLOOR || t == T_ICE || t == T_WATER;
}

struct rect {
	int x, y, w, h;
};

static int dist_map(const struct game *g, int sx, int sy, int *out)
{
	static const int dx[4] = { 1, -1, 0, 0 };
	static const int dy[4] = { 0, 0, 1, -1 };
	int qx[MAPH * MAPW], qy[MAPH * MAPW];
	int head = 0, tail = 0, i, maxd = 0;

	for (i = 0; i < MAPH * MAPW; i++)
		out[i] = -1;
	out[sy * MAPW + sx] = 0;
	qx[tail] = sx;
	qy[tail] = sy;
	tail++;

	while (head < tail) {
		int x = qx[head], y = qy[head], d = out[y * MAPW + x];
		head++;
		if (d > maxd)
			maxd = d;
		for (i = 0; i < 4; i++) {
			int nx = x + dx[i], ny = y + dy[i];

			if (nx < 0 || ny < 0 || nx >= MAPW || ny >= MAPH)
				continue;
			if (!passable(g, nx, ny))
				continue;
			if (out[ny * MAPW + nx] >= 0)
				continue;
			out[ny * MAPW + nx] = d + 1;
			qx[tail] = nx;
			qy[tail] = ny;
			tail++;
		}
	}
	return maxd;
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

static int add_rime(struct game *g, int x, int y, int units)
{
	if (g->nrimes >= (int)(sizeof g->rimes / sizeof g->rimes[0]))
		return 0;
	g->rimes[g->nrimes].x = x;
	g->rimes[g->nrimes].y = y;
	g->rimes[g->nrimes].units = units;
	g->rimes[g->nrimes].sides = 15;
	g->nrimes++;
	g->map[y][x] = T_RIME;
	return 1;
}

static void rooms(struct game *g, struct rect *r, int *nr, int want)
{
	int tries = 0, i;

	*nr = 0;
	while (*nr < want && tries < 400) {
		struct rect c;

		tries++;
		c.w = 3 + (int)(rnd_next(&g->rnd) % 6);
		c.h = 3 + (int)(rnd_next(&g->rnd) % 4);
		c.x = 1 + (int)(rnd_next(&g->rnd) % (uint32_t)(MAPW - c.w - 2));
		c.y = 1 + (int)(rnd_next(&g->rnd) % (uint32_t)(MAPH - c.h - 2));

		for (i = 0; i < *nr; i++) {
			const struct rect *o = &r[i];

			if (c.x - 1 < o->x + o->w && o->x - 1 < c.x + c.w &&
			    c.y - 1 < o->y + o->h && o->y - 1 < c.y + c.h)
				break;
		}
		if (i < *nr)
			continue;
		r[(*nr)++] = c;
	}
}

static void carve_room(struct game *g, const struct rect *c)
{
	int x, y;

	for (y = c->y; y < c->y + c->h; y++)
		for (x = c->x; x < c->x + c->w; x++)
			g->map[y][x] = T_FLOOR;
}

/* l shaped, and the corner is not always in the same place, which is
 * the only reason the corridors don't all look like the same corridor */
static void carve_l(struct game *g, int ax, int ay, int bx, int by)
{
	int x, y;

	if (rnd_next(&g->rnd) & 1) {
		for (x = ax; x != bx; x += (bx > ax ? 1 : -1))
			g->map[ay][x] = T_FLOOR;
		for (y = ay; y != by; y += (by > ay ? 1 : -1))
			g->map[y][bx] = T_FLOOR;
	} else {
		for (y = ay; y != by; y += (by > ay ? 1 : -1))
			g->map[y][ax] = T_FLOOR;
		for (x = ax; x != bx; x += (bx > ax ? 1 : -1))
			g->map[by][x] = T_FLOOR;
	}
	g->map[by][bx] = T_FLOOR;
}

static void connect(struct game *g, struct rect *r, int nr)
{
	int intree[MAXROOM], i, n;

	if (nr < 2)
		return;
	memset(intree, 0, sizeof intree);
	intree[0] = 1;

	for (n = 1; n < nr; n++) {
		int best = -1, bi = 0, bj = 0, bd = 1 << 30;

		for (i = 0; i < nr; i++) {
			int j;

			if (!intree[i])
				continue;
			for (j = 0; j < nr; j++) {
				int d;

				if (intree[j])
					continue;
				d = (r[i].x + r[i].w / 2 - r[j].x - r[j].w / 2) *
				    (r[i].x + r[i].w / 2 - r[j].x - r[j].w / 2) +
				    (r[i].y + r[i].h / 2 - r[j].y - r[j].h / 2) *
				    (r[i].y + r[i].h / 2 - r[j].y - r[j].h / 2);
				if (d < bd) {
					bd = d;
					bi = i;
					bj = j;
					best = j;
				}
			}
		}
		if (best < 0)
			break;
		intree[best] = 1;
		carve_l(g, r[bi].x + r[bi].w / 2, r[bi].y + r[bi].h / 2,
			r[bj].x + r[bj].w / 2, r[bj].y + r[bj].h / 2);
	}

	for (i = 0; i < 2 && nr > 3; i++) {
		int a = (int)(rnd_next(&g->rnd) % (uint32_t)nr);
		int b = (int)(rnd_next(&g->rnd) % (uint32_t)nr);

		if (a == b)
			continue;
		carve_l(g, r[a].x + r[a].w / 2, r[a].y + r[a].h / 2,
			r[b].x + r[b].w / 2, r[b].y + r[b].h / 2);
	}
}

/* find a cell that sits between the player and the stair and nothing
 * else. we walk back along the shortest route from the stair and try
 * each cell in turn until removing it actually cuts the route. */
static int pick_seal(struct game *g, int *sx, int *sy)
{
	static int d[MAPH * MAPW];
	static int path[MAPH * MAPW][2];
	static const int dx[4] = { 1, -1, 0, 0 };
	static const int dy[4] = { 0, 0, 1, -1 };
	int plen = 0, tries;

	dist_map(g, g->exitx, g->exity, d);
	{
		int x = g->px, y = g->py;

		while (d[y * MAPW + x] > 0) {
			int k, found = 0;

			path[plen][0] = x;
			path[plen][1] = y;
			plen++;
			for (k = 0; k < 4; k++) {
				int nx = x + dx[k], ny = y + dy[k];

				if (nx < 0 || ny < 0 || nx >= MAPW || ny >= MAPH)
					continue;
				if (d[ny * MAPW + nx] == d[y * MAPW + x] - 1) {
					x = nx;
					y = ny;
					found = 1;
					break;
				}
			}
			if (!found)
				break;
		}
	}
	if (plen < 4)
		return 0;

	for (tries = 0; tries < 90; tries++) {
		int idx = 2 + (int)(rnd_next(&g->rnd) % (uint32_t)(plen - 3));
		int cx = path[idx][0], cy = path[idx][1];
		uint8_t save = g->map[cy][cx];
		int cut;

		if (cx == g->px && cy == g->py)
			continue;
		g->map[cy][cx] = T_WALL;
		dist_map(g, g->px, g->py, d);
		cut = (d[g->exity * MAPW + g->exitx] < 0);
		g->map[cy][cx] = save;

		if (!cut)
			continue;
		*sx = cx;
		*sy = cy;
		return 1;
	}
	return 0;
}

static int free_floor(const struct game *g, int *out)
{
	int n = 0, y, x;

	for (y = 1; y < MAPH - 1; y++)
		for (x = 1; x < MAPW - 1; x++)
			if (g->map[y][x] == T_FLOOR)
				out[n++] = y * MAPW + x;
	return n;
}

static int spot(struct game *g, int minfrom, int *x, int *y)
{
	static int cells[MAPH * MAPW];
	int n, i;

	n = free_floor(g, cells);
	for (i = 0; i < 60 && n; i++) {
		int k = (int)(rnd_next(&g->rnd) % (uint32_t)n);
		int cx = cells[k] % MAPW, cy = cells[k] / MAPW;
		int dx = cx - g->px, dy = cy - g->py;

		if (NABS(dx) + NABS(dy) < minfrom)
			continue;
		if (cx == g->exitx && cy == g->exity)
			continue;
		*x = cx;
		*y = cy;
		return 1;
	}
	return 0;
}

static void scatter(struct game *g, int depth, int sealxx, int sealyy)
{
	static int d[MAPH * MAPW];
	int i, x, y, n, pools;

	/* meltwater that has already gone over */
	pools = 2 + (int)(rnd_next(&g->rnd) % 3);
	for (i = 0; i < pools; i++) {
		int len, s;

		if (!spot(g, 3, &x, &y))
			continue;
		len = 3 + (int)(rnd_next(&g->rnd) % 6);
		for (s = 0; s < len; s++) {
			if (g->map[y][x] == T_FLOOR) {
				g->map[y][x] = T_WATER;
				g->heat[y][x] = H_FREEZE + 6;
			}
			switch (rnd_next(&g->rnd) & 3) {
			case 0: if (x < MAPW - 2) x++; break;
			case 1: if (x > 1) x--; break;
			case 2: if (y < MAPH - 2) y++; break;
			default: if (y > 1) y--; break;
			}
		}
	}

	/* frost that has crept in off the walls */
	for (i = 0; i < 8 + depth; i++) {
		int s, len;

		if (!spot(g, 2, &x, &y))
			continue;
		len = 2 + (int)(rnd_next(&g->rnd) % 4);
		for (s = 0; s < len; s++) {
			if (g->map[y][x] == T_FLOOR) {
				g->map[y][x] = T_ICE;
				g->heat[y][x] = H_COLD + 4;
			}
			switch (rnd_next(&g->rnd) & 3) {
			case 0: if (x < MAPW - 2) x++; break;
			case 1: if (x > 1) x--; break;
			case 2: if (y < MAPH - 2) y++; break;
			default: if (y > 1) y--; break;
			}
		}
	}

	/* the Wept, standing where they fell. only ever in a room and
	 * never in a doorway: a frozen one you cannot push anywhere is a
	 * door that does not open, and there is no digging in this game. */
	n = 3 + depth;
	if (n > 8)
		n = 8;
	for (i = 0; i < n * 4 && g->nmobs < n; i++) {
		int cut;
		uint8_t savedseal = g->map[sealyy][sealxx];

		if (!spot(g, 7, &x, &y))
			continue;
		if (g->map[y][x] != T_FLOOR)
			continue;
		if (nopen(g, x, y) < 3)
			continue;

		/* a frozen one is a thing you have to walk around or shove.
		 * if there is no around, do not put one there. the plug is
		 * open for the duration, it is going to be melted anyway. */
		g->map[sealyy][sealxx] = T_FLOOR;
		g->map[y][x] = T_WALL;
		dist_map(g, g->px, g->py, d);
		cut = (d[g->exity * MAPW + g->exitx] < 0);
		g->map[y][x] = T_FLOOR;
		g->map[sealyy][sealxx] = savedseal;
		if (cut)
			continue;

		g->mobs[g->nmobs].x = x;
		g->mobs[g->nmobs].y = y;
		g->mobs[g->nmobs].awake = 0;
		g->mobs[g->nmobs].warm = 0;
		g->mobs[g->nmobs].stagger = 0;
		g->nmobs++;
	}

	/* cold braziers. one of them always ends up near the plug. */
	for (i = 0; i < 3; i++) {
		if (!spot(g, 4, &x, &y))
			continue;
		if (g->nbraz >= (int)(sizeof g->braz / sizeof g->braz[0]))
			break;
		g->braz[g->nbraz].x = x;
		g->braz[g->nbraz].y = y;
		g->braz[g->nbraz].lit = 0;
		g->nbraz++;
	}

	/* something down there is still breathing */
	n = 1 + (int)(rnd_next(&g->rnd) % 2);
	for (i = 0; i < n; i++) {
		if (!spot(g, 5, &x, &y))
			continue;
		if (g->nvents >= (int)(sizeof g->vents / sizeof g->vents[0]))
			break;
		g->vents[g->nvents].x = x;
		g->vents[g->nvents].y = y;
		g->nvents++;
	}
}

/* rime that is only in the way, not a puzzle. it always melts and it
 * is never load bearing, it's just there so the vault reads as frozen */
static void decorate(struct game *g, int sealx, int sealy)
{
	static int d[MAPH * MAPW];
	static int cells[MAPH * MAPW];
	int i, n, want = 3 + (int)(rnd_next(&g->rnd) % 3);

	dist_map(g, g->px, g->py, d);
	n = free_floor(g, cells);

	for (i = 0; i < 200 && want > 0; i++) {
		int k = (int)(rnd_next(&g->rnd) % (uint32_t)n);
		int x = cells[k] % MAPW, y = cells[k] / MAPW;
		int dd = d[y * MAPW + x];

		if (g->map[y][x] != T_FLOOR)
			continue;
		if (dd < 0 || dd < 3)
			continue;
		if (x == g->exitx && y == g->exity)
			continue;
		if (NABS(x - sealx) + NABS(y - sealy) < 2)
			continue;
		if (!add_rime(g, x, y, RIME_UNITS))
			break;
		want--;
	}
}

static void brazier_by_seal(struct game *g, int sx, int sy)
{
	static const int dx[4] = { 1, -1, 0, 0 };
	static const int dy[4] = { 0, 0, 1, -1 };
	int i;

	for (i = 0; i < 4; i++) {
		int nx = sx + dx[i], ny = sy + dy[i];

		if (nx < 0 || ny < 0 || nx >= MAPW || ny >= MAPH)
			continue;
		if (!standable(g->map[ny][nx]))
			continue;
		if (g->nbraz >= (int)(sizeof g->braz / sizeof g->braz[0]))
			return;
		g->braz[g->nbraz].x = nx;
		g->braz[g->nbraz].y = ny;
		g->braz[g->nbraz].lit = 0;
		g->nbraz++;
		return;
	}
}

/* the rime you decorate a level with can land in a corridor and wall
 * off the plug you actually need, so check the whole level is still
 * solvable before anyone has to play it: at every point, at least one
 * plug has to be reachable and meltable. */
static int still_solvable(struct game *g)
{
	static int d[MAPH * MAPW];
	static const int dx[4] = { 1, -1, 0, 0 };
	static const int dy[4] = { 0, 0, 1, -1 };
	int guard = 0;

	while (guard++ < 32) {
		int i, k, any = 0;

		dist_map(g, g->px, g->py, d);
		if (d[g->exity * MAPW + g->exitx] >= 0)
			return 1;

		for (i = 0; i < g->nrimes && !any; i++) {
			for (k = 0; k < 4; k++) {
				int nx = g->rimes[i].x + dx[k];
				int ny = g->rimes[i].y + dy[k];

				if (nx < 1 || ny < 1 || nx >= MAPW - 1 || ny >= MAPH - 1)
					continue;
				if (!standable(g->map[ny][nx]))
					continue;
				if (d[ny * MAPW + nx] < 0)
					continue;
				any = 1;
				break;
			}
		}
		if (!any)
			return 0;

		/* pretend we burned the nearest one and look again */
		{
			int bi = -1, bd = -1;

			for (i = 0; i < g->nrimes; i++) {
				int x = g->rimes[i].x, y = g->rimes[i].y;

				for (k = 0; k < 4; k++) {
					int nx = x + dx[k], ny = y + dy[k], dd;

					if (nx < 1 || ny < 1 || nx >= MAPW - 1 || ny >= MAPH - 1)
						continue;
					if (!standable(g->map[ny][nx]))
						continue;
					dd = d[ny * MAPW + nx];
					if (dd < 0)
						continue;
					if (bd < 0 || dd < bd) {
						bd = dd;
						bi = i;
					}
				}
			}
			if (bi < 0)
				return 0;
			g->map[g->rimes[bi].y][g->rimes[bi].x] = T_WATER;
			g->rimes[bi] = g->rimes[g->nrimes - 1];
			g->nrimes--;
		}
	}
	return 0;
}

static int build(struct game *g, int depth, uint32_t seed)
{
	struct rect r[MAXROOM];
	static int d[MAPH * MAPW];
	static uint8_t savedmap[MAPH][MAPW];
	static struct rime savedrime[16];
	int nr = 0, i, x, y, want, sealx = -1, sealy = -1;
	int ok;

	memset(g->map, T_WALL, sizeof g->map);
	for (y = 0; y < MAPH; y++)
		for (x = 0; x < MAPW; x++)
			g->heat[y][x] = H_COLD;

	g->rnd = seed ? seed : 0x12345678u;
	g->nmobs = 0;
	g->nflames = 0;
	g->nbraz = 0;
	g->nvents = 0;
	g->nrimes = 0;
	g->nmsg = 0;
	g->depth = depth;
	g->huddling = 0;

	want = 5 + depth / 2;
	if (want > 9)
		want = 9;
	rooms(g, r, &nr, want);
	if (nr < 3)
		return 0;
	for (i = 0; i < nr; i++)
		carve_room(g, &r[i]);
	connect(g, r, nr);

	/* the stair goes as far from the middle as the vault allows */
	dist_map(g, r[0].x + r[0].w / 2, r[0].y + r[0].h / 2, d);
	{
		int best = -1;

		g->exitx = g->exity = -1;
		for (y = 1; y < MAPH - 1; y++)
			for (x = 1; x < MAPW - 1; x++)
				if (g->map[y][x] == T_FLOOR && d[y * MAPW + x] > best) {
					best = d[y * MAPW + x];
					g->exitx = x;
					g->exity = y;
				}
	}
	if (g->exitx < 0)
		return 0;

	g->map[g->exity][g->exitx] = T_STAIRS;

	/* and you start at the other end of it */
	dist_map(g, g->exitx, g->exity, d);
	{
		int best = -1;

		g->px = g->py = -1;
		for (y = 1; y < MAPH - 1; y++)
			for (x = 1; x < MAPW - 1; x++)
				if (g->map[y][x] == T_FLOOR && d[y * MAPW + x] > best) {
					best = d[y * MAPW + x];
					g->px = x;
					g->py = y;
				}
	}
	if (g->px < 0 || (g->px == g->exitx && g->py == g->exity))
		return 0;

	if (!pick_seal(g, &sealx, &sealy))
		return 0;
	if (!add_rime(g, sealx, sealy, RIME_UNITS))
		return 0;
	g->sealed = 1;
	g->sealx = sealx;
	g->sealy = sealy;
	brazier_by_seal(g, sealx, sealy);

	scatter(g, depth, sealx, sealy);
	decorate(g, sealx, sealy);

	/* a level with nothing to see is a level with nothing to do */
	if (g->nmobs < 2 || g->nbraz < 2)
		return 0;

	/* the solvability hunt melts plugs as it goes, so work on a copy
	 * and put the level back the way it was afterwards */
	memcpy(savedmap, g->map, sizeof savedmap);
	memcpy(savedrime, g->rimes, sizeof savedrime);
	i = g->nrimes;
	ok = still_solvable(g);
	memcpy(g->map, savedmap, sizeof g->map);
	memcpy(g->rimes, savedrime, sizeof g->rimes);
	g->nrimes = i;
	if (!ok)
		return 0;

	for (i = 0; i < g->nmobs; i++)
		g->mobs[i].awake = 0;

	return 1;
}

int gen_level(struct game *g, int depth, uint32_t seed)
{
	int keepbody = g->body, keepbraz = g->braziers;
	long keepturn = g->turn;
	int tries;

	if (keepbody <= 0) {
		keepbody = BODY_START;
		keepbraz = 4;
		keepturn = 0;
	}
	if (keepbraz <= 0)
		keepbraz = 1;

	for (tries = 0; tries < 40; tries++) {
		if (build(g, depth, seed + (uint32_t)tries * 7919u))
			break;
	}
	if (tries >= 40)
		return 0;

	g->body = keepbody;
	g->braziers = keepbraz;
	g->turn = keepturn;
	g->dead = 0;
	g->over = 0;
	g->epitaph[0] = 0;
	g->nmsg = 0;
	return 1;
}
