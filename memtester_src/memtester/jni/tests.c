/*
 * Very simple but very effective user-space memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 * This file contains the functions for the actual tests, called from the
 * main routine in memtester.c.  See other comments in that file.
 *
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "types.h"
#include "memtester.h"
#include "get_phy_addr.h"

#define ONE 0x00000001L

/* Function definitions. */
static void run_command() {
	if (command == NULL) {
		return;
	}

	system(command);
}

static int check_using_pattern(ulv *bufa, size_t count) {
	int r = 0;
	size_t i;
	ulv *p1;
	off_t physaddr;
	ull phy_addr;
	size_t pattern_index;

	run_command();

	p1 = bufa;
	pattern_index = 0;

	for (i = 0; i < count; i++, p1++, pattern_index++) {
		if (pattern_index == pattern_count)
			pattern_index = 0;

		if (*p1 != test_patterns[pattern_index]) {
			if (use_phys) {
				physaddr = physaddrbase + (i * sizeof(ul));
				fprintf(stderr,
						"First Read FAILURE: 0x%08lX != 0x%08lX at physical address "
								"0x%08lX.\n", (ul) *p1, (ul) test_patterns[pattern_index], physaddr);
			} else {
				fprintf(stderr,
						"First Read FAILURE: 0x%08lX != 0x%08lX at offset 0x%08lX.\n",
						(ul) *p1, (ul) test_patterns[pattern_index], (ul) (i * sizeof(ul)));

				if (get_phy_addr(&phy_addr, 0, GET_VIRT_ADDR(p1)) == 0) {
					fprintf(stderr,
							"First Read FAILURE: 0x%08lX != 0x%08lX at physical address "
									"0x%llX.\n", (ul) *p1, (ul) test_patterns[pattern_index], phy_addr);
				}
			}
			fflush(stdout);

			r = -1;
		}
	}

	p1 = bufa;
	pattern_index = 0;

	for (i = 0; i < count; i++, p1++, pattern_index++) {
		if (pattern_index == pattern_count)
			pattern_index = 0;

		if (*p1 != test_patterns[pattern_index]) {
			if (use_phys) {
				physaddr = physaddrbase + (i * sizeof(ul));
				fprintf(stderr,
						"Second Read FAILURE: 0x%08lX != 0x%08lX at physical address "
								"0x%08lX.\n", (ul) *p1, (ul) test_patterns[pattern_index], physaddr);
			} else {
				fprintf(stderr,
						"Second Read FAILURE: 0x%08lX != 0x%08lX at offset 0x%08lX.\n",
						(ul) *p1, (ul) test_patterns[pattern_index], (ul) (i * sizeof(ul)));

				if (get_phy_addr(&phy_addr, 0, GET_VIRT_ADDR(p1)) == 0) {
					fprintf(stderr,
							"Second Read FAILURE: 0x%08lX != 0x%08lX at physical address "
									"0x%llX.\n", (ul) *p1, (ul) test_patterns[pattern_index], phy_addr);
				}
			}
			fflush(stdout);

			r = -1;
		}
	}

	return r;
}

int test_using_pattern(ulv *bufa, size_t count) {
	ulv *p1;
	unsigned int j;
	size_t i;
	size_t pattern_index;

	if (test_patterns == NULL) {
		fprintf(stderr, "There is no test_patterns. use \"-i\" option. skip this test.: ");
		return 0;
	}

	for (j = 0; j < 16; j++) {
		p1 = (ulv *) bufa;
		pattern_index = 0;

		for (i = 0; i < count; i++, pattern_index++) {
			if (pattern_index == pattern_count)
				pattern_index = 0;

			*p1++ = test_patterns[pattern_index];
		}
		if (check_using_pattern(bufa, count)) {
			return -1;
		}
	}
	return 0;
}

