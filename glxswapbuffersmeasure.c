/* gcc -shared -fpic -Os -W -Wall -o glxswapbuffersmeasure.so glxswapbuffersmeasure.c -lGL -ldl -lrt -lm */
/* License: Creative Commons CC0 1.0 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <GL/glx.h>

static void (*next_glXSwapBuffers)(Display *dpy, GLXDrawable drawable);

static __attribute__((__constructor__)) void init(void) {
	next_glXSwapBuffers = dlsym(RTLD_NEXT, "glXSwapBuffers");
	if (!next_glXSwapBuffers)
		abort();
}

static struct timespec last;
static unsigned long count;
static double avg, m2, max_diff;

#define NS_S (1000 * 1000 * 1000)

static void sample(void) {
	struct timespec now, ts_diff;
	double diff, delta;

	if (clock_gettime(CLOCK_MONOTONIC, &now))
		return;

	if (!last.tv_sec && !last.tv_nsec) {
		last = now;
		return;
	}

	ts_diff.tv_sec = now.tv_sec - last.tv_sec;
	ts_diff.tv_nsec = now.tv_nsec - last.tv_nsec;
	if (ts_diff.tv_nsec < 0) {
		ts_diff.tv_sec -= 1;
		ts_diff.tv_nsec += NS_S;
	}
	diff = (double)ts_diff.tv_sec + ((double)ts_diff.tv_nsec / NS_S);
	if (diff > max_diff)
		max_diff = diff;

	/* source: http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#On-line_algorithm */
	count++;
	delta = diff - avg;
	avg += delta / count;
	m2 += delta * (diff - avg);

	last = now;
}

void glXSwapBuffers(Display *dpy, GLXDrawable drawable) {
	next_glXSwapBuffers(dpy, drawable);
	sample();
}

static __attribute__((__destructor__)) void fini(void) {
	double variance = NAN;
	if (count > 1)
		variance = m2 / (count - 1);
	fprintf(stderr, "glXSwapBuffers count: %lu, avg: %f,"
		" variance: %f, std dev: %f, max: %f\n", count, avg,
		variance, sqrt(variance), max_diff);
}
