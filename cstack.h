/* $Id: cstack.h,v 1.5 2012/06/10 07:00:23 mit-sato Exp $ */

#include <sys/types.h>
#include <setjmp.h>
#include "config.h"
#include "types.h"

#ifndef __CSTACK__
#define __CSTACK__

void init_cstack();
int  cstack_get(char *memo);
void cstack_release(int);
Toy_Type* cstack_list();
void* cstack_get_start_addr(int);
void* cstack_get_end_addr(int);
int  cstack_get_size();
int  cstack_get_coroid();
void cstack_set_jmpbuff(int slot, sigjmp_buf *buff);
int  cstack_enter(int new_slot);
void cstack_leave(int old_slot);
void cstack_protect(int slot);
void cstack_unprotect(int slot);
void cstack_return();
int  cstack_isalive(int slot);

#define SS_INVAL	(-1)
#define SS_FREE		(0)
#define SS_USE		(1)

#endif /* __CSTACK__ */

#ifdef __CSTACK_DEF__

static struct _cstack {
    int number_of_slot;
    int slot_size;
    struct _stack_slot {
	int state;
	__PTRDIFF_TYPE__ *start_addr;
	__PTRDIFF_TYPE__ *barrier_addr;
	__PTRDIFF_TYPE__ *safe_addr;
	__PTRDIFF_TYPE__ *end_addr;
	int jmp_buff_enable;
	sigjmp_buf jmp_buff;
	char *memo;
    } stack_slot[STACK_SLOT_MAX];
} CStack;

volatile static int Current_coroutine;
volatile sig_atomic_t CStack_in_baria;

#endif /* __CSTACK_DEF__ */
