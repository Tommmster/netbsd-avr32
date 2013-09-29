/*	$NetBSD: linux_types.h,v 1.10 2008/04/28 20:23:43 martin Exp $ */

/*-
 * Copyright (c) 1995, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden, Eric Haszlakiewicz and Emmanuel Dreyfus.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AVR32_LINUX_TYPES_H
#define _AVR32_LINUX_TYPES_H

/*
 * from Linux's include/asm-avr32/posix-types.h
 */
typedef unsigned int linux_uid_t;
typedef unsigned int linux_gid_t;
typedef unsigned int linux_dev_t;
typedef unsigned int linux_mode_t;
typedef unsigned long linux_time_t;
typedef long linux_clock_t;
typedef long linux_off_t;
typedef int linux_pid_t;

typedef unsigned long linux_ino_t;
typedef unsigned int linux_nlink_t;
typedef linux_ino_t linux_ino_t32;
typedef linux_nlink_t linux_nlink_t32;
typedef int linux_timer_t;

/*
 * From Linux's include/asm-avr32/termbits.h
 */
typedef unsigned char linux_cc_t;
typedef unsigned int linux_speed_t;
typedef unsigned int linux_tcflag_t;

/*
 * This matches struct stat64 in glibc2.1, hence the absolutely
 * insane amounts of padding around dev_t's.
 *
 * Still from Linux's include/asm-avr32/stat.h
 */
struct linux_stat64 {
	unsigned long long	lst_dev;
	unsigned long long	lst_ino;
	linux_mode_t	lst_mode;
	linux_nlink_t	lst_nlink;
	linux_uid_t	lst_uid;
	linux_gid_t	lst_gid;
	unsigned long long	lst_rdev;
	long long	lst_size;
	unsigned long	lst__pad1;	/* align 64-bit st_blocks */
	unsigned long	lst_blksize;
	unsigned long long	lst_blocks;	/* Number 512-byte blocks allocated. */
	linux_time_t	lst_atime;
	linux_time_t	lst_atime_nsec;
	linux_time_t	lst_mtime;
	linux_time_t	lst_mtime_nsec;
	linux_time_t	lst_ctime;
	linux_time_t	lst_ctime_nsec;
	unsigned long	__unused1;
	unsigned long	__unused2;
};

/*
 * struct linux_stat is defined in Linux's include/asm-avr32/stat.h
 * There is also a old_kernel_stat in Linux
 */
struct linux_stat {     
        short		lst_dev;
        short		lst_ino;
        short		lst_mode;
        unsigned short	lst_nlink;
        short		lst_uid;
        short		lst_gid;
        unsigned long	lst_rdev;
        unsigned long	lst_size;
	unsigned long 	lst_blksize;
	unsigned long 	lst_blocks;
        unsigned long	lst_atime;
        unsigned long	lst_atime_nsec;
        unsigned long	lst_mtime;
        unsigned long	lst_mtime_nsec;
        unsigned long	lst_ctime;
        unsigned long	lst_ctime_nsec;
	unsigned long	__unused4;
	unsigned long	__unused5;
}; 

#endif /* !_AVR32_LINUX_TYPES_H */
