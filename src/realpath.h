#ifndef _REAL_PATH_H
#define _REAL_PATH_H
/* a safer implementation of realpath. */
extern char *private_realpath(const char *path, char *resolved_path, int m);

#ifndef PATH_MAX
#define PATH_MAX 8192
#endif

/* And a macro to replace current calls, assuming that the buffer
   size is PATH_MAX
*/

#ifdef realpath
#undef realpath
#endif
#define realpath(a,b) private_realpath(a,b,PATH_MAX)


#endif
