/*	$NetBSD$	*/

__KERNEL_RCSID(1, "$NetBSD$");

const int linux_to_native_signo[LINUX__NSIG] = {
	0,		/* 0 */
	SIGHUP,		/* 1 */
	SIGINT,		/* 2 */
	SIGQUIT,	/* 3 */
	SIGILL,		/* 4 */
	SIGTRAP,	/* 5 */
	SIGABRT,	/* 6 */
	SIGBUS,		/* 7 */
	SIGFPE,		/* 8 */
	SIGKILL,	/* 9 */
	SIGUSR1,	/* 10 */
	SIGSEGV,	/* 11 */
	SIGUSR2,	/* 12 */
	SIGPIPE,	/* 13 */
	SIGALRM,	/* 14 */
	SIGTERM,	/* 15 */
	0,		/* 16 SIGSTKFLT */
	SIGCHLD,	/* 17 */
	SIGCONT,	/* 18 */
	SIGSTOP,	/* 19 */
	SIGTSTP,	/* 20 */
	SIGTTIN,	/* 21 */
	SIGTTOU,	/* 22 */
	SIGURG,		/* 23 */
	SIGXCPU,	/* 24 */
	SIGXFSZ,	/* 25 */
	SIGVTALRM,	/* 26 */
	SIGPROF,	/* 27 */
	SIGWINCH,	/* 28 */
	SIGIO,		/* 29 */
	SIGPWR,		/* 30 */
	SIGSYS,		/* 31 */
	SIGRTMIN + 0,	/* 32 */
	SIGRTMIN + 1,	/* 33 */
	SIGRTMIN + 2,	/* 34 */
	SIGRTMIN + 3,	/* 35 */
	SIGRTMIN + 4,	/* 36 */
	SIGRTMIN + 5,	/* 37 */
	SIGRTMIN + 6,	/* 38 */
	SIGRTMIN + 7,	/* 39 */
	SIGRTMIN + 8,	/* 40 */
	SIGRTMIN + 9,	/* 41 */
	SIGRTMIN + 10,	/* 42 */
	SIGRTMIN + 11,	/* 43 */
	SIGRTMIN + 12,	/* 44 */
	SIGRTMIN + 13,	/* 45 */
	SIGRTMIN + 14,	/* 46 */
	SIGRTMIN + 15,	/* 47 */
	SIGRTMIN + 16,	/* 48 */
	SIGRTMIN + 17,	/* 49 */
	SIGRTMIN + 18,	/* 50 */
	SIGRTMIN + 19,	/* 51 */
	SIGRTMIN + 20,	/* 52 */
	SIGRTMIN + 21,	/* 53 */
	SIGRTMIN + 22,	/* 54 */
	SIGRTMIN + 23,	/* 55 */
	SIGRTMIN + 24,	/* 56 */
	SIGRTMIN + 25,	/* 57 */
	SIGRTMIN + 26,	/* 58 */
	SIGRTMIN + 27,	/* 59 */
	SIGRTMIN + 28,	/* 60 */
	SIGRTMIN + 29,	/* 61 */
	SIGRTMIN + 30,	/* 62 */
	0,		/* 63 */
};
