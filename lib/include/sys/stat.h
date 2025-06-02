/**
 * The SakuraOS Standard Library
 * Copyright 2025 Adam Judge
 */

#ifndef _SYS_STAT_H
#define _SYS_STAT_H

/*
 * File mode values.
 */
#define S_IFMT    0170000  /* File type mask */
#define S_IFSOCK  0140000  /* Socket */
#define S_IFLNK   0120000  /* Symbolic link */
#define S_IFREG   0100000  /* Regular file */
#define S_IFBLK   0060000  /* Block device */
#define S_IFDIR   0040000  /* Directory */
#define S_IFCHR   0020000  /* Character device */
#define S_IFIFO   0010000  /* Pipe */
#define S_ISUID   0004000  /* Set user ID */
#define S_ISGID   0002000  /* Set group ID */
#define S_IFVTX   0001000  /* Sticky bit */
#define S_IRUSR   0000400  /* User readable */
#define S_IWUSR   0000200  /* User writable */
#define S_IXUSR   0000100  /* User executable */
#define S_IRWXU   0000700  /* User R/W/X */
#define S_IRGRP   0000040  /* Group readable */
#define S_IWGRP   0000020  /* Group writable */
#define S_IXGRP   0000010  /* Group executable */
#define S_IRWXG   0000070  /* Group R/W/X */
#define S_IROTH   0000004  /* Other readable */
#define S_IWOTH   0000002  /* Other writable */
#define S_IXOTH   0000001  /* Other executable */
#define S_IRWXO   0000007  /* Other R/W/X */

/*
 * File type test macros.
 */
#define S_ISBLK(m) (m & S_IFBLK)
#define S_ISCHR(m) (m & S_IFCHR)
#define S_ISDIR(m) (m & S_IFDIR)
#define S_ISFIFO(m) (m & S_IFIFO)
#define S_ISREG(m) (m & S_IFREG)
#define S_ISLNK(m) (m & S_IFLNK)

#endif
