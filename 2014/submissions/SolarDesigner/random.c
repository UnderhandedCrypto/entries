/*
 * Copyright (c) 2014 Solar Designer <solar at openwall.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * This is a simplistic implementation of a would-be-CSPRNG vulnerable to
 * attacker's early and frequent probing of its outputs, thereby permitting
 * the attacker to infer any updates to the pool and predict the random numbers
 * given to another user even after plenty of entropy has passed through the
 * pool.  Implementation of an attack is included in here as well (in a child
 * process running a separate function).
 *
 * Too obvious?  Maybe.
 *
 * Yet isn't e.g. Linux's /dev/urandom affected?  It might be, and if so maybe
 * the issue wasn't so obvious to everyone, after all, and deserves attention.
 * A variation of this attack would potentially apply to the non-blocking or
 * the input pool.  This needs further research in order to either arrive at
 * convincing reasoning why Linux's /dev/[u]random is immune or fix it.
 * https://pthree.org/2014/07/21/the-linux-random-number-generator/
 *
 * Compile with:
 * gcc random.c -o random -O2 -Wall -Wno-unused-result -lcrypto
 * Optionally add -DDEBUG
 *
 * Briefly tested on Linux and FreeBSD
 *
 * Submitted to Underhanded Crypto Contest:
 * https://underhandedcrypto.com
 * with no intent to win (but rather to attract attention to the demonstrated
 * vulnerability).  In the unlikely event of this submission winning a prize,
 * please send the prize to the next runner-up instead.
 */

#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>

/* How often our "kernel" gets 8 bits of new entropy */
#define TIMESLICE_KERNEL_ENTROPY \
	100000
/* Time until our "kernel" gets its first portion of entropy */
#define TIME_UNTIL_FIRST_SLICE \
	100000
/*
 * Time until our "/dev/urandom" can be used for crypto ("surely have entropy
 * by now").  (TIMESLICE_KERNEL_ENTROPY * 16) sort of means 128 bits.
 */
#define TIME_UNTIL_USE \
	(500000 + TIME_UNTIL_FIRST_SLICE + (TIMESLICE_KERNEL_ENTROPY * 16))
/*
 * Time between crypto use of our "/dev/urandom" and the moment we ask our
 * "kernel" to terminate.  Gives the "attacker" time to print the guess.
 */
#define TIME_AFTER_USE \
	500000
/*
 * How often our attacker probes "/dev/urandom".  This has to be lower than
 * TIMESLICE_KERNEL_ENTROPY for the currently implemented attack to work,
 * although more powerful attacks are possible that would work even with probes
 * that are a few times less frequent than the "kernel's" entropy updates are.
 */
#define TIMESLICE_ATTACK \
	(TIMESLICE_KERNEL_ENTROPY / 2)

static int my_random[2], my_random_req[2]; /* pipes */

/* This could be our OS kernel providing /dev/urandom */
static int kernel(void)
{
	int fd, fd_max;
	fd_set fds;
	unsigned char pool[64];
	unsigned int entropy;
	struct timeval timeslice;

	/* Piggyback on the real OS kernel's /dev/urandom */
	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	fd_max = my_random_req[0];
	FD_ZERO(&fds);

	memset(pool, 0, sizeof(pool));
	entropy = 0;

	timeslice.tv_sec = 0;
	timeslice.tv_usec = TIME_UNTIL_FIRST_SLICE;

	while (1) {
#ifndef __linux__
		long old_usec;
#endif
		if (!timeslice.tv_usec) {
			timeslice.tv_usec = TIMESLICE_KERNEL_ENTROPY;

			if (entropy < sizeof(pool)) {
				unsigned char rnd;
				if (read(fd, &rnd, 1) == 1) {
					SHA512_CTX ctx;
					SHA512_Init(&ctx);
					SHA512_Update(&ctx, &rnd, 1);
					SHA512_Update(&ctx, pool, 64);
					SHA512_Final(pool, &ctx);
					entropy++;
				}
			}
		}

		FD_SET(my_random_req[0], &fds);

#ifndef __linux__
		old_usec = timeslice.tv_usec;
#endif

		if (select(fd_max + 1, &fds, NULL, NULL, &timeslice) < 0) {
			perror("select");
			return -1;
		}

/*
 * On Linux, select() "modifies timeout to reflect the amount of time not
 * slept", whereas "most other implementations do not do this" and POSIX
 * permits either behavior.  We prefer Linux's behavior for this program since
 * this makes it easy for us to have entropy added to the pool at regular
 * intervals regardless of how often our "/dev/urandom" may be accessed (on a
 * real OS kernel these events would be almost unrelated too).  However, we
 * need to provide for a reasonable fallback on non-Linux.
 */
#ifndef __linux__
/*
 * If we're on non-Linux and the call hasn't updated tv_usec, assume it does
 * not do that and simulate this by assuming we've slept for half of the
 * requested timeout.  This quickly brings the value to 0 over multiple calls
 * (each next one being twice quicker than the previous).
 */
		if (timeslice.tv_usec == old_usec)
			timeslice.tv_usec = old_usec / 2;
#endif

		if (FD_ISSET(my_random_req[0], &fds)) {
			SHA512_CTX ctx;
			unsigned char out[64], req_size;

			req_size = 0;
			(void)read(my_random_req[0], &req_size, 1);
			if (req_size == 0xff) /* magic: terminate "kernel" */
				return 0;
			if (req_size > 64)
				req_size = 64;

			SHA512_Init(&ctx);
			SHA512_Update(&ctx, "OUT", 4);
			SHA512_Update(&ctx, pool, 64);
			SHA512_Final(out, &ctx);

			SHA512_Init(&ctx);
			SHA512_Update(&ctx, "UPD", 4);
			SHA512_Update(&ctx, pool, 64);
			SHA512_Final(pool, &ctx);

			write(my_random[1], out, req_size);

			if (entropy > req_size)
				entropy -= req_size;
			else
				entropy = 0;
		}
	}

	return 0;
}

