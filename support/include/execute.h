/*
 * support/include/execute.h
 *
 * Routines that will execute random shell commands.
 *
 */

#ifndef EXECUTE_H
#define EXECUTE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _RMDIR_CMD
#define _RMDIR_CMD "/bin/rmdir"
#endif

#ifndef _MKDIR_CMD
#define _MKDIR_CMD "/bin/mkdir"
#endif

inline int
exec_rmdir(char *dir)
{
	char *const myargv[] = {_RMDIR_CMD, dir, NULL};

	return execute_cmd(_RMDIR_CMD, myargv);
}
inline int
exec_mkpath(char *dir)
{
	char *const myargv[] = {_MKDIR_CMD, "-p", dir, NULL};

	return execute_cmd(_MKDIR_CMD, myargv);
}

#endif /* EXECUTE_H */
