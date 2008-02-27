#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "xlog.h"

/*
 * Log system errors or error messages.
 */
void 
syserror(char *message)
{
	if (errno)
		xlog(L_ERROR, "%s: errno %d (%s)", message, errno, strerror(errno));
	else
		xlog(L_ERROR, "Error: %s", message);
}
/*
 * Log the the given argv[] if debugging is on.
 */
static void
show_argv(char *msg, char *const argv[])
{
	char buf[BUFSIZ];
	int i, cc=0;

	if (!xlog_enabled(D_CALL))
		return;

	if (msg) {
		sprintf(buf+cc, "%s ", msg);
		cc = strlen(buf);
	}
	for (i=0; argv[i] != NULL && cc < BUFSIZ; i++) {
		if ((cc + strlen(argv[i])) >= BUFSIZ)
			break;
		sprintf(buf+cc, "%s ", argv[i]);
		cc = strlen(buf);
	}
	xlog(D_CALL, "%s", buf);
}
/*
 *  Execute the given command using the 
 *  given argv as the arguments.
 */
int
execute_cmd(const char *cmd,  char *const myargv[])
{
	int pfd[2], status, cc;
	pid_t pid;
	char buf[BUFSIZ], *ch;

	show_argv("executing:", myargv);

	if (pipe(pfd) < 0) {
		syserror("pipe() failed");
		return errno;
	}
	switch((pid = fork())) {
	case -1: 
		syserror("fork() failed");
		break;
	case 0:  /* child */
		close(pfd[0]);
		if (dup2(pfd[1], STDOUT_FILENO) < 0)
			perror("dup2(STDOUT_FILENO)");
		if (dup2(pfd[1], STDERR_FILENO) < 0)
			perror("dup2(STDERR_FILENO)");
		close(pfd[1]);
		execvp(cmd, myargv);
		perror("execv");
		_exit(255);
		break;
	default: /* parent */
		status = 0;
		close(pfd[1]);
		if (waitpid(pid, &status, 0) < 0 && errno != ECHILD) {
			syserror("waitpid() failed");
		} else if (WIFEXITED(status)) {
			cc = read(pfd[0], buf, BUFSIZ);
			if (cc > 0) { 
				if ((ch = strrchr(buf, '\n')) != NULL)
					*ch = '\0';
				errno = 0;
				syserror(buf);
			}
			close(pfd[0]);
		}
		break;
	}
	return errno;
}