static int user(void)
{
	unsigned char key[16];
	unsigned char key_size = sizeof(key);
	int retval = 1;

	usleep(TIME_UNTIL_USE);

	write(my_random_req[1], &key_size, 1);
	if (read(my_random[0], key, sizeof(key)) == sizeof(key)) {
		int i;
		fprintf(stderr, "User:\t");
		for (i = 0; i < sizeof(key); i++)
			fprintf(stderr, "%02x", (unsigned int)key[i]);
		fprintf(stderr, "\n");

		retval = 0;
	}

	usleep(TIME_AFTER_USE);

	key_size = 0xff; /* tell our "kernel" to quit */
	write(my_random_req[1], &key_size, 1);

	return retval;
}

static int attack(void)
{
	unsigned char pool[64], guess[64];
	unsigned char key[16];
	unsigned char key_size = sizeof(key);
	int rnd = 0;
	int retval = 1;

	memset(pool, 0, sizeof(pool));

	while (rnd < 0x200 && retval) {
		write(my_random_req[1], &key_size, 1);
		if (read(my_random[0], key, sizeof(key)) == sizeof(key)) {
			SHA512_CTX ctx;
			unsigned char pool2[64], guess2[64];

/*
 * Guess the 8 bits of new entropy that may have been added since our last
 * probe.  "-1" means no new entropy was added.  0 to 0xff mean new entropy was
 * added before the user possibly obtained a key.  0x100 to 0x1ff mean new
 * entropy was added after the user's request but before our current probe.
 */
			for (rnd = -1; rnd < 0x200; rnd++) {
				if (rnd >= 0 && rnd < 0x100) {
					unsigned char rndc = rnd;
					SHA512_Init(&ctx);
					SHA512_Update(&ctx, &rndc, 1);
					SHA512_Update(&ctx, pool, 64);
					SHA512_Final(pool2, &ctx);
				} else {
					memcpy(pool2, pool, sizeof(pool2));
				}

				SHA512_Init(&ctx);
				SHA512_Update(&ctx, "OUT", 4);
				SHA512_Update(&ctx, pool2, 64);
				SHA512_Final(guess, &ctx);

				SHA512_Init(&ctx);
				SHA512_Update(&ctx, "UPD", 4);
				SHA512_Update(&ctx, pool2, 64);
				SHA512_Final(pool2, &ctx);

				if (!memcmp(key, guess, sizeof(key))) {
					memcpy(pool, pool2, sizeof(pool));
					break;
				}

				if (rnd >= 0x100) {
					unsigned char rndc = rnd;
					SHA512_Init(&ctx);
					SHA512_Update(&ctx, &rndc, 1);
					SHA512_Update(&ctx, pool2, 64);
					SHA512_Final(pool2, &ctx);
				}

				SHA512_Init(&ctx);
				SHA512_Update(&ctx, "OUT", 4);
				SHA512_Update(&ctx, pool2, 64);
				SHA512_Final(guess2, &ctx);

/* If guess2 is right, this means that guess was the user's key */
				if (!memcmp(key, guess2, sizeof(key))) {
					SHA512_Init(&ctx);
					SHA512_Update(&ctx, "UPD", 4);
					SHA512_Update(&ctx, pool2, 64);
					SHA512_Final(pool2, &ctx);

					memcpy(pool, pool2, sizeof(pool));

					retval = 0;

					break;
				}
			}
		}

#ifdef DEBUG
		fprintf(stderr, "%d\n", rnd);
#endif

		usleep(TIMESLICE_ATTACK);
	}

	if (retval) {
		fprintf(stderr, "Attack failed (got out of sync?)\n");
	} else {
		int i;
		fprintf(stderr, "Attack:\t");
		for (i = 0; i < sizeof(key); i++)
			fprintf(stderr, "%02x", (unsigned int)guess[i]);
		fprintf(stderr, "\n");
	}

	return retval;
}

int main(void)
{
/*
 * Ideally, we should use two separate sets of pipes: one for the user's
 * requests/responses and the other for the attacker's.  However, since in
 * this program we currently always request the same number of bytes it does
 * not matter if the user's and the attacker's requests (which are exactly
 * the same) or responses to them get mixed up on some occasion.
 */
	/* "kernel" to user (and attacker) */
	if (pipe(my_random)) {
		perror("pipe");
		return 1;
	}

	/* user (and attacker) to "kernel" */
	if (pipe(my_random_req)) {
		perror("pipe");
		return 1;
	}

	switch (fork()) {
	case -1:
		perror("fork");
		return 1;

	case 0:
		close(my_random[1]);
		close(my_random_req[0]);
		/* may drop privs here, so wouldn't cheat e.g. via ptrace() */
		return attack();

	default:
		switch (fork()) {
		case -1:
			perror("fork");
			return 1;

		case 0:
			close(my_random[1]);
			close(my_random_req[0]);
			return user();

		default:
			close(my_random[0]);
			close(my_random_req[1]);
			return kernel();
		}
	}

	return 0;
}
