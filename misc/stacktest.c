/* $Id: cstack.c,v 1.7 2012/06/10 07:00:23 mit-sato Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <sys/mman.h>
#include <gc.h>
#include <pthread.h>

/* pthread stack size */
#define MIN_STACK_SIZE		(16*1024*1024)

/* for main co-routine stack size */
#define STACKSIZE		(1024)

/* for children co-routine stack size */
#define CO_STACKSIZE		(1024)

/* maximum number of slot */
// #define STACK_SLOT_MAX		(512)
#define STACK_SLOT_MAX		(256)

/*
 * This number is co-routines native C stack size.
 * Real allocate size is STACK_SLOT_SIZE * sizeof(u_int32_t) (may be 4)
 */
// about 128K bytes
#define STACK_SLOT_SIZE		(32*1024)

// about 256K bytes
// #define STACK_SLOT_SIZE		(64*1024)

// about 512K bytes
// #define STACK_SLOT_SIZE		(128*1024)

/* for SIGSEGV signal handler */
#define SIGASTKSZ		(4*1024)

#define SS_INVAL	(-1)
#define SS_FREE		(0)
#define SS_USE		(1)

/* define mprotect page size */
#define MP_ALIGN		(0x0fff)
#define MP_PAGESIZE		(0x1000)
#define MP_SPARE		(4096)

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
    } stack_slot[STACK_SLOT_MAX];
} CStack;

volatile static int Current_coroutine;
volatile sig_atomic_t CStack_in_baria;

void init_cstack();
int  cstack_get();
void cstack_release(int);
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

static void sig_cstack(int flag, siginfo_t* siginfo, void* ptr);
static void sig_cstack_running_handler(int flag, siginfo_t* siginfo, void* ptr);
static int alloc_slot(int slot);
static sigjmp_buf jmp_env;

int 
main(int argc, char **argv) {
    int i;
    pthread_attr_t attr;
    size_t v;

    if (0 != pthread_attr_init(&attr)) {
	fprintf(stderr, "pthread_attr_init failed.\n");
	exit(1);
    }
    pthread_attr_getstacksize(&attr, &v);
    fprintf(stderr, "getstacksize: %d\n", (int)v);
    if (0 != pthread_attr_setstacksize(&attr, MIN_STACK_SIZE)) {
	fprintf(stderr, "WARNING: pthread_attr_setstacksize failed.\n");
    }
    pthread_attr_getstacksize(&attr, &v);
    fprintf(stderr, "getstacksize: %d\n", (int)v);

    if (0 != pthread_attr_setstack(&attr, &i, MIN_STACK_SIZE)) {
	fprintf(stderr, "WARNING: pthread_attr_setstack failed.\n");
    }

    init_cstack();
    for (i=0; i<CStack.number_of_slot; i++) {
	fprintf(stderr, "%03d: %016lx %016lx %016lx %016lx\n",
		i,
		(long int)CStack.stack_slot[i].start_addr,
		(long int)CStack.stack_slot[i].barrier_addr,
		(long int)CStack.stack_slot[i].safe_addr,
		(long int)CStack.stack_slot[i].end_addr);
    }
}

void
init_cstack() {
    int i, fin;
    stack_t ss;
    struct sigaction newsig, oldsig;
    char *p;

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

    ss.ss_sp = malloc(SIGASTKSZ);
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
    CStack_in_baria = 0;

    /* running signal install */
    ss.ss_sp = malloc(SIGASTKSZ);
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
//    volatile __PTRDIFF_TYPE__ *stack_frame;
    __PTRDIFF_TYPE__ stack_frame[STACK_SLOT_SIZE];

//    stack_frame = alloca(STACK_SLOT_SIZE * sizeof(__PTRDIFF_TYPE__));
//    fprintf(stderr, "%02d: %016lx\n", slot, (long int)stack_frame);

    if (0 == sigsetjmp(jmp_env, 1)) {
	memset((void*)stack_frame, 0, STACK_SLOT_SIZE * sizeof(__PTRDIFF_TYPE__));
#if 0
	for (i = (STACK_SLOT_SIZE - 1) ; i >= 0 ; i--) {
	    stack_frame[i] = 0;
	}
#endif
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
    fprintf(stderr, "si_reason: %d\n", siginfo->_reason._fault._trapno);
    if (CStack_in_baria) {
	fprintf(stderr, "SOVF Double fault detect.\n");
	exit(1);
    }
    CStack_in_baria = 1;
    if (CStack.stack_slot[Current_coroutine].jmp_buff_enable) {
	cstack_unprotect(Current_coroutine);
	sigreturn(ptr);
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
