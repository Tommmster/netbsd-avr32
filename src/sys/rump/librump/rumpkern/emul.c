/*	$NetBSD: emul.c,v 1.53 2008/10/14 10:42:27 pooka Exp $	*/

/*
 * Copyright (c) 2007 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by Google Summer of Code.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define malloc(a,b,c) __wrap_malloc(a,b,c)

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/null.h>
#include <sys/vnode.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/syslog.h>
#include <sys/namei.h>
#include <sys/kauth.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kthread.h>
#include <sys/cpu.h>
#include <sys/kmem.h>
#include <sys/poll.h>
#include <sys/tprintf.h>
#include <sys/timetc.h>

#include <machine/stdarg.h>

#include <rump/rumpuser.h>

#include <uvm/uvm_map.h>

#include "rump_private.h"

time_t time_second = 1;

kmutex_t *proc_lock;
struct lwp lwp0;
struct vnode *rootvp;
struct device *root_device;
dev_t rootdev;
int physmem = 256*256; /* 256 * 1024*1024 / 4k, PAGE_SIZE not always set */
int doing_shutdown;
int ncpu = 1;
const int schedppq = 1;
int hardclock_ticks;
bool mp_online = false;
struct vm_map *mb_map;

char hostname[MAXHOSTNAMELEN];
size_t hostnamelen;

u_long	bufmem_valimit;
u_long	bufmem_hiwater;
u_long	bufmem_lowater;
u_long	bufmem;
u_int	nbuf;

const char *panicstr;
const char ostype[] = "NetBSD";
const char osrelease[] = "999"; /* paradroid 4evah */
const char kernel_ident[] = "RUMP-ROAST";
const char *domainname;
int domainnamelen;

const struct filterops seltrue_filtops;

void
panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	printf("panic: ");
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
	abort();
}