static int check_stuck_address(ulv *bufa, size_t count) {
	int r = 0;
	size_t i;
	ulv *p1;
	off_t physaddr;
	ull phy_addr;

	run_command();

	p1 = bufa;

	for (i = 0; i < count; i++, p1++) {
		if (*p1 != ((i % 2) == 0 ? (ul) p1 : ~((ul) p1))) {
			if (use_phys) {
				physaddr = physaddrbase + (i * sizeof(ul));
				fprintf(stderr,
						"First Read FAILURE: possible bad address line at physical "
								"address 0x%08lX.\n", physaddr);
			} else {
				fprintf(stderr,
						"First Read FAILURE: possible bad address line at offset "
								"0x%08lX.\n", (ul) (i * sizeof(ul)));

				if (get_phy_addr(&phy_addr, 0, GET_VIRT_ADDR(p1)) == 0) {
					fprintf(stderr,
							"First Read FAILURE: possible bad address line at physical "
							"address 0x%llX.\n", phy_addr);
				}
			}
			fflush(stdout);

			r = -1;
		}
	}

	p1 = bufa;

	for (i = 0; i < count; i++, p1++) {
		if (*p1 != ((i % 2) == 0 ? (ul) p1 : ~((ul) p1))) {
			if (use_phys) {
				physaddr = physaddrbase + (i * sizeof(ul));
				fprintf(stderr,
						"Second Read FAILURE: possible bad address line at physical "
								"address 0x%08lX.\n", physaddr);
			} else {
				fprintf(stderr,
						"Second Read FAILURE: possible bad address line at offset "
								"0x%08lX.\n", (ul) (i * sizeof(ul)));

				if (get_phy_addr(&phy_addr, 0, GET_VIRT_ADDR(p1)) == 0) {
					fprintf(stderr,
							"Second Read FAILURE: possible bad address line at physical "
							"address 0x%llX.\n", phy_addr);
				}
			}
			fflush(stdout);

			r = -1;
		}
	}

	return r;
}

int test_stuck_address(ulv *bufa, size_t count) {
	ulv *p1;
	unsigned int j;
	size_t i;

	for (j = 0; j < 16; j++) {
		p1 = (ulv *) bufa;

		for (i = 0; i < count; i++) {
			*p1 = ((i % 2) == 0 ? (ul) p1 : ~((ul) p1));
			p1++;
		}
		if (check_stuck_address(bufa, count)) {
			return -1;
		}
	}
	return 0;
}

static int compare_regions_0(ulv *bufa, ulv *bufb, size_t count) {
	int r = 0;
	size_t i;
	ulv *p1;
	ulv *p2;
	off_t physaddr1, physaddr2;
	ull phy_addr_p1, phy_addr_p2;

	p1 = bufa;
	p2 = bufb;

	for (i = 0; i < count; i++, p1++, p2++) {
		if (*p1 != *p2) {
			if (use_phys) {
				physaddr1 = physaddrbase + (i * sizeof(ul));
				physaddr2 = physaddrbase + (count * sizeof(ul)) + (i * sizeof(ul));
				fprintf(stderr,
						"First Read FAILURE: 0x%08lX != 0x%08lX at physical address "
								"[0x%08lX], [0x%08lX].\n",
								(ul) *p1, (ul) *p2, physaddr1, physaddr2);
			} else {
				fprintf(stderr,
						"First Read FAILURE: 0x%08lX != 0x%08lX at offset 0x%08lX.\n",
						(ul) *p1, (ul) *p2, (ul) (i * sizeof(ul)));

				if (get_phy_addr(&phy_addr_p1, 0, GET_VIRT_ADDR(p1)) == 0) {
					if (get_phy_addr(&phy_addr_p2, 0, GET_VIRT_ADDR(p2)) == 0) {
						fprintf(stderr,
								"First Read FAILURE: 0x%08lX != 0x%08lX at physical address "
								"[0x%llX], [0x%llX].\n",
								(ul) *p1, (ul) *p2, phy_addr_p1, phy_addr_p2);
					}
				}
			}
			fflush(stderr);

			r = -1;
		}
	}

	p1 = bufa;
	p2 = bufb;

	for (i = 0; i < count; i++, p1++, p2++) {
		if (*p1 != *p2) {
			if (use_phys) {
				physaddr1 = physaddrbase + (i * sizeof(ul));
				physaddr2 = physaddrbase + (count * sizeof(ul)) + (i * sizeof(ul));
				fprintf(stderr,
						"Second Read FAILURE: 0x%08lX != 0x%08lX at physical address "
								"[0x%08lX], [0x%08lX].\n",
								(ul) *p1, (ul) *p2, physaddr1, physaddr2);
			} else {
				fprintf(stderr,
						"Second Read FAILURE: 0x%08lX != 0x%08lX at offset 0x%08lX.\n",
						(ul) *p1, (ul) *p2, (ul) (i * sizeof(ul)));

				if (get_phy_addr(&phy_addr_p1, 0, GET_VIRT_ADDR(p1)) == 0) {
					if (get_phy_addr(&phy_addr_p2, 0, GET_VIRT_ADDR(p2)) == 0) {
						fprintf(stderr,
								"Second Read FAILURE: 0x%08lX != 0x%08lX at physical address "
								"[0x%llX], [0x%llX].\n",
								(ul) *p1, (ul) *p2, phy_addr_p1, phy_addr_p2);
					}
				}
			}
			fflush(stderr);

			r = -1;
		}
	}

	return r;
}

