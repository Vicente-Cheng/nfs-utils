/*
 * support/include/mounts.h
 *
 * Routines used to mount and umount different
 * type of filesystems.
 *
 */

#ifndef MOUNTS_H
#define MOUNTS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/mount.h>

struct mountargs {
	const char *spec;
	const char *node;
	const char *type;
	int flags;
	void *data;
};
extern int v4root_mount(struct mountargs *args);
extern int v4root_umount(struct mountargs *args);

#ifndef MNT_DETACH
#define MNT_DETACH  0x00000002  /* Just detach from the tree */
#endif

inline int
bind_mount(char *dev, char *dir)
{
	struct mountargs args = {dev, dir, "bind", MS_BIND, NULL};

	xlog(D_CALL, "mount --bind %s %s", dev, dir);

	return v4root_mount(&args);
}
inline int
tmpfs_mount(char *dir)
{
	struct mountargs args = {"tmpfs", dir, "tmpfs", 0, NULL};

	xlog(D_CALL, "mount -t tmpfs tmpfs %s", dir);

	return v4root_mount(&args);
}
inline int
lazy_umount(char *dir)
{
	struct mountargs args = {NULL, dir, NULL, MNT_DETACH, NULL};

	xlog(D_CALL, "umount -l %s", dir);

	return v4root_umount(&args);
}

#endif /* MOUNTS_H */
