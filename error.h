/* $Id: error.h,v 1.14 2011/10/22 12:28:52 mit-sato Exp $ */

#ifndef __ERROR__
#define __ERROR__

#define	TE_INTERNAL		"ErrInternal"
#define	TE_NOMEMORY		"ErrNoMemory"
#define TE_PARSESTRING		"WarnParseString"
#define TE_PARSECLOSE		"WarnParseClose"
#define TE_PARSEBADCHAR		"ErrParseClose"
#define TE_NONAMEARG		"ErrNoGivenNamedArg"
#define TE_NOVAR		"ErrNoSuchVariable"
#define TE_NORUNNABLE		"ErrNoRunnableObject"
#define TE_NOFUNC		"_ErrNoFunction"
#define TE_NOOBJECT		"ErrNoTypeObject"
#define TE_FEWARGS		"ErrFewArg"
#define TE_MANYARGS		"ErrManyArg"
#define TE_NOSPECARGS		"ErrNoSpecifiedArg"
#define TE_SYNTAX		"ErrSyntax"
#define TE_STACKOVERFLOW	"ErrStackOverflow"
#define TE_STACKUNDERFLOW	"ErrStackUnderflow"
#define TE_NOCLASS		"ErrNoClass"
#define TE_NOMETHOD		"ErrNoMethod"
#define TE_NODELEGATE		"ErrNoDelegate"
#define TE_TYPE			"ErrBadType"
#define TE_BADMETHOD		"ErrBadMethod"
#define TE_FILENOTOPEN		"ErrFileNotOpen"
#define TE_FILEACCESS		"ErrFileAccess"
#define TE_ARRAYBOUNDARY	"ErrArrayBoundary"
#define TE_SYSCALL		"ErrSysCall"
#define TE_ALIAS		"ErrLinkAlias"
#define TE_REGEX		"ErrRegex"
#define TE_ZERODIV		"ErrZeroDivide"
#define TE_ALREADYDEF		"ErrAlreadyExists"
#define TE_BADBASE		"ErrBadStackBase"
#define TE_BADBINDSPEC		"ErrBadBindSpec"
#define TE_NOTIMPLIMENT		"ErrNotImpliment"
#define TE_NOHOST		"ErrNoFoundHost"
#define TE_COROUTINE		"ErrCreateCoroutine"
#define TE_COOUTOFLIFE		"ErrCoroutineOutOfLife"
#define TE_NOCOROUTINE		"ErrNotCoroutine"
#define TE_NOSLOT		"ErrNoStackSlot"
#define TE_IOAGAIN		"ErrIOAgain"

#endif /* __ERROR__ */
