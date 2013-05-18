/*
 * MACHINE GENERATED: DO NOT EDIT
 *
 * swapnetbsd.c, from "MINIMAL"
 */

#include <sys/param.h>
#include <sys/conf.h>

const char *rootspec = NULL;
dev_t	rootdev = NODEV;	/* wildcarded */

const char *dumpspec = NULL;
dev_t	dumpdev = NODEV;	/* unspecified */

int (*mountroot)(void) = NULL;
