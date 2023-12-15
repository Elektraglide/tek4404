/*
     This describes the data structure used to
     perform non-local "goto"s using setjmp()
     and longjmp()
*/

#define setjmp_h

struct _env
{
     unsigned  _d1_jmp;       /* d1 (may go away on TSCcc) */
     unsigned  _d2_jmp;       /* d2 (may go away on TSCcc) */
     unsigned  _d3_jmp;       /* d3 */
     unsigned  _d4_jmp;       /* d4 */
     unsigned  _d5_jmp;       /* d5 */
     unsigned  _d6_jmp;       /* d6 */
     unsigned  _d7_jmp;       /* d7 */
     unsigned  _a0_jmp;       /* a0 (may go away on TSCcc) */
     unsigned  _a1_jmp;       /* a1 (may go away on TSCcc) */
     unsigned  _a2_jmp;       /* a2 */
     unsigned  _a3_jmp;       /* a3 */
     unsigned  _a4_jmp;       /* a4 */
     unsigned  _a5_jmp;       /* a5 */
     unsigned  _a6_jmp;       /* a6 (backlink) */
     unsigned  _sp_jmp;       /* a7 (stack pointer) */
     unsigned  _pc_jmp;       /* Program counter [at setjmp()] */
};

typedef   unsigned  jmp_buf[sizeof(struct _env)/sizeof(unsigned)];
