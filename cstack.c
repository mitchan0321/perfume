/* $Id: cstack.c,v 1.7 2012/06/10 07:00:23 mit-sato Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <sys/mman.h>
#include "toy.h"

#define __CSTACK_DEF__
#include "cstack.h"
#include "types.h"

static void sig_cstack(int flag, siginfo_t* siginfo, void* ptr);
static void sig_cstack_running_handler(int flag, siginfo_t* siginfo, void* ptr);
static int alloc_slot(int slot);
static jmp_buf jmp_env;

void
init_cstack() {
    int i, fin;
    stack_t ss;
    struct sigaction newsig, oldsig;
    
    CStack.slot_size = STACK_SLOT_SIZE * sizeof(__PTRDIFF_TYPE__);
    for (i = 0 ; i < STACK_SLOT_MAX ; i++) {
	CStack.stack_slot[i].state = SS_INVAL;
	CStack.stack_slot[i].start_addr = 0;
	CStack.stack_slot[i].barrier_addr = 0;
	CStack.stack_slot[i].safe_addr = 0;
	CStack.stack_slot[i].end_addr = 0;
	CStack.stack_slot[i].jmp_buff_enable = 0;
	memset(CStack.stack_slot[i].jmp_buff, 0, sizeof(jmp_buf));
    }

    ss.ss_sp = GC_MALLOC(SIGASTKSZ);
    if (NULL == ss.ss_sp) {
	fprintf(stderr, "Can't alloc altinative stack(1).\n");
	exit(1);
    }
    ss.ss_size = SIGASTKSZ;
    ss.ss_flags = 0;
    if (-1 == sigaltstack(&ss, NULL)) {
	fprintf(stderr, "Can't set altinative stack(1).\n");
	exit(1);
    }

    if (-1 == sigaction(SIGSEGV, NULL, &oldsig)) {
	fprintf(stderr, "Can't save sigaction(1).\n");
	exit(1);
    }

    sigemptyset(&newsig.sa_mask);
    sigaddset(&newsig.sa_mask, SIGSEGV);
    newsig.sa_sigaction = sig_cstack;
    newsig.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
    if (-1 == sigaction(SIGSEGV, &newsig, NULL)) {
	fprintf(stderr, "Can't set sigaction(1).\n");
	exit(1);
    }

    fin = alloc_slot(0);
    CStack.number_of_slot = fin;

    if (-1 == sigaction(SIGSEGV, &oldsig, NULL)) {
	fprintf(stderr, "Can't restore sigaction.\n");
	exit(1);
    }

    /* for main interp stack to use */
    CStack.stack_slot[0].state = SS_USE;

    /* init current coroutine */
    Current_coroutine = 0;

    /* running signal install */
    ss.ss_sp = GC_MALLOC(SIGASTKSZ);
    if (NULL == ss.ss_sp) {
	fprintf(stderr, "Can't alloc altinative stack(2).\n");
	exit(1);
    }
    ss.ss_size = SIGASTKSZ;
    ss.ss_flags = 0;
    if (-1 == sigaltstack(&ss, NULL)) {
	fprintf(stderr, "Can't set altinative stack(2).\n");
	exit(1);
    }

    if (-1 == sigaction(SIGSEGV, NULL, &oldsig)) {
	fprintf(stderr, "Can't save sigaction(2).\n");
	exit(1);
    }

    sigemptyset(&newsig.sa_mask);
    sigaddset(&newsig.sa_mask, SIGSEGV);
    newsig.sa_sigaction = sig_cstack_running_handler;
    newsig.sa_flags = SA_SIGINFO | SA_RESTART | SA_ONSTACK;
    if (-1 == sigaction(SIGSEGV, &newsig, NULL)) {
	fprintf(stderr, "Can't set sigaction(2).\n");
	exit(1);
    }
}

static int
alloc_slot(int slot) {
    volatile int i;
    volatile __PTRDIFF_TYPE__ *stack_frame;

    stack_frame = alloca(STACK_SLOT_SIZE * sizeof(__PTRDIFF_TYPE__));

    if (0 == setjmp(jmp_env)) {
	for (i = (STACK_SLOT_SIZE - 1) ; i >= 0 ; i--) {
	    stack_frame[i] = 0;
	}
    } else {
	return slot;
    }

    CStack.stack_slot[slot].state = SS_FREE;
    CStack.stack_slot[slot].start_addr = (__PTRDIFF_TYPE__*)stack_frame;
    CStack.stack_slot[slot].end_addr = (__PTRDIFF_TYPE__*)
	((void*)stack_frame + (STACK_SLOT_SIZE * sizeof(__PTRDIFF_TYPE__)));

    /* set memory protect barrier */
    if ((__PTRDIFF_TYPE__)(((void*)stack_frame + MP_SPARE)) & MP_ALIGN) {
	CStack.stack_slot[slot].barrier_addr = (__PTRDIFF_TYPE__*)
	    (((__PTRDIFF_TYPE__)((void*)stack_frame
				 + MP_SPARE + MP_PAGESIZE)) & ~MP_ALIGN);
    } else {
	CStack.stack_slot[slot].barrier_addr = (__PTRDIFF_TYPE__*)
	    ((void*)(stack_frame) + MP_SPARE);
    }
    CStack.stack_slot[slot].safe_addr = (__PTRDIFF_TYPE__*)
	((void*)(CStack.stack_slot[slot].barrier_addr) + MP_PAGESIZE);
    mprotect((void*)CStack.stack_slot[slot].barrier_addr, MP_PAGESIZE, PROT_READ);
    
    /* indicate garbage collection address block to BoehmGC */
    GC_add_roots((void*)CStack.stack_slot[slot].safe_addr,
		 (void*)CStack.stack_slot[slot].end_addr);

    slot ++;
    if (slot > STACK_SLOT_MAX) return (slot-1);
    return alloc_slot(slot);
}

