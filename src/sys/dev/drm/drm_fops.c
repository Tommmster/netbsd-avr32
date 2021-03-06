/* $NetBSD: drm_fops.c,v 1.4 2007/12/11 11:17:31 lukem Exp $ */

/* drm_fops.h -- File operations for DRM -*- linux-c -*-
 * Created: Mon Jan  4 08:58:31 1999 by faith@valinux.com
 */
/*-
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Rickard E. (Rik) Faith <faith@valinux.com>
 *    Daryll Strauss <daryll@valinux.com>
 *    Gareth Hughes <gareth@valinux.com>
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_fops.c,v 1.4 2007/12/11 11:17:31 lukem Exp $");
/*
__FBSDID("$FreeBSD: src/sys/dev/drm/drm_fops.c,v 1.2 2005/11/28 23:13:52 anholt Exp $");
*/

#include "drmP.h"

drm_file_t *drm_find_file_by_proc(drm_device_t *dev, DRM_STRUCTPROC *p)
{
	int restart = 1;
	uid_t uid = kauth_cred_getsvuid(p->p_cred);
	pid_t pid = p->p_pid;
	drm_file_t *priv;

	DRM_SPINLOCK_ASSERT(&dev->dev_lock);

	while (restart) {
		restart = 0;
		TAILQ_FOREACH(priv, &dev->files, link) {

	/* if the process disappeared, free the resources 
	 * NetBSD only calls drm_close once, so this frees
	 * resources earlier.
	 */
			if (pfind(priv->pid) == NULL) {
				drm_close_pid(dev, priv, priv->pid);
				restart = 1;
				break;
			}
			else
			if (priv->pid == pid && priv->uid == uid)
				return priv;
		}
	}
	return NULL;
}

/* drm_open_helper is called whenever a process opens /dev/drm. */
int drm_open_helper(DRM_CDEV kdev, int flags, int fmt, DRM_STRUCTPROC *p,
		    drm_device_t *dev)
{
	int	     m = minor(kdev);
	drm_file_t   *priv;
	int retcode;

	if (flags & O_EXCL)
		return EBUSY; /* No exclusive opens */
	dev->flags = flags;

	DRM_DEBUG("pid = %d, minor = %d\n", DRM_CURRENTPID, m);

	DRM_LOCK();
	priv = drm_find_file_by_proc(dev, p);
	if (priv) {
		priv->refs++;
	} else {
		priv = malloc(sizeof(*priv), M_DRM, M_NOWAIT | M_ZERO);
		if (priv == NULL) {
			DRM_UNLOCK();
			return DRM_ERR(ENOMEM);
		}
		priv->uid		= kauth_cred_getsvuid(p->p_cred);
		priv->pid		= p->p_pid;
		priv->refs		= 1;
		priv->minor		= m;
		priv->ioctl_count 	= 0;

		/* for compatibility root is always authenticated */
		priv->authenticated	= DRM_SUSER(p);

		if (dev->driver.open) {
			retcode = dev->driver.open(dev, priv);
			if (retcode != 0) {
				free(priv, M_DRM);
				DRM_UNLOCK();
				return retcode;
			}
		}

		/* first opener automatically becomes master */
		priv->master = TAILQ_EMPTY(&dev->files);

		TAILQ_INSERT_TAIL(&dev->files, priv, link);
	}
	DRM_UNLOCK();
	return 0;
}


/* The drm_read and drm_poll are stubs to prevent spurious errors
 * on older X Servers (4.3.0 and earlier) */

int drm_read(DRM_CDEV kdev, struct uio *uio, int ioflag)
{
	return 0;
}

int drm_poll(DRM_CDEV kdev, int events, DRM_STRUCTCDEVPROC *p)
{
	return 0;
}
