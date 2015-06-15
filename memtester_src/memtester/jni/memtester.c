/*
 * memtester version 5.0.0
 *
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 */

#define __version__ "5.0.0"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "types.h"
#include "tests.h"
#include "get_phy_addr.h"

#define EXIT_FAIL_NONSTARTER    0x01
#define EXIT_FAIL_ADDRESSLINES  0x02
#define EXIT_FAIL_OTHERTEST     0x04

static struct test tests[] = {
		{ "pattern", NULL, test_using_pattern },
		{ "stuckaddr", NULL, test_stuck_address },
		{ "random", test_random_value, NULL },
		{ "xor", test_xor_comparison, NULL },
		{ "sub", test_sub_comparison, NULL },
		{ "mul", test_mul_comparison, NULL },
		{ "div", test_div_comparison, NULL },
		{ "or", test_or_comparison, NULL },
		{ "and", test_and_comparison, NULL },
		{ "seqinc", test_seqinc_comparison, NULL },
		{ "solidbits", test_solidbits_comparison, NULL },
		{ "blockseq", test_blockseq_comparison, NULL },
		{ "checkerboard", test_checkerboard_comparison, NULL },
		{ "bitspread", test_bitspread_comparison, NULL },
		{ "bitflip", test_bitflip_comparison, NULL },
		{ "fast_bitflip", test_fast_bitflip_comparison, NULL },
		{ "walkbits1", test_walkbits1_comparison, NULL },
		{ "walkbits0", test_walkbits0_comparison, NULL },
		{ "write_only_checkerboard", test_write_only_checkerboard, NULL },
		{ "nop", test_nop_comparison, NULL },
		{ "move", test_move_comparison, NULL },
		{ "bitchk", test_bitflip_check, NULL },
#ifdef TEST_NARROW_WRITES    
		{ "8bit", test_8bit_wide_random, NULL },
		{ "16bit", test_16bit_wide_random, NULL },
#endif
		{ NULL, NULL }
};

/* Sanity checks and portability helper macros. */
#ifdef _SC_VERSION
static void check_posix_system(void) {
	if (sysconf(_SC_VERSION) < 198808L) {
		fprintf(stderr, "A POSIX system is required.  Don't be surprised if "
				"this craps out.\n");
		fprintf(stderr, "_SC_VERSION is %d\n", sysconf(_SC_VERSION));
	}
}
#else
#define check_posix_system()
#endif

#ifdef _SC_PAGE_SIZE
int memtester_pagesize(void) {
	static int print_info = 1;

	int pagesize = sysconf(_SC_PAGE_SIZE);
	if (pagesize == -1) {
		perror("get page size failed");
		exit(EXIT_FAIL_NONSTARTER);
	}

	if (print_info) {
		printf("pagesize is %ld\n", (long) pagesize);
		print_info = 0;
	}

	return pagesize;
}
#else
int memtester_pagesize(void) {
	printf("sysconf(_SC_PAGE_SIZE) not supported; using pagesize of 8192\n");
	return 8192;
}
#endif

/* Some systems don't define MAP_LOCKED.  Define it to 0 here
 so it's just a no-op when ORed with other constants. */
#ifndef MAP_LOCKED
#define MAP_LOCKED 0
#endif

/* Global vars - so tests have access to this information */
int use_phys = 0;
off_t physaddrbase = 0;
ul *test_patterns = NULL;
size_t pattern_count = 0;
char *command = NULL;
size_t read_count = 1;

static int keep_going = 1;



/* Function definitions */
static void usage(char *me) {
	fprintf(stderr, "\n"
			"for detailed description, use \"%s -h\".\n"
			"Usage: \n"
			"    %s [-p physaddrbase [-d device] -t testlist -k -i patternfile -c command] <mem>[B|K|M|G] [loops]\n"
			"    %s -h\n",
			me, me, me);
	exit(EXIT_FAIL_NONSTARTER);
}

