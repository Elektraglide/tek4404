//
//  fdset.h
//  tek4404_utilities
//
//  Created by Adam Billyard on 26/09/2023.
//

#ifndef fdset_h
#define fdset_h

/* by code inspection, Uniflex supports max 32 fds over an array of bits */
typedef struct _fdset {int fdmask[8];} fd_set;
#define FD_SET(A,SET)	(SET)->fdmask[(A)/32] |= (1<<((A)&31))
#define FD_CLR(A,SET)	(SET)->fdmask[(A)/32] &= ~(1<<((A)&31))
#define FD_ZERO(SET)	memset(SET, 0, sizeof(fd_set))
#define FD_ISSET(A,SET)	((SET)->fdmask[(A)/32] & (1<<((A)&31)))


#endif /* fdset_h */
