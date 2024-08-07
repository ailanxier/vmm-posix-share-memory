#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <sys/time.h>
#include <strings.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

typedef unsigned long long uint64;
static struct timeval start_tv, stop_tv;
static const char *OUTPUT_PREFIX = "[test_output] ";
#define MB	(1024. * 1024.)
#define KB	(1024.)

/*
 * Print the output to the specified stream with a custom prefix.
 */
static void test_fprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s", OUTPUT_PREFIX);
    vfprintf(stderr, format, args);
    va_end(args); 
}

/*
 * Make the time spend be usecs.
 */
static void settime(uint64 usecs){
	bzero((void*)&start_tv, sizeof(start_tv));
	stop_tv.tv_sec = usecs / 1000000;
	stop_tv.tv_usec = usecs % 1000000;
}

static void tvsub(struct timeval * tdiff, struct timeval * t1, struct timeval * t0){
	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0 && tdiff->tv_sec > 0) {
		tdiff->tv_sec--;
		tdiff->tv_usec += 1000000;
		assert(tdiff->tv_usec >= 0);
	}

	/* time shouldn't go backwards!!! */
	if (tdiff->tv_usec < 0 || t1->tv_sec < t0->tv_sec) {
		tdiff->tv_sec = 0;
		tdiff->tv_usec = 0;
	}
}

static uint64 tvdelta(struct timeval *start, struct timeval *stop){
	struct timeval td;
	uint64	usecs;

	tvsub(&td, stop, start);
	usecs = td.tv_sec;
	usecs *= 1000000;
	usecs += td.tv_usec;
	return (usecs);
}

/*
 * Start timing now.
 */
static void start(struct timeval *tv){
	if (tv == NULL) 
		tv = &start_tv;
	
	(void) gettimeofday(tv, (struct timezone *) 0);
}

/*
 * Stop timing and return real time in microseconds.
 */
static uint64 stop(struct timeval *begin, struct timeval *end){
	if (end == NULL) 
		end = &stop_tv;
	(void) gettimeofday(end, (struct timezone *) 0);

	if (begin == NULL) 
		begin = &start_tv;

	return (tvdelta(begin, end));
}

static uint64 gettime(void){
	return (tvdelta(&start_tv, &stop_tv));
}

/*
 * Return the time in usecs since the last settime call.
 */
static void calculate_bandwidth(uint64 time, uint64 bytes, uint64 iter){
	double secs = (double)time / 1000000.0;
	double mb;
	
	test_fprintf("Time: %f seconds\n", secs);
    mb = bytes / MB;
	// if (mb < 1.)
	// 	(void) test_fprintf("size: %.6f MB ", mb);
	// else 
	// 	(void) test_fprintf("size: %.2f MB ", mb);
	if (mb / secs < 1.)
		(void) test_fprintf("bandwidth: %.6f MB/s\n", mb/secs);
	else
		(void) test_fprintf("bandwidth: %.2f MB/s\n", mb/secs);
}

#endif