static void usage_detail(char *me) {
	int i;

	printf("\n"
			"Usage: \n"
			"    %s [-p physaddrbase [-d device] -t testlist -k -i patternfile -c command] mem[B|K|M|G] [loops]\n"
			"    %s -h\n"
			"        -p : use physical address. should be 4k aligned value. (e.g 0x12345000)\n"
			"        -d : device location.\n"
			"        -t : select tests. run test by selected order.\n"
			"             for available test list, see \"%s -h\". (e.g -t pattern,xor,pattern)\n"
			"        -k : do not stop on failure. (keep going) (e.g -k 1)\n"
			"             1: stop test on failure. (default)\n"
			"             2: do not stop on failure.\n"
			"             3: stop and kernel crash.\n"
			"             4: stop and another crash.\n"
			"        -a : another crash command.\n"
			"             default is kernel crash. (e.g echo c > /proc/sysrq-trigger)\n"
			"        -i : use test pattern. (e.g -i ./test_pattern.txt)\n"
			"             pattern file should conatin one hex string per line. (e.g 0x12345678)\n"
			"        -c : run command before read. (e.g -c \"ps | grep 123\")\n"
			"        -r : read count after write. (e.g -r 5). default is 1.\n"
			"        mem : tested mem size. (e.g 4M)\n"
			"        loop : loop count. if omitted, run infinitely.\n",
			me, me, me);

	printf("available test list : \n");
	for (i = 0;; i++) {
		if (!tests[i].name)
			break;
		else
			printf("    %s\n", tests[i].name);
	}
	exit(0);
}

static size_t get_test_count(const char *test_list_str) {
	char *temp_test_name;
	char test_name[128];
	int i;
	size_t test_count = 0;
	char *temp_test_list_str = (char *)malloc(strlen(test_list_str)+1);
	memcpy(temp_test_list_str, test_list_str, (size_t)(strlen(test_list_str)+1));

	temp_test_name = strtok(temp_test_list_str, ",");
	while(temp_test_name != NULL) {
		sscanf(temp_test_name, "%s", test_name);

		for (i = 0;; i++) {
			if (!tests[i].name)
				break;

			if (strcmp(tests[i].name, test_name) == 0) {
				test_count++;
				break;
			}
		}

		temp_test_name = strtok(NULL, ",");
	}

	free((void *)temp_test_list_str);

	if(test_count == 0) {
		fprintf(stderr, "available test is not exist......\n");
	}

	return test_count;
}

static char ** create_test_list(const char *test_list_str, size_t test_count) {
	if(test_count == 0) {
		fprintf(stderr, "available test is not exist......\n");
		return NULL;
	}

	char **test_list = (char **)malloc((test_count+1) * sizeof(char *));

	char *temp_test_name;
	char test_name[128];
	size_t temp_test_count = 0;
	int i;
	char *temp_test_list_str = (char *)malloc(strlen(test_list_str)+1);
	memcpy(temp_test_list_str, test_list_str, (size_t)(strlen(test_list_str)+1));

	temp_test_name = strtok(temp_test_list_str, ",");
	while(temp_test_name != NULL) {
		if (temp_test_count == test_count)
			break;

		sscanf(temp_test_name, "%s", test_name);
		for (i = 0;; i++) {
			if (!tests[i].name)
				break;

			if (strcmp(tests[i].name, test_name) == 0) {
				test_list[temp_test_count] = (char *)malloc(strlen(test_name)+1);
				memcpy(test_list[temp_test_count], test_name, (size_t)(strlen(test_name)+1));
				temp_test_count++;
				break;
			}
		}
		temp_test_name = strtok(NULL, ",");
	}

	test_list[test_count] = NULL;

	free((void *)temp_test_list_str);

	return test_list;
}

static void print_test_list(char **test_list) {
	int i = 0;

	printf("using following tests : \n");
	while(test_list[i] != NULL) {
		printf("    %s\n", test_list[i]);
		i++;
	}
}

static void delete_test_list(char **test_list) {
	int i = 0;

	if (test_list == NULL)
		return;

	while(test_list[i] != NULL) {
		free((void *)test_list[i]);
		i++;
	}

	free((void *)test_list);
}

static int check_pattern_file(const char *filename) {
	struct stat statbuf;

	if (stat(filename, &statbuf)) {
		fprintf(stderr, "can not use %s : %s\n", filename, strerror(errno));
		return -1;
	} else {
		if (S_ISREG(statbuf.st_mode)) {
			return 0;
		} else {
			fprintf(stderr, "%s is not regular file\n", filename);
			return -2;
		}
	}
}

static size_t set_pattern_count(const char *filename) {
	char buffer[1024];
	size_t count = 0;
	FILE *patterns_file = fopen(filename, "r");

	if (patterns_file == NULL) {
		fprintf(stderr, "error during open file......\n");
		return 0;
	}

	while (fgets(buffer, sizeof(buffer), patterns_file) != NULL) {
		// only count line started with "0x"
		if (strncmp(buffer, "0x", 2) == 0)
			count++;
	}

	fclose(patterns_file);

	if (count == 0) {
		fprintf(stderr, "pattern does not exist. should be hex address (0x123...)\n");
	}

	pattern_count = count;

	return count;
}

