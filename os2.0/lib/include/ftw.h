#ifndef ftw_h
#define ftw_h
#endif

/*
 * File Tree Walk ---
 *
 * Definitions for the "ftw()" library function.
 *
 * FTW_F   - Entry is a file.
 * FTW_D   - Entry is a searchable, readable directory.
 * FTW_DNR - Entry is a non-searchable and/or non-readable directory.
 * FTW_NS  - No "stat" operation could be performed on this entry.
 */

#define FTW_F   1
#define FTW_D   2
#define FTW_DNR 3
#define FTW_NS  4

int ftw();
