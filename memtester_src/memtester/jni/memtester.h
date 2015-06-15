/*
 * Very simple (yet, for some reason, very effective) memory tester.
 * Originally by Simon Kirby <sim@stormix.com> <sim@neato.org>
 * Version 2 by Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Version 3 not publicly released.
 * Version 4 rewrite:
 * Copyright (C) 2004-2012 Charles Cazabon <charlesc-memtester@pyropus.ca>
 * Licensed under the terms of the GNU General Public License version 2 (only).
 * See the file COPYING for details.
 *
 * This file contains the declarations for external variables from the main file.
 * See other comments in that file.
 *
 */

#ifndef	__MEMTESTER_MEMTESTER_H__
#define	__MEMTESTER_MEMTESTER_H__

#include <stddef.h>
#include <sys/types.h>
#include "types.h"

int memtester_pagesize(void);

/* extern declarations. */

extern int use_phys;
extern off_t physaddrbase;
extern ul *test_patterns;
extern size_t pattern_count;
extern char *command;
extern size_t read_count;

#endif 	// __MEMTESTER_MEMTESTER_H__