static int set_test_patterns(const char *filename) {
	char buffer[1024];
	int i = 0;
	ul pattern;
	FILE *patterns_file;

	if (pattern_count == 0) {
		fprintf(stderr, "pattern does not exist. should be hex address (0x123...)\n");
		return -1;
	}

	patterns_file = fopen(filename, "r");

	if (patterns_file == NULL) {
		fprintf(stderr, "error during open file......\n");
		return -2;
	}

	test_patterns = (ul *)malloc(pattern_count * sizeof(ul));

	while (fgets(buffer, sizeof(buffer), patterns_file) != NULL) {
		if (i == pattern_count)
			break;

		// only parse line started with "0x"
		if (strncmp(buffer, "0x", 2) != 0)
			continue;

		errno = 0;
		pattern = (ul)strtoull(buffer, NULL, 16);

		if (errno != 0) {
			fprintf(stderr,
					"failed to parse pattern. should be hex address (0x123...)\n");
			free((void *)test_patterns);
			test_patterns = NULL;
			fclose(patterns_file);
			return -4;
		}

		test_patterns[i] = pattern;
		i++;
	}

	fclose(patterns_file);

	return 0;
}

static void print_test_patterns() {
	int i = 0;
	printf("using following test patterns : \n");
	for(i=0; i<pattern_count; i++) {
		printf("    0x%08lX\n", test_patterns[i]);
	}
}

static int run_test(
		ul test_index,
		void volatile *aligned, size_t bufsize,
		ulv *bufa, ulv *bufb, size_t count) {

	printf("  %-24s: ", tests[test_index].name);
	fflush(stdout);

	if (tests[test_index].fp_all != NULL) {
		if (!tests[test_index].fp_all(aligned, (bufsize / sizeof(ul))))
			printf("ok\n");
		else
			return -1;

	} else { // tests[i].fp_half != NULL
		if (!tests[test_index].fp_half(bufa, bufb, count))
			printf("ok\n");
		else
			return -1;;
	}

	return 0;
}

