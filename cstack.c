#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <sys/mman.h>
#include "toy.h"

#define __CSTACK_DEF__
#include "cstack.h"
#include "types.h"

static void sig_cstack(int flag, siginfo_t* siginfo, void* ptr);
static void sig_cstack_running_handler(int flag, siginfo_t* siginfo, void* ptr);
static int alloc_slot(int slot);
static sigjmp_buf jmp_env;

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
	CStack.stack_slot[i].memo = 0;
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
    CStack.stack_slot[0].memo = L"(main stack)";

    /* main stack add root to GC */
    GC_add_roots((void*)CStack.stack_slot[0].safe_addr,
		 (void*)CStack.stack_slot[0].end_addr);

    /* init current coroutine */
    Current_coroutine = 0;
    CStack_in_baria = 0;

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
    __PTRDIFF_TYPE__ stack_frame[STACK_SLOT_SIZE];

    if (0 == sigsetjmp(jmp_env, 1)) {
	/* memset((void*)stack_frame, 0, STACK_SLOT_SIZE * sizeof(__PTRDIFF_TYPE__)); */
	memset((void*)stack_frame, 0, MP_PAGESIZE);
    } else {
	if (slot <= 1) {
	    fprintf(stderr, "Can\'t allocate stack slot, going to exit.\n");
	    exit(1);
	}
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

    cstack_protect(slot);

    /* indicate garbage collection address block to BoehmGC */
/*
    GC_add_roots((void*)CStack.stack_slot[slot].safe_addr,
		 (void*)CStack.stack_slot[slot].end_addr);
*/
    slot ++;
    if (slot > STACK_SLOT_MAX) return (slot-1);
    return alloc_slot(slot);
}

void
cstack_protect(int slot) {
    mprotect((void*)CStack.stack_slot[slot].barrier_addr, MP_PAGESIZE, PROT_READ);
}

void
cstack_unprotect(int slot) {
    mprotect((void*)CStack.stack_slot[slot].barrier_addr, MP_PAGESIZE, PROT_READ | PROT_WRITE);
}

static void
sig_cstack(int flag, siginfo_t* siginfo, void* ptr) {
/*
    fprintf(stderr, "sig: %d\n", siginfo->si_signo);
    if (siginfo->si_code == SEGV_MAPERR) {
	fprintf(stderr, "MAPERROR, exit\n");
	exit(1);
    }
*/
    siglongjmp(jmp_env, 1);
}

static void
sig_cstack_running_handler(int flag, siginfo_t* siginfo, void* ptr) {
    fprintf(stderr, "*** receive SIGSEGV by sig_cstack_running_handler.\n");
    fprintf(stderr, "flag: %d\n", flag);
    fprintf(stderr, "si_signo: %d\n", siginfo->si_signo);
    fprintf(stderr, "si_errno: %d\n", siginfo->si_errno);
    fprintf(stderr, "si_code: %d\n", siginfo->si_code);
    fprintf(stderr, "si_pid: %d\n", siginfo->si_pid);
    fprintf(stderr, "si_uid: %d\n", siginfo->si_uid);
    fprintf(stderr, "si_status: %d\n", siginfo->si_status);
    fprintf(stderr, "si_addr: %016lx\n", (long int)siginfo->si_addr);
    fprintf(stderr, "si_value: %d\n", siginfo->si_value.sival_int);
#ifdef __FreeBSD__
    fprintf(stderr, "si_reason: %d\n", siginfo->_reason._fault._trapno);
#endif
    if (CStack_in_baria) {
	fprintf(stderr, "SOVF Double fault detect.\n");
	exit(1);
    }
    CStack_in_baria = 1;
    if (CStack.stack_slot[Current_coroutine].jmp_buff_enable) {
	cstack_unprotect(Current_coroutine);
	//sigreturn(ptr);
	return;
#if 0
	siglongjmp(CStack.stack_slot[Current_coroutine].jmp_buff, 1);
#endif
    }

    fprintf(stderr, "Overflow handler is not installed, at %d.\n", Current_coroutine);
    exit(1);
}