static int compare_regions(ulv *bufa, ulv *bufb, size_t count) {
	size_t i;

	for(i=0; i<read_count; i++) {
		run_command();

		if (memcmp((void *)bufa, (void *)bufb, sizeof(ulv) * count)) {
			if (compare_regions_0(bufa, bufb, count)) {
				return -1;
			}
		}
	}

	return 0;
}

int test_random_value(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;

	for (i = 0; i < count; i++) {
		*p1++ = *p2++ = rand_ul();
	}
	return compare_regions(bufa, bufb, count);
}

int test_xor_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	// for initialization
	test_random_value(bufa, bufb, count);

	for (i = 0; i < count; i++) {
		*p1++ ^= q;
		*p2++ ^= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_sub_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	// for initialization
	test_random_value(bufa, bufb, count);

	for (i = 0; i < count; i++) {
		*p1++ -= q;
		*p2++ -= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_mul_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	// for initialization
	test_random_value(bufa, bufb, count);

	for (i = 0; i < count; i++) {
		*p1++ *= q;
		*p2++ *= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_div_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	// for initialization
	test_random_value(bufa, bufb, count);

	for (i = 0; i < count; i++) {
		if (!q) {
			q++;
		}
		*p1++ /= q;
		*p2++ /= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_or_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	// for initialization
	test_random_value(bufa, bufb, count);

	for (i = 0; i < count; i++) {
		*p1++ |= q;
		*p2++ |= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_and_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	// for initialization
	test_random_value(bufa, bufb, count);

	for (i = 0; i < count; i++) {
		*p1++ &= q;
		*p2++ &= q;
	}
	return compare_regions(bufa, bufb, count);
}

int test_seqinc_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	size_t i;
	ul q = rand_ul();

	for (i = 0; i < count; i++) {
		*p1++ = *p2++ = (i + q);
	}
	return compare_regions(bufa, bufb, count);
}

int test_solidbits_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	ul q;
	size_t i;

	for (j = 0; j < 64; j++) {
		q = (j % 2) == 0 ? UL_ONEBITS : 0;
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		for (i = 0; i < count; i++) {
			*p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
		}
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	return 0;
}

int test_checkerboard_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	ul q;
	size_t i;

	for (j = 0; j < 64; j++) {
		q = (j % 2) == 0 ? CHECKERBOARD1 : CHECKERBOARD2;
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		for (i = 0; i < count; i++) {
			*p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
		}
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	return 0;
}

int test_blockseq_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	size_t i;

	for (j = 0; j < 256; j++) {
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		for (i = 0; i < count; i++) {
			*p1++ = *p2++ = (ul) UL_BYTE(j);
		}
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	return 0;
}

int test_walkbits0_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	size_t i;

	for (j = 0; j < UL_LEN * 2; j++) {
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		for (i = 0; i < count; i++) {
			if (j < UL_LEN) { /* Walk it up. */
				*p1++ = *p2++ = ONE << j;
			} else { /* Walk it back down. */
				*p1++ = *p2++ = ONE << (UL_LEN * 2 - j - 1);
			}
		}
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	return 0;
}

int test_walkbits1_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	size_t i;

	for (j = 0; j < UL_LEN * 2; j++) {
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		for (i = 0; i < count; i++) {
			if (j < UL_LEN) { /* Walk it up. */
				*p1++ = *p2++ = UL_ONEBITS ^ (ONE << j);
			} else { /* Walk it back down. */
				*p1++ = *p2++ = UL_ONEBITS ^ (ONE << (UL_LEN * 2 - j - 1));
			}
		}
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	return 0;
}

int test_bitspread_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	size_t i;

	for (j = 0; j < UL_LEN * 2; j++) {
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		for (i = 0; i < count; i++) {
			if (j < UL_LEN) { /* Walk it up. */
				*p1++ = *p2++ =
						(i % 2 == 0) ?
								(ONE << j) | (ONE << (j + 2)) :
								UL_ONEBITS ^ ((ONE << j) | (ONE << (j + 2)));
			} else { /* Walk it back down. */
				*p1++ =
						*p2++ =
								(i % 2 == 0) ?
										(ONE << (UL_LEN * 2 - 1 - j))
												| (ONE << (UL_LEN * 2 + 1 - j)) :
										UL_ONEBITS
												^ (ONE << (UL_LEN * 2 - 1 - j)
														| (ONE
																<< (UL_LEN * 2
																		+ 1 - j)));
			}
		}
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	return 0;
}

int test_bitflip_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j, k;
	ul q;
	size_t i;

	for (k = 0; k < UL_LEN; k++) {
		q = ONE << k;
		for (j = 0; j < 8; j++) {
			q = ~q;
			p1 = (ulv *) bufa;
			p2 = (ulv *) bufb;

			for (i = 0; i < count; i++) {
				*p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
			}

			if (compare_regions(bufa, bufb, count)) {
				return -1;
			}
		}
	}
	return 0;
}

#define BUF_SIZE 1024
#define BYTE_PATTERN 0x01010101
#define BIT_PER_BYTE 8

int test_fast_bitflip_comparison(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j, k;
	ul q;
	size_t i;
	size_t block_count;
	int not_use_memcpy = (count % BUF_SIZE);

	if (not_use_memcpy) {
		fprintf(stderr, "count is not multiple %d. not using memcpy.: ", BUF_SIZE);
	}

	for (k = 0; k < BIT_PER_BYTE; k++) {
		q = BYTE_PATTERN << k;
		for (j = 0; j < 8; j++) {
			q = ~q;
			p1 = (ulv *) bufa;
			p2 = (ulv *) bufb;

			if (not_use_memcpy) {
				for (i = 0; i < count; i++) {
					*p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
				}

			} else {
				block_count = (count / BUF_SIZE) - 1;

				for (i = 0; i < BUF_SIZE ; i++) {
					*p1++ = (i % 2) == 0 ? q : ~q;
				}
				memcpy((void *)p2, (void *)bufa, BUF_SIZE * sizeof(*p1));
				p2 += BUF_SIZE;

				for (i = 0; i < block_count ; i++) {
					memcpy((void *)p1, (void *)bufa, BUF_SIZE * sizeof(*p1));
					memcpy((void *)p2, (void *)bufa, BUF_SIZE * sizeof(*p1));
					p1 += BUF_SIZE;
					p2 += BUF_SIZE;
				}
			}

			if (compare_regions(bufa, bufb, count)) {
				return -1;
			}
		}
	}
	return 0;
}

int test_write_only_checkerboard(ulv *bufa, ulv *bufb, size_t count) {
	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j;
	ul q;
	size_t i;

	for (j = 0; j < 64; j++) {
		q = CHECKERBOARD1;
		p1 = (ulv *) bufa;
		p2 = (ulv *) bufb;
		for (i = 0; i < count; i++) {
			*p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
		}

		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	return 0;
}

int test_nop_comparison(ulv *bufa, ulv *bufb, size_t count) {
	int status;
	char * pfile = "/data/local/tmp/stopcmd" ;

	// remove the previous file .
	remove(pfile);
	
	while(1)
	{
		status = remove(pfile);
		//printf("status is %d", status) ;
		if ( status == 0 ) 
			break;
		else
			run_command();
	}
	return 0;
}

int test_move_comparison(ulv *bufa, ulv *bufb, size_t count) {
	int status, i ;
	char * pfile = "/data/local/tmp/stopcmd" ;
	ulv  *p1, *p2 ;

	// remove the previous file .
	remove(pfile);
	
	while(1)
	{
		p1 = (ulv*)bufa ; 
		p2 = (ulv*)bufb ; 
		for(i = 0 ; i< 1000 ; i++ ) 
			*p1++ = (*p1 + *p2 ) * *p2++  ;
		
		status = remove(pfile);
		//printf("status is %d", status) ;
		if ( status == 0 ) 
			break;
		else
			run_command();
	}
	return 0;
}

int test_bitflip_check(ulv *bufa, ulv *bufb, size_t count) 
{
	int status ;
	char * pstop = "./stopcmd" ;
	char * ppause = "./pausecmd" ;
	char * pstart = "./startcmd" ;
	

	ulv *p1 = bufa;
	ulv *p2 = bufb;
	unsigned int j, k;
	ul q;
	size_t i;
	size_t block_count;
	int not_use_memcpy = (count % BUF_SIZE);

	remove(pstop);

	while(1)
	{

		for (k = 0; k < BIT_PER_BYTE; k++) 
		{
			q = BYTE_PATTERN << k;
			for (j = 0; j < 8; j++) 
			{
				q = ~q;
				p1 = (ulv *) bufa;
				p2 = (ulv *) bufb;

				if (not_use_memcpy) 
				{
					for (i = 0; i < count; i++) 
					{
						*p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
					}

				} 
				else 
				{
					block_count = (count / BUF_SIZE) - 1;

					for (i = 0; i < BUF_SIZE ; i++) {
						*p1++ = (i % 2) == 0 ? q : ~q;
					}
					memcpy((void *)p2, (void *)bufa, BUF_SIZE * sizeof(*p1));
					p2 += BUF_SIZE;

					for (i = 0; i < block_count ; i++) {
						memcpy((void *)p1, (void *)bufa, BUF_SIZE * sizeof(*p1));
						memcpy((void *)p2, (void *)bufa, BUF_SIZE * sizeof(*p1));
						p1 += BUF_SIZE;
						p2 += BUF_SIZE;
					}
				}

				if (memcmp((void *)bufa, (void *)bufb, sizeof(ulv) * count) != 0 ) 
				{
					system("setprop memtest fail");
					printf("\nerror,bitflip comes out\n");
					return -1 ;
				}
				else
				{
					system("setprop memtest pass");
					printf("\npass,bitflip test done\n");
				}

				status = remove(pstop);
				if ( status == 0 ) 
					return 0 ;

				status = remove(ppause);
				while ( status == 0 ) 
				{
					sleep(5);
					status = remove(pstart);
					if (remove(pstop) == 0 ) 
						return  0 ; 
				}

				
				
			}
		}

	}
	return 0;
}


#ifdef TEST_NARROW_WRITES    
int test_8bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
	u8v *p1, *t;
	ulv *p2;
	int attempt;
	unsigned int b;
	size_t i;

	for (attempt = 0; attempt < 2; attempt++) {
		if (attempt & 1) {
			p1 = (u8v *) bufa;
			p2 = bufb;
		} else {
			p1 = (u8v *) bufb;
			p2 = bufa;
		}
		for (i = 0; i < count; i++) {
			t = mword8.bytes;
			*p2++ = mword8.val = rand_ul();
			for (b = 0; b < UL_LEN / 8; b++) {
				*p1++ = *t++;
			}
		}
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	return 0;
}

int test_16bit_wide_random(ulv* bufa, ulv* bufb, size_t count) {
	u16v *p1, *t;
	ulv *p2;
	int attempt;
	unsigned int b;
	size_t i;

	for (attempt = 0; attempt < 2; attempt++) {
		if (attempt & 1) {
			p1 = (u16v *) bufa;
			p2 = bufb;
		} else {
			p1 = (u16v *) bufb;
			p2 = bufa;
		}
		for (i = 0; i < count; i++) {
			t = mword16.u16s;
			*p2++ = mword16.val = rand_ul();
			for (b = 0; b < UL_LEN / 16; b++) {
				*p1++ = *t++;
			}
		}
		if (compare_regions(bufa, bufb, count)) {
			return -1;
		}
	}
	return 0;
}
#endif