void
log(int level, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void
vlog(int level, const char *fmt, va_list ap)
{

	vprintf(fmt, ap);
}

void
uprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

/* relegate this to regular printf */
tpr_t
tprintf_open(struct proc *p)
{

	return (tpr_t)0x111;
}

void
tprintf(tpr_t tpr, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void
tprintf_close(tpr_t tpr)
{

}

void
printf_nolog(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void
aprint_normal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

int
copyin(const void *uaddr, void *kaddr, size_t len)
{

	memcpy(kaddr, uaddr, len);
	return 0;
}

int
copyout(const void *kaddr, void *uaddr, size_t len)
{

	memcpy(uaddr, kaddr, len);
	return 0;
}

int
copystr(const void *kfaddr, void *kdaddr, size_t len, size_t *done)
{

	return copyinstr(kfaddr, kdaddr, len, done);
}

int
copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
{

	strlcpy(kaddr, uaddr, len);
	if (done)
		*done = strlen(kaddr)+1; /* includes termination */
	return 0;
}

int
copyin_vmspace(struct vmspace *vm, const void *uaddr, void *kaddr, size_t len)
{

	return copyin(uaddr, kaddr, len);
}

int
copyout_vmspace(struct vmspace *vm, const void *kaddr, void *uaddr, size_t len)
{

	return copyout(kaddr, uaddr, len);
}

int
kcopy(const void *src, void *dst, size_t len)
{

	memcpy(dst, src, len);
	return 0;
}

int
uiomove(void *buf, size_t n, struct uio *uio)
{
	struct iovec *iov;
	uint8_t *b = buf;
	size_t cnt;
	int rv;

	if (uio->uio_vmspace != UIO_VMSPACE_SYS)
		panic("%s: vmspace != UIO_VMSPACE_SYS", __func__);

	/*
	 * See if rump ubc code claims the offset.  This is of course
	 * a blatant violation of abstraction levels, but let's keep
	 * me simple & stupid for now.
	 */
	if (rump_ubc_magic_uiomove(buf, n, uio, &rv, NULL))
		return rv;

	while (n && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;

		if (uio->uio_rw == UIO_READ)
			memcpy(iov->iov_base, b, cnt);
		else
			memcpy(b, iov->iov_base, cnt);

		iov->iov_base = (uint8_t *)iov->iov_base + cnt;
		iov->iov_len -= cnt;
		b += cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		n -= cnt;
	}

	return 0;
}

void
uio_setup_sysspace(struct uio *uio)
{

	uio->uio_vmspace = UIO_VMSPACE_SYS;
}

const struct bdevsw *
bdevsw_lookup(dev_t dev)
{

	return (const struct bdevsw *)1;
}

devclass_t
device_class(device_t dev)
{

	if (dev != root_device)
		panic("%s: dev != root_device not supported", __func__);

	return DV_DISK;
}

void
getmicrouptime(struct timeval *tvp)
{
	int error;

	rumpuser_gettimeofday(tvp, &error);
}

void
malloc_type_attach(struct malloc_type *type)
{

	return;
}

void
malloc_type_detach(struct malloc_type *type)
{

	return;
}

void *
__wrap_malloc(unsigned long size, struct malloc_type *type, int flags)
{
	void *rv;

	rv = rumpuser_malloc(size, (flags & (M_CANFAIL | M_NOWAIT)) != 0);
	if (rv && flags & M_ZERO)
		memset(rv, 0, size);

	return rv;
}

void
nanotime(struct timespec *ts)
{
	struct timeval tv;
	int error;

	rumpuser_gettimeofday(&tv, &error);
	TIMEVAL_TO_TIMESPEC(&tv, ts);
}

/* hooray for mick, so what if I do */
void
getnanotime(struct timespec *ts)
{

	nanotime(ts);
}

void
microtime(struct timeval *tv)
{
	int error;

	rumpuser_gettimeofday(tv, &error);
}

void
getmicrotime(struct timeval *tv)
{
	int error;

	rumpuser_gettimeofday(tv, &error);
}

void
bdev_strategy(struct buf *bp)
{

	panic("%s: not supported", __func__);
}

int
bdev_type(dev_t dev)
{

	return D_DISK;
}

struct kthdesc {
	void (*f)(void *);
	void *arg;
	struct lwp *mylwp;
};

static void *
threadbouncer(void *arg)
{
	struct kthdesc *k = arg;
	void (*f)(void *);
	void *thrarg;

	f = k->f;
	thrarg = k->arg;
	rumpuser_set_curlwp(k->mylwp);
	kmem_free(k, sizeof(struct kthdesc));

	f(thrarg);
	panic("unreachable, should kthread_exit()");
}

int
kthread_create(pri_t pri, int flags, struct cpu_info *ci,
	void (*func)(void *), void *arg, lwp_t **newlp, const char *fmt, ...)
{
	struct kthdesc *k;
	struct lwp *l;
	int rv;

	if (!rump_threads) {
		/* fake them */
		if (strcmp(fmt, "vrele") == 0) {
			printf("rump warning: threads not enabled, not starting"
			   " vrele thread\n");
			return 0;
		} else if (strcmp(fmt, "cachegc") == 0) {
			printf("rump warning: threads not enabled, not starting"
			   " namecache g/c thread\n");
			return 0;
		} else
			panic("threads not available, setenv RUMP_THREADS 1");
	}

	KASSERT(fmt != NULL);
	if (ci != NULL)
		panic("%s: bounded threads not supported", __func__);

	k = kmem_alloc(sizeof(struct kthdesc), KM_SLEEP);
	k->f = func;
	k->arg = arg;
	k->mylwp = l = rump_setup_curlwp(0, rump_nextlid(), 0);
	rv = rumpuser_thread_create(threadbouncer, k);
	if (rv)
		return rv;

	if (newlp)
		*newlp = l;
	return 0;
}

void
kthread_exit(int ecode)
{

	rumpuser_thread_exit();
}

struct proc *
p_find(pid_t pid, uint flags)
{

	panic("%s: not implemented", __func__);
}

struct pgrp *
pg_find(pid_t pid, uint flags)
{

	panic("%s: not implemented", __func__);
}

void
psignal(struct proc *p, int signo)
{

	switch (signo) {
	case SIGSYS:
		break;
	default:
		panic("unhandled signal %d", signo);
	}
}

void
kpsignal(struct proc *p, ksiginfo_t *ksi, void *data)
{

	panic("%s: not implemented", __func__);
}

void
kpgsignal(struct pgrp *pgrp, ksiginfo_t *ksi, void *data, int checkctty)
{

	panic("%s: not implemented", __func__);
}

int
pgid_in_session(struct proc *p, pid_t pg_id)
{

	panic("%s: not implemented", __func__);
}

int
sigispending(struct lwp *l, int signo)
{

	return 0;
}

void
sigpending1(struct lwp *l, sigset_t *ss)
{

	panic("%s: not implemented", __func__);
}

void
knote_fdclose(int fd)
{

	/* since we don't add knotes, we don't have to remove them */
}

int
seltrue_kqfilter(dev_t dev, struct knote *kn)
{

	panic("%s: not implemented", __func__);
}

int
kpause(const char *wmesg, bool intr, int timeo, kmutex_t *mtx)
{
	extern int hz;
	int rv, error;
	struct timespec time;
	
	if (mtx)
		mutex_exit(mtx);

	time.tv_sec = timeo / hz;
	time.tv_nsec = (timeo % hz) * (1000000000 / hz);

	rv = rumpuser_nanosleep(&time, NULL, &error);
	
	if (mtx)
		mutex_enter(mtx);

	if (rv)
		return error;

	return 0;
}

void
suspendsched()
{

	panic("%s: not implemented", __func__);
}

u_int
lwp_unsleep(lwp_t *l, bool cleanup)
{

	KASSERT(mutex_owned(l->l_mutex));

	return (*l->l_syncobj->sobj_unsleep)(l, cleanup);
}

vaddr_t
calc_cache_size(struct vm_map *map, int pct, int va_pct)
{
	paddr_t t;

	t = (paddr_t)physmem * pct / 100 * PAGE_SIZE;
	if ((vaddr_t)t != t) {
		panic("%s: needs tweak", __func__);
	}
	return t;
}

int
seltrue(dev_t dev, int events, struct lwp *l)
{
        return (events & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM));
}

void
selrecord(lwp_t *selector, struct selinfo *sip)
{
}

void
selinit(struct selinfo *sip)
{
}

void
selnotify(struct selinfo *sip, int events, long knhint)
{
}

void
seldestroy(struct selinfo *sip)
{
}

const char *
device_xname(device_t dv)
{
	return "bogus0";
}

void
assert_sleepable(void)
{

	/* always sleepable, although we should improve this */
}

int
devsw_attach(const char *devname, const struct bdevsw *bdev, int *bmajor,
	const struct cdevsw *cdev, int *cmajor)
{

	panic("%s: not implemented", __func__);
}

int
devsw_detach(const struct bdevsw *bdev, const struct cdevsw *cdev)
{

	panic("%s: not implemented", __func__);
}

void
tc_setclock(struct timespec *ts)
{

	panic("%s: not implemented", __func__);
}

void
proc_crmod_enter()
{

	panic("%s: not implemented", __func__);
}

void
proc_crmod_leave(kauth_cred_t c1, kauth_cred_t c2, bool sugid)
{

	panic("%s: not implemented", __func__);
}