static void
sig_cstack(int flag, siginfo_t* siginfo, void* ptr) {
    longjmp(jmp_env, 1);
}

static void
sig_cstack_running_handler(int flag, siginfo_t* siginfo, void* ptr) {
    if (CStack.stack_slot[Current_coroutine].jmp_buff_enable) {
	longjmp(CStack.stack_slot[Current_coroutine].jmp_buff, 1);
    }

    fprintf(stderr, "Overflow handler is not installed, at %d.\n", Current_coroutine);
    exit(1);
}

int
cstack_get() {
    int i;

    for (i = 0 ; i < CStack.number_of_slot ; i++) {
	if (SS_FREE == CStack.stack_slot[i].state) {
	    CStack.stack_slot[i].state = SS_USE;
	    memset((void*)CStack.stack_slot[i].safe_addr, 0,
		   CStack.stack_slot[i].end_addr - CStack.stack_slot[i].safe_addr);
	    return i;
	}
    }
    
    return -1;
}

void
cstack_release(int slot_id) {
    if ((slot_id >= CStack.number_of_slot) ||
	(slot_id < 1)) {
	return;
    }
    
    CStack.stack_slot[slot_id].state = SS_FREE;
    CStack.stack_slot[slot_id].jmp_buff_enable = 0;
}

Toy_Type*
cstack_list() {
    int i;
    Toy_Type *result, *slist, *elist;

    result = new_list(NULL);
    list_append(result, new_integer_si(CStack.number_of_slot));

    slist = new_list(NULL);
    for (i = 0 ; i < CStack.number_of_slot ; i++) {
	elist = new_list(NULL);
	switch (CStack.stack_slot[i].state) {
	case SS_FREE:
	    list_append(elist, new_symbol("FREE"));
	    list_append(elist,
			new_integer_si((__PTRDIFF_TYPE__)
				       (__PTRDIFF_TYPE__)
				       CStack.stack_slot[i].start_addr));
	    list_append(elist,
			new_integer_si((__PTRDIFF_TYPE__)
				       (__PTRDIFF_TYPE__)
				       CStack.stack_slot[i].barrier_addr));
	    list_append(elist,
			new_integer_si((__PTRDIFF_TYPE__)
				       (__PTRDIFF_TYPE__)
				       CStack.stack_slot[i].safe_addr));
	    list_append(elist,
			new_integer_si((__PTRDIFF_TYPE__)
				       (__PTRDIFF_TYPE__)
				       CStack.stack_slot[i].end_addr));
	    list_append(elist,
			new_integer_si((__PTRDIFF_TYPE__)
				       (__PTRDIFF_TYPE__)
				       CStack.stack_slot[i].jmp_buff_enable));
	    break;
	case SS_USE:
	    list_append(elist, new_symbol("USE"));
	    list_append(elist,
			new_integer_si((__PTRDIFF_TYPE__)
				       (__PTRDIFF_TYPE__)
				       CStack.stack_slot[i].start_addr));
	    list_append(elist,
			new_integer_si((__PTRDIFF_TYPE__)
				       (__PTRDIFF_TYPE__)
				       CStack.stack_slot[i].barrier_addr));
	    list_append(elist,
			new_integer_si((__PTRDIFF_TYPE__)
				       (__PTRDIFF_TYPE__)
				       CStack.stack_slot[i].safe_addr));
	    list_append(elist,
			new_integer_si((__PTRDIFF_TYPE__)
				       (__PTRDIFF_TYPE__)
				       CStack.stack_slot[i].end_addr));
	    list_append(elist,
			new_integer_si((__PTRDIFF_TYPE__)
				       (__PTRDIFF_TYPE__)
				       CStack.stack_slot[i].jmp_buff_enable));
	    break;
	}
	list_append(slist, elist);
    }
    list_append(result, slist);

    return result;
}

void*
cstack_get_start_addr(int slot_id) {

    if ((slot_id >= CStack.number_of_slot) ||
	(slot_id < 0)) {
	return 0;
    }

    return (void*)CStack.stack_slot[slot_id].start_addr;
}

void*
cstack_get_end_addr(int slot_id) {

    if ((slot_id >= CStack.number_of_slot) ||
	(slot_id < 0)) {
	return 0;
    }

    return (void*)CStack.stack_slot[slot_id].end_addr;
}

int
cstack_get_size() {
    return CStack.slot_size;
}

void
cstack_set_jmpbuff(int slot, jmp_buf *buff) {
    memcpy(&(CStack.stack_slot[slot].jmp_buff), buff, sizeof(jmp_buf));
    CStack.stack_slot[slot].jmp_buff_enable = 1;
}

int
cstack_enter(int new_slot) {
    int old_slot;

    old_slot = Current_coroutine;
    Current_coroutine = new_slot;

    return old_slot;
}

void
cstack_leave(int old_slot) {
    Current_coroutine = old_slot;
}
