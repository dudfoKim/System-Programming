#ifndef __CONFIG_H_
#define __CONFIG_H_

/*
 * config.h - malloc lab configuration file
 *
 * Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
 * May not be used, modified, or copied without permission.
 */

/*
 * This is the default path where the driver will look for the
 * default tracefiles. You can override it at runtime with the -t flag.
 */
#define TRACEDIR "./traces/"

/*
 * This is the list of default tracefiles in TRACEDIR that the driver
 * will use for testing. Modify this if you want to add or delete
 * traces from the driver's test suite.
 *
 * The first four test correctness.  The last several test utilization
 * and performance.
 */
#define DEFAULT_TRACEFILES \
	"malloc.rep", \
        "malloc-free.rep", \
        "corners.rep", \
        "perl.rep",\
        "hostname.rep",\
        "xterm.rep",\
	"amptjp-bal.rep", \
	"cccp-bal.rep", \
	"cp-decl-bal.rep", \
	"expr-bal.rep", \
	"coalescing-bal.rep", \
	"random-bal.rep", \
	"binary-bal.rep" 

/* 
 * Students can get more points for building faster allocators, up to
 * this point (in ops / sec)
 */
#define MAX_SPEED       1000000

/* 
 * Students can get more points for building more efficient allocators,
 * up to this point (1 is perfect).
 */
#define MAX_SPACE       0.93

 /*
  * This constant determines the contributions of space utilization
  * (UTIL_WEIGHT) and throughput (1 - UTIL_WEIGHT) to the performance
  * index.
  */
#define UTIL_WEIGHT .60

/*
 * Alignment requirement in bytes (either 4 or 8)
 */
#define ALIGNMENT 8

/*
 * Maximum heap size in bytes
 */
#define MAX_HEAP (40*(1<<20))  /* 20 MB */

/*****************************************************************************
 * Set exactly one of these USE_xxx constants to "1" to select a timing method
 *****************************************************************************/
#define USE_FCYC   1   /* cycle counter w/K-best scheme (x86 & Alpha only) */
#define USE_ITIMER 0   /* interval timer (any Unix box) */
#define USE_GETTOD 0   /* gettimeofday (any Unix box) */

#endif /* __CONFIG_H */
