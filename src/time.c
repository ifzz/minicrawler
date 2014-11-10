#include "h/struct.h"
#include "h/proto.h"

#include <time.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>


/**
 * It seems that cats' OSes cannot provide non-decreasing time. :-(
 * Good job Mr. Jobs.
 */
#ifdef __APPLE__
static long long get_uptime(void)
{
	struct timeval tmval;
	if (gettimeofday(&tmval, NULL) == -1) {
		return 0LL;
	}
	return (long long)(tmval.tv_sec*1000LL + tmval.tv_usec/1000);
}
#else
/**
 * monotonic time since some unspecified starting point
 */
static long long get_uptime(void)
{
	struct timespec tm;
	long long uptime;
	if (clock_gettime(CLOCK_MONOTONIC, &tm) == -1) {
		return 0LL;
	}
	uptime = tm.tv_sec * 1000LL + tm.tv_nsec/1000000;
	if (tm.tv_nsec % 1000000 >= 500000) uptime++; // rounding
	return uptime;
}
#endif

static long long birth;
static void init_birth(void) __attribute__ ((constructor));
static void init_birth(void)
{
	birth = get_uptime();
}

/**
 * vrati pocet milisekund od spusteni programu
 */
int get_time_int(void)
{
	const long long now = get_uptime();
	assert(birth <= now);
	assert(now - birth < INT_MAX);
	return now != birth ? (int)(now - birth) : 1;
}

/**
 * Let's use some good hash function.
 *   Good hashing is important, otherwise we will perform
 *   redundant waiting.
 */
static unsigned hash_uns(const unsigned char key[16])
{
	unsigned hash, k;
	hash = *(int *)key;
	for (int i = 0; i < 16; i += sizeof(unsigned)) {
		k = *(int *)(key + i);
		hash = 13*(k >> 16 | k << 16) ^ 113*(k >> 20 | k << 10) ^ hash;
	}
	return hash;
}

#define HASH_SIZE 64
int hash_table[] = { [0 ... (HASH_SIZE - 1)]=INT_MIN, };

/**
 * Returns slot that is assigned to the supplied key.
 */
unsigned get_time_slot(const unsigned char key[16])
{
	return hash_uns(key) % HASH_SIZE;
}

/**
 * Returns hash item that is assigned to the supplied key.
 */
static int *get_hash_item(const unsigned char key[16])
{
	return &hash_table[get_time_slot(key)];
}

/**
 * Calculates slot that is assigned to the particular ip.
 * If the slot is free, then actual time is assigned to it and the time is returned.
 * Zero is returned otherwise.
 */
int test_free_channel(const unsigned char u_ip[16], const unsigned milis, const int force)
{
	const int now = get_time_int();
	int *slot = get_hash_item(u_ip);
	debugf("%d; %d => %d || %d\n", now, *slot, force, (int)milis <= 0 || *slot + (int)milis <= now);
	if (force || (int)milis <= 0 || *slot + (int)milis <= now) {
		return *slot = now;
	}
        else {
		return 0;
	}
}