void cstack_return() {
    CStack_in_baria = 0;
    cstack_protect(Current_coroutine);
    siglongjmp(CStack.stack_slot[Current_coroutine].jmp_buff, 1);
}

static void
cstack_clear(int slot_id) {
    memset((void*)CStack.stack_slot[slot_id].barrier_addr, 0,
	   CStack.stack_slot[slot_id].end_addr - CStack.stack_slot[slot_id].barrier_addr);
}

static void
cstack_clear_all(int slot_id) {
    memset((void*)CStack.stack_slot[slot_id].start_addr, 0,
	   CStack.stack_slot[slot_id].end_addr - CStack.stack_slot[slot_id].start_addr);
}

int
cstack_get(wchar_t *memo) {
    int i;

    for (i = 0 ; i < CStack.number_of_slot ; i++) {
	if (SS_FREE == CStack.stack_slot[i].state) {
	    CStack.stack_slot[i].state = SS_USE;
	    CStack.stack_slot[i].memo = memo;
	    cstack_unprotect(i);
	    cstack_clear_all(i);
	    cstack_protect(i);    

	    /* indicate garbage collection address block to BoehmGC */
	    GC_add_roots((void*)CStack.stack_slot[i].safe_addr,
			 (void*)CStack.stack_slot[i].end_addr);
	    
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
    
    CStack.stack_slot[slot_id].state = SS_PEND;
    CStack.stack_slot[slot_id].jmp_buff_enable = 0;
    CStack.stack_slot[slot_id].memo = 0;

    /* indicate garbage collection address block to BoehmGC */
    GC_remove_roots((void*)CStack.stack_slot[slot_id].safe_addr,
		    (void*)CStack.stack_slot[slot_id].end_addr);
}

void cstack_release_clear(int slot_id) {
    if ((slot_id >= CStack.number_of_slot) ||
	(slot_id < 1)) {
	return;
    }
    
    cstack_release(slot_id);
    CStack.stack_slot[slot_id].state = SS_FREE;
    cstack_unprotect(slot_id);
    cstack_clear(slot_id);
    cstack_protect(slot_id);
    
    /* indicate garbage collection address block to BoehmGC */
    GC_remove_roots((void*)CStack.stack_slot[slot_id].safe_addr,
		    (void*)CStack.stack_slot[slot_id].end_addr);
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
	    list_append(elist, new_integer_si(i));
	    list_append(elist, new_symbol(L"FREE"));
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
	    list_append(elist, new_string_str(L""));

	    break;

	case SS_USE:
	    list_append(elist, new_integer_si(i));
	    list_append(elist, new_symbol(L"USE"));
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
	    list_append(elist, new_string_str(CStack.stack_slot[i].memo));

	    break;

	case SS_PEND:
	    list_append(elist, new_integer_si(i));
	    list_append(elist, new_symbol(L"PEND"));
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
	    list_append(elist, new_string_str(CStack.stack_slot[i].memo));

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

int
cstack_get_coroid() {
    return Current_coroutine;
}

void
cstack_set_jmpbuff(int slot, sigjmp_buf *buff) {
    memcpy(&(CStack.stack_slot[slot].jmp_buff), buff, sizeof(jmp_buf));
    CStack.stack_slot[slot].jmp_buff_enable = 1;
}

int
cstack_enter(int new_slot) {
    int old_slot;

    old_slot = Current_coroutine;
    Current_coroutine = new_slot;
    CStack_in_baria = 0;

    return old_slot;
}

void
cstack_leave(int old_slot) {
    Current_coroutine = old_slot;
}

int  cstack_isalive(int slot) {
    if (SS_USE == CStack.stack_slot[slot].state) {
	return 1;
    } else {
	return 0;
    }
}

void*
cstack_get_safe_addr() {
    return (void*)CStack.stack_slot[Current_coroutine].safe_addr;
}