int main(int argc, char **argv) {
	ul loops, loop, i, j;
	size_t pagesize, wantraw, wantmb, wantbytes, wantbytes_orig, bufsize,
			halflen, count;
	char *memsuffix, *addrsuffix, *loopsuffix;
	ptrdiff_t pagesizemask;
	void volatile *buf, *aligned;
	ulv *bufa, *bufb;
	int do_mlock = 1, done_mem = 0;
	int exit_code = 0;
	int memfd, opt, memshift;
	size_t maxbytes = -1; /* addressable memory, in bytes */
	size_t maxmb = (maxbytes >> 20) + 1; /* addressable memory, in MB */
	/* Device to mmap memory from with -p, default is normal core */
	char *device_name = "/dev/mem";
	struct stat statbuf;
	int device_specified = 0;
	char *test_list_str = "all";
	int test_count = 0;
	char **test_list = NULL;
	char *test_pattern_file = NULL;
	int is_continue = 1;
	char *crash_command = NULL;

	printf("memtester version " __version__ " (%d-bit)\n", UL_LEN);
	printf("Copyright (C) 2001-2012 Charles Cazabon.\n");
	printf("Licensed under the GNU General Public License version 2 (only).\n");
	printf("\n");
	check_posix_system();
	pagesize = memtester_pagesize();
	pagesizemask = (ptrdiff_t) ~(pagesize - 1);
	printf("pagesizemask is 0x%tx\n", pagesizemask);

	while ((opt = getopt(argc, argv, "p:d:t:k:i:c:r:a:h")) != -1) {
		switch (opt) {
		case 'p': //  Physical address
			errno = 0;
			physaddrbase = (off_t) strtoull(optarg, &addrsuffix, 16);
			if (errno != 0) {
				fprintf(stderr,
						"failed to parse physaddrbase arg; should be hex "
								"address (0x123...)\n");
				usage(argv[0]); /* doesn't return */
			}
			if (*addrsuffix != '\0') {
				/* got an invalid character in the address */
				fprintf(stderr,
						"failed to parse physaddrbase arg; should be hex "
								"address (0x123...)\n");
				usage(argv[0]); /* doesn't return */
			}
			if (physaddrbase & (pagesize - 1)) {
				fprintf(stderr, "bad physaddrbase arg; does not start on page "
						"boundary\n");
				usage(argv[0]); /* doesn't return */
			}
			/* okay, got address */
			use_phys = 1;
			break;
		case 'd':
			if (stat(optarg, &statbuf)) {
				fprintf(stderr, "can not use %s as device: %s\n", optarg,
						strerror(errno));
				usage(argv[0]); /* doesn't return */
			} else {
				if (!S_ISCHR(statbuf.st_mode)) {
					fprintf(stderr, "can not mmap non-char device %s\n",
							optarg);
					usage(argv[0]); /* doesn't return */
				} else {
					device_name = optarg;
					device_specified = 1;
				}
			}
			break;
		case 'h':
			usage_detail(argv[0]);
			break;
		case 't':
			test_list_str = optarg;
			break;
		case 'k':
			keep_going = atoi(optarg);
			if ((keep_going < 1) || (keep_going > 4)) {
				fprintf(stderr, "invalid -k argument: %s (should be 1 ~ 4)\n", optarg);
				usage(argv[0]);
			}
			printf("keep goning mode is %d\n", keep_going);
			break;
		case 'r':
			read_count = atoi(optarg);
			if ((keep_going < 1)) {
				fprintf(stderr, "invalid -r argument: %s (should be greater than 0)\n", optarg);
				usage(argv[0]);
			}
			printf("read count is %d\n", read_count);
			break;
		case 'i':
			if (check_pattern_file(optarg) < 0)
				usage(argv[0]);

			test_pattern_file = optarg;
			break;
		case 'c':
			command = optarg;
			break;
		case 'a':
			crash_command = optarg;
			break;
		default: /* '?' */
			usage(argv[0]); /* doesn't return */
		}
	}

	if (device_specified && !use_phys) {
		fprintf(stderr,
				"for mem device, physaddrbase (-p) must be specified\n");
		usage(argv[0]); /* doesn't return */
	}

	if (optind >= argc) {
		fprintf(stderr, "need memory argument, in MB\n");
		usage(argv[0]); /* doesn't return */
	}

	errno = 0;
	wantraw = (size_t) strtoul(argv[optind], &memsuffix, 0);
	if (errno != 0) {
		fprintf(stderr, "failed to parse memory argument");
		usage(argv[0]); /* doesn't return */
	}
	switch (*memsuffix) {
	case 'G':
	case 'g':
		memshift = 30; /* gigabytes */
		break;
	case 'M':
	case 'm':
		memshift = 20; /* megabytes */
		break;
	case 'K':
	case 'k':
		memshift = 10; /* kilobytes */
		break;
	case 'B':
	case 'b':
		memshift = 0; /* bytes*/
		break;
	case '\0': /* no suffix */
		memshift = 20; /* megabytes */
		break;
	default:
		/* bad suffix */
		usage(argv[0]); /* doesn't return */
	}
	wantbytes_orig = wantbytes = ((size_t) wantraw << memshift);
	wantmb = (wantbytes_orig >> 20);
	optind++;
	if (wantmb > maxmb) {
		fprintf(stderr, "This system can only address %llu MB.\n", (ull) maxmb);
		exit(EXIT_FAIL_NONSTARTER);
	}
	if (wantbytes < pagesize) {
		fprintf(stderr,
				"bytes %zu < pagesize %zu -- memory argument too large?\n",
				wantbytes, pagesize);
		exit(EXIT_FAIL_NONSTARTER);
	}

	if (optind >= argc) {
		loops = 0;
	} else {
		errno = 0;
		loops = strtoul(argv[optind], &loopsuffix, 0);
		if (errno != 0) {
			fprintf(stderr, "failed to parse number of loops");
			usage(argv[0]); /* doesn't return */
		}
		if (*loopsuffix != '\0') {
			fprintf(stderr, "loop suffix %c\n", *loopsuffix);
			usage(argv[0]); /* doesn't return */
		}
	}

	printf("want %lluMB (%llu bytes)\n", (ull) wantmb, (ull) wantbytes);
	buf = NULL;

	if (use_phys) {
		memfd = open(device_name, O_RDWR | O_SYNC);
		if (memfd == -1) {
			fprintf(stderr, "failed to open %s for physical memory: %s\n",
					device_name, strerror(errno));
			exit(EXIT_FAIL_NONSTARTER);
		}

#ifdef USE_MMAP64
		buf = (void volatile *) mmap64(0, wantbytes, PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_LOCKED, memfd, physaddrbase);
#else
		buf = (void volatile *) mmap(0, wantbytes, PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_LOCKED, memfd, physaddrbase);
#endif


		if (buf == MAP_FAILED) {
			fprintf(stderr, "failed to mmap %s for physical memory: %s\n",
					device_name, strerror(errno));
			exit(EXIT_FAIL_NONSTARTER);
		}

		if (mlock((void *) buf, wantbytes) < 0) {
			fprintf(stderr, "failed to mlock mmap'ed space\n");
			do_mlock = 0;
		}

		bufsize = wantbytes; /* accept no less */
		aligned = buf;
		done_mem = 1;
	}

	while (!done_mem) {
		while (!buf && wantbytes) {
			buf = (void volatile *) malloc(wantbytes);
			if (!buf) {
				wantbytes -= pagesize;
			}

		}
		bufsize = wantbytes;
		printf("got  %lluMB (%llu bytes)", (ull) wantbytes >> 20,
				(ull) wantbytes);
		if (do_mlock) {
			printf(", trying mlock ...");
			if ((size_t) buf % pagesize) {
				aligned = (void volatile *) ((size_t) buf & pagesizemask)
						+ pagesize;
				bufsize -= ((size_t) aligned - (size_t) buf);
			} else {
				aligned = buf;
			}
			/* Try mlock */
			if (mlock((void *) aligned, bufsize) < 0) {
				switch (errno) {
				case EAGAIN: /* BSDs */
					printf("over system/pre-process limit, reducing...\n");
					free((void *) buf);
					buf = NULL;
					wantbytes -= pagesize;
					break;
				case ENOMEM:
					printf("too many pages, reducing...\n");
					free((void *) buf);
					buf = NULL;
					wantbytes -= pagesize;
					break;
				case EPERM:
					printf("insufficient permission.\n");
					printf("Trying again, unlocked:\n");
					do_mlock = 0;
					free((void *) buf);
					buf = NULL;
					wantbytes = wantbytes_orig;
					break;
				default:
					printf("failed for unknown reason.\n");
					do_mlock = 0;
					done_mem = 1;
				}
			} else {
				printf("locked.\n");
				done_mem = 1;
			}
		} else {
			done_mem = 1;
			printf("\n");
		}
	}

	if (!do_mlock)
		fprintf(stderr, "Continuing with unlocked memory; testing "
				"will be slower and less reliable.\n");

	fflush(stdout);

	if (test_pattern_file != NULL) {
		if (set_pattern_count(test_pattern_file) == 0)
			usage(argv[0]);

		if (set_test_patterns(test_pattern_file) < 0)
			usage(argv[0]);

		print_test_patterns();
	}

	if (strcmp(test_list_str, "all") != 0) {
		if((test_count = get_test_count(test_list_str)) == 0)
			usage(argv[0]);

		if ((test_list = create_test_list(test_list_str, test_count)) == NULL)
			usage(argv[0]);

		print_test_list(test_list);
	}

	halflen = bufsize / 2;
	count = halflen / sizeof(ul);
	bufa = (ulv *) aligned;
	bufb = (ulv *) ((size_t) aligned + halflen);

	for (loop = 1; (((!loops) || loop <= loops) && (is_continue)); loop++) {
		printf("Loop %lu", loop);
		if (loops) {
			printf("/%lu", loops);
		}
		printf(":\n");
		fflush(stdout);

		if (strcmp(test_list_str, "all") != 0) {
			i = 0;
			while(test_list[i] != NULL) {
				for (j = 0;; j++) {
					if (!tests[j].name)
						break;

					if (strcmp(tests[j].name, test_list[i]) == 0) {
						break;
					}
				}

				if (!tests[j].name)
					continue;

				if (run_test(j, aligned, bufsize, bufa, bufb, count) < 0) {
					exit_code |= EXIT_FAIL_OTHERTEST;

					if (keep_going == 1) {
						is_continue = 0;
						break;
					}
					else if (keep_going == 3) {
						is_continue = 0;
						system("echo c > /proc/sysrq-trigger");
						break;
					}
					else if (keep_going == 4) {
						is_continue = 0;
						if (crash_command == NULL) {
							system("echo c > /proc/sysrq-trigger");
						} else {
							system(crash_command);
						}
						break;
					}
				}

				i++;
			}
		} else { // test_list_str == "all"
			for (i = 0;; i++) {
				if (!tests[i].name)
					break;

				if (run_test(i, aligned, bufsize, bufa, bufb, count) < 0) {
					exit_code |= EXIT_FAIL_OTHERTEST;

					if (keep_going == 1) {
						is_continue = 0;
						break;
					}
					else if (keep_going == 3) {
						is_continue = 0;
						system("echo c > /proc/sysrq-trigger");
						break;
					}
					else if (keep_going == 4) {
						is_continue = 0;
						if (crash_command == NULL) {
							system("echo c > /proc/sysrq-trigger");
						} else {
							system(crash_command);
						}
						break;
					}
				}
			}
		}

		printf("\n");
		fflush(stdout);
	}
	if (do_mlock)
		munlock((void *) aligned, bufsize);

	if (test_pattern_file != NULL)
		free((void *)test_patterns);

	delete_test_list(test_list);

	printf("Done.\n");
	fflush(stdout);
	exit(exit_code);
}
