/*
 * support/include/v4root.h
 *
 * General support functions for Dynamic pseudo root functionality 
 *
 */

#ifndef V4ROOT_H
#define V4ROOT_H

extern int v4root_needed, v4root_check;

extern struct exportent *v4root_chkroot(int , unsigned int , char *);
extern char *v4root_maproot(char *);
extern struct exportent *v4root_export(char *);

#endif /* V4ROOT_H */
