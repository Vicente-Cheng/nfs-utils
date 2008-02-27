/*
 * support/export/v4root.c
 *
 * Routines that create and destroy v4 pseudo roots
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>

#include "xlog.h"
#include "exportfs.h"
#include "nfslib.h"
#include "misc.h"
#include "mounts.h"
#include "execute.h"

#ifndef _PATH_PSEUDO_ROOT
#define _PATH_PSEUDO_ROOT		NFS_STATEDIR "/v4root"
#endif

struct mounts_t {
	TAILQ_ENTRY(mounts_t) list;
	nfs_export	*exp;
};
TAILQ_HEAD(mount_list, mounts_t) head = LIST_HEAD_INITIALIZER(head);
#define QUE_SORT(_m1) { \
	if (TAILQ_EMPTY(&head)) \
		TAILQ_INSERT_HEAD(&head, _m1, list); \
	else { \
		TAILQ_FOREACH(m2, &head, list) { \
			if (strlen(m2->exp->m_export.e_path) > \
					strlen(_m1->exp->m_export.e_path)) { \
				TAILQ_INSERT_BEFORE(m2, _m1, list); \
				break; \
			} \
		} \
		if (m2 == NULL) \
			TAILQ_INSERT_TAIL(&head, _m1, list); \
	} \
}

void v4root_destroy(void);
void v4root_create(void);
void v4root_umountall(void);

int v4root_needed, v4root_check;
static char errbuf[BUFSIZ];
static int v4root_mkroot(void);
static int v4root_mkroot(void);

static struct exportent pf_export = {
	.e_hostname = "*",
	.e_path     = _PATH_PSEUDO_ROOT,
	.m_path = _PATH_PSEUDO_ROOT,
	.e_flags = NFSEXP_READONLY | NFSEXP_ROOTSQUASH
			| NFSEXP_NOSUBTREECHECK | NFSEXP_FSID
			| NFSEXP_CROSSMOUNT,
	.e_anonuid = 65534,
	.e_anongid = 65534,
	.e_squids = NULL,
	.e_nsquids = 0,
	.e_sqgids = NULL,
	.e_nsqgids = 0,
	.e_fsid = 0,
	.e_mountpoint = NULL,
};
static struct pseudo_ent {
	struct exportent *export;
	char *fh;
} pseudo_export = {NULL, NULL};

int
v4root_mount(struct mountargs *args)
{
	int status;

	status = mount (args->spec, args->node, args->type, args->flags, args->data);
	if (status < 0) {
		snprintf(errbuf, BUFSIZ, "mounting %s failed: ",  args->node);
		syserror(errbuf);
	}

	return status;
}

int
v4root_umount(struct mountargs *args)
{
	int status;

	if (args->flags)
		status = umount2(args->node, args->flags);
	else
		status = umount(args->node);
	if (status < 0) {
		snprintf(errbuf, BUFSIZ, "unmounting %s failed: ",  args->node);
		syserror(errbuf);
	}

	return status;
}

/*
 * Destroy the pseudo root
 */
void
v4root_destroy()
{
	if (access(_PATH_PSEUDO_ROOT, F_OK) < 0)
		return;

	pseudo_export.export = NULL;
	pseudo_export.fh = NULL;
	lazy_umount(_PATH_PSEUDO_ROOT);
	exec_rmdir(_PATH_PSEUDO_ROOT);
}

/*
 * Create the pseudo root directory
 */
static int
v4root_mkroot()
{
	struct stat sb;

	if (stat(_PATH_PSEUDO_ROOT, &sb) < 0) {
		if (errno != ENOENT) {
			syserror(_PATH_PSEUDO_ROOT);
			return 1;
		}
		if (mkdir(_PATH_PSEUDO_ROOT, 0755) < 0) {
			syserror("Unable to create " _PATH_PSEUDO_ROOT);
			return 1;
		}
	} else if (!S_ISDIR(sb.st_mode)) {
		errno =  ENOTDIR;
		syserror(_PATH_PSEUDO_ROOT);
		return 1;
	}

	return 0;
}
/*
 * Create the pseudo tree by running through
 * the exports and bind mounting them to 
 * directories in the tree
 */
void
v4root_create()
{
	nfs_export	*exp, *nxt;
	struct mounts_t *m1, *m2;
	char path[BUFSIZ];
	int		i;

	if (!v4root_needed)
		return;

	if (v4root_mkroot())
		return;

	if (!is_mountpoint(_PATH_PSEUDO_ROOT))
		tmpfs_mount(_PATH_PSEUDO_ROOT);

	/*
	 * To build the mount tree correctly, we
	 * mount shortest path to longest path.
	 * So run through exports sorting the paths
	 * by shortest to longest into a queue
	 */
	for (i = 0; i < MCL_MAXTYPES; i++) 
		for (exp = exportlist[i]; exp; exp = nxt) {
			nxt = exp->m_next;

			m1 = (struct mounts_t *)malloc(sizeof(struct mounts_t));
			m1->exp = exp;
			QUE_SORT(m1);
		}

	/*
	 * Now run through the sorted queue creating the
	 * bind mounts as well as freeing the que elements.
	 */
	m1 = TAILQ_FIRST(&head);
	while (m1 != NULL) {
		exp = m1->exp;

		snprintf(path, BUFSIZ, "%s/%s", _PATH_PSEUDO_ROOT, 
			exp->m_export.e_path);
		exec_mkpath(path);
		bind_mount(exp->m_export.e_path, path);
		
		m2 =  TAILQ_NEXT(m1, list);
		free(m1);
		m1 = m2;
	}
	pseudo_export.export = &pf_export;
}

void
v4root_umountall()
{
	v4root_destroy();
}
struct exportent *
v4root_chkroot(int fsidtype, unsigned int fsidnum, char *fhuuid)
{
	struct exportent *pseudo_root = NULL;

	if (pseudo_export.export == NULL)
		return NULL;

	switch(fsidtype) {
		case FSID_NUM:
			if (fsidnum == 0)
				pseudo_root = pseudo_export.export;
			break;
	}

	return pseudo_root;
}
char *
v4root_maproot(char *path)
{
	char *mpath;

	if (pseudo_export.export == NULL)
		return NULL;

	if (strstr(path, _PATH_PSEUDO_ROOT) == NULL)
		return NULL;

	if (strcmp(path, _PATH_PSEUDO_ROOT) != NULL) 
		mpath = strdup(path + strlen(_PATH_PSEUDO_ROOT));
	else
		mpath = strdup(_PATH_PSEUDO_ROOT);

	return mpath;
}
struct exportent *
v4root_export(char *path)
{
	if (pseudo_export.export == NULL)
		return NULL;

	if (strcmp(path, _PATH_PSEUDO_ROOT) != NULL) 
		return NULL;

	return pseudo_export.export;
}
