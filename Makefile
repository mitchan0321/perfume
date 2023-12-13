###
### Set options initial value
###
OPTIONS = -gdwarf-4
# Note: if you build external libraries, you need specify CC flag before ./configure like this: 'export CFLAGS=-gdwarf-4'

OPTLIBS =
OPTLIBS2 =

INCLUDE	= -I/usr/local/include -I.

HDRS	= bulk.h binbulk.h cell.h array.h error.h hash.h interp.h parser.h \
	  toy.h types.h config.h global.h cstack.h util.h encoding.h fclib.h
SRCS	= bulk.c binbulk.c cell.c array.c hash.c list.c parser.c types.c \
	  eval.c interp.c commands.c methods.c global.c cstack.c util.c \
	  encoding.c encoding-table.c fclib.c
OBJS	= bulk.o binbulk.o cell.o array.o hash.o list.o parser.o types.o \
	  eval.o interp.o commands.o methods.o global.o cstack.o util.o \
	  encoding.o encoding-table.o fclib.o

######################################################################################
###
###   FIX ME
###
PREFIX		= /usr/local
ifeq ($(shell basename `which clang`X),clangX)
  CC		= clang
else
  CC		= cc
endif
PKG_TMP		= $(HOME)/tmp
PKG_DIR		= $(PKG_TMP)/pmacs-install
PKG_EXTLIB_DIR	= /usr/local/lib
PKG_TAR_NAME	= pmacs-install.tar.gz
ifeq ($(shell uname),Linux)
  MAKE		= make
else
  ifeq ($(shell uname),FreeBSD)
    MAKE	= gmake
  else
    MAKE	= make
  endif
endif

###
### SET ENABLE FEATURE OPTIONS
###

### Enable curses library (for use in perfume intepriter execute curs-* commands)
### If you specified NCURSES=yes, ncurses commands are included.
NCURSES = yes

### If you specified EVAL_STAT=yes, eval-stat command is included.
EVAL_STAT = no

### If you specified NO_LAZY_CALL=yes, the lazy call mechanism when calling a function is omitted.
NO_LAZY_CALL = no

### If you use to co-routine library 'coru' set yes, if use 'pcl' set otherwise.
CORU_USE = yes

### If you use mouse on pmacs text editor.
MOUSE = yes

ifeq ($(NCURSES),yes)
  OPTIONS	+= -DNCURSES
  SRCS		+= ncurses.c
  OBJS		+= ncurses.o
  ifeq ($(shell uname),Darwin)
    OPTLIBS	+= -lncurses
    OPTLIBS2	+= -lncurses
  else
    OPTLIBS	+= -lncursesw
    OPTLIBS2	+= -lncursesw
  endif
endif

ifeq ($(MOUSE),yes)
  OPTIONS	+= -DMOUSE
endif

ifeq ($(EVAL_STAT),yes)
  OPTIONS	+= -DEVAL_STAT
endif

ifeq ($(NO_LAZY_CALL),yes)
  OPTIONS	+= -DNO_LAZY
endif

ifeq ($(CORU_USE),yes)
  OPTIONS	+= -DCORU_USE
  OPTLIBS       += extlib/coru/coru.a
  OPTLIBS2      += ,extlib/coru/coru.a
else
  OPTLIBS       += -lpcl
  OPTLIBS2      += ,-lpcl
endif

### for product build. (use BoehmGC)

ifeq ($(shell uname),FreeBSD)
	CFLAGS	= -Wall -O2 -c -g $(OPTIONS)
  ifeq ($(CORU_USE),yes)
	INCLUDE	+= -I./extlib/coru
  endif
	LIB	= -static -L/usr/lib -L/lib -L/usr/local/lib -lm -lpthread -lgmp -lgc -lonigmo $(OPTLIBS)
else
	CFLAGS	= -Wall -O2 -c -g $(OPTIONS)
  ifeq ($(CORU_USE),yes)
	INCLUDE	+= -I./extlib/coru
  endif
	LIB	= -L/usr/lib -L/lib -L/usr/local/lib -lm -lpthread -lgmp -lgc -lonigmo $(OPTLIBS)
endif

### for normaly link option (BSD and Linux)
#		  -lm -lpthread -lgmp -lgc -lonigmo $(OPTLIBS)
### for Linux static link options
#		  -static-libgcc -Wl,-Bdynamic,-lc,-ldl,-lm,-lpthread,-lgc,-Bstatic,-lgmp,-lonigmo,$(OPTLIBS2)
### for BSD static link options
#		  -static -lm -lpthread -lgmp -lgc -lonigmo $(OPTLIBS)

### for memory debuging build.
#CFLAGS		= -Wall -c -g -DPROF $(OPTIONS)
#ifeq ($(CORU_USE),yes)
#INCLUDE		+= -I./extlib/coru
#endif
#LIB		= -L/usr/local/lib -lm -lonigmo -lgmp $(OPTLIBS)

### for profiling build.
#CFLAGS		= -Wall -c -g -pg -DPROF $(OPTIONS)
#ifeq ($(CORU_USE),yes)
#INCLUDE		+= -I./extlib/coru
#endif
#LIB		= -pg -L/usr/local/lib -lonigmo -lgmp $(OPTLIBS)

###
###   DONE
###
######################################################################################

all:	extlib/coru/coru.a perfumesh

install:
	if [ ! -d $(PREFIX)/lib/perfume ]; then mkdir $(PREFIX)/lib/perfume; fi
	if [ ! -d $(PREFIX)/lib/perfume/lib ]; then mkdir $(PREFIX)/lib/perfume/lib; fi
	if [ ! -d $(PREFIX)/lib/perfume/lib/pdoc ]; then mkdir $(PREFIX)/lib/perfume/lib/pdoc; fi
	if [ ! -d $(PREFIX)/lib/perfume/lib/Tasklet ]; then mkdir $(PREFIX)/lib/perfume/lib/Tasklet; fi
	install -m 755 perfumesh $(PREFIX)/bin
	install -m 644 setup.prfm $(PREFIX)/lib/perfume
	install -m 644 lib/*.prfm $(PREFIX)/lib/perfume/lib
	install -m 644 lib/*.conf $(PREFIX)/lib/perfume/lib
	install -m 644 lib/*.key $(PREFIX)/lib/perfume/lib
	install -m 644 lib/*.logo $(PREFIX)/lib/perfume/lib
	install -m 644 lib/*.keymap $(PREFIX)/lib/perfume/lib
	install -m 444 lib/pdoc/* $(PREFIX)/lib/perfume/lib/pdoc
	install -m 444 lib/Tasklet/* $(PREFIX)/lib/perfume/lib/Tasklet
	install -m 644 misc/default.fcab $(PREFIX)/lib/perfume/lib
	install -m 755 pkg/pmacs.in $(PREFIX)/bin
	install -m 755 pkg/pmacs-client.in $(PREFIX)/bin
	install -m 755 pkg/pmacs-install.sh $(PREFIX)/pmacs-install.sh
	install -m 644 RELEASE $(PREFIX)/lib/perfume
	(cd $(PREFIX); sh ./pmacs-install.sh; rm -f bin/pmacs.in bin/pmacs-client.in pmacs-install.sh)

ifeq ($(CORU_USE),yes)
extlib/coru/coru.a:	extlib/coru/coru.c extlib/coru/coru.h extlib/coru/coru_platform.c extlib/coru/coru_platform.h extlib/coru/coru_util.h
	(cd extlib/coru; $(MAKE))

perfumesh:	$(OBJS) perfumesh.o extlib/coru/coru.a
	$(CC) $(OBJS) perfumesh.o $(LIB) -o perfumesh

else
perfumesh:	$(OBJS) perfumesh.o
	$(CC) $(OBJS) perfumesh.o $(LIB) -o perfumesh

endif

perfumesh.o:	$(SRCS) $(HDRS) perfumesh.c
	$(CC) $(CFLAGS) $(INCLUDE) perfumesh.c -o perfumesh.o

cell.o:		cell.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) cell.c -o cell.o

array.o:	array.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) array.c -o array.o

list.o:		list.c
	$(CC) $(CFLAGS) $(INCLUDE) list.c -o list.o

hash.o:		hash.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) hash.c -o hash.o

bulk.o:		bulk.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) bulk.c -o bulk.o

binbulk.o:	binbulk.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) binbulk.c -o binbulk.o

types.o:	types.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) types.c -o types.o

parser.o:	parser.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) parser.c -o parser.o

interp.o:	interp.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) interp.c -o interp.o

eval.o:		eval.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) eval.c -o eval.o

commands.o:	commands.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) commands.c -o commands.o

methods.o:	methods.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) methods.c -o methods.o

global.o:	global.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) global.c -o global.o

cstack.o:	cstack.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) cstack.c -o cstack.o

util.o:		util.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) util.c -o util.o

encoding.o:	encoding.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) encoding.c -o encoding.o

encoding-table.o: encoding-table.c $(HDRS)
	rm -f encoding-set-utoj.h encoding-set-jtou.h
	awk -f jisconv.awk < doc/jis0208.txt
	$(CC) $(CFLAGS) -O0 $(INCLUDE) encoding-table.c -o encoding-table.o

fclib.o:	fclib.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) fclib.c -o fclib.o

###
### Compile optional feature
###
ifeq ($(NCURSES),yes)
ncurses.o:	ncurses.c $(HDRS)
	$(CC) $(CFLAGS) $(INCLUDE) ncurses.c -o ncurses.o
endif
###
### End
###

config.h:	config.h.in
	sed 	-e s%@PREFIX@%$(PREFIX)%g \
		-e s%@VERSION@%`head -1 RELEASE | awk '{print $$3}'`%g \
		-e s%@BUILD@%`grep '^[0-9]' RELEASE | tail -1 | sed -e 's!/!!g'`%g \
		< config.h.in > config.h

clean:
	rm -f *.o perfumesh *~ lib/*~ tests/*~ *core* *.gmon config.h tests/setup.prfm a.out tests/test
	rm -f encoding-set-utoj.h encoding-set-jtou.h
ifeq ($(CORU_USE),yes)
	(cd extlib/coru; $(MAKE) clean)
endif

build-pkg:
	$(MAKE) clean
	$(MAKE)
	rm -rf $(PKG_TMP)
	mkdir -p $(PKG_DIR)
	mkdir -p $(PKG_DIR)/lib
ifeq ($(shell uname),Linux)
	cp -r $(PKG_EXTLIB_DIR)/libgc*.so*     $(PKG_DIR)/lib
	cp -r $(PKG_EXTLIB_DIR)/libgmp*.so*    $(PKG_DIR)/lib
	cp -r $(PKG_EXTLIB_DIR)/libonigmo*.so* $(PKG_DIR)/lib
ifeq ($(shell uname -p),aarch64)
	cp -r /usr/lib/aarch64-linux-gnu/libpcl*.so* $(PKG_DIR)/lib
else
	cp -r $(PKG_EXTLIB_DIR)/libpcl*.so*    $(PKG_DIR)/lib
endif
	chmod 444 $(PKG_DIR)/lib/lib*
endif
	mkdir -p $(PKG_DIR)/bin
	mkdir -p $(PKG_DIR)/lib/perfume
	mkdir -p $(PKG_DIR)/lib/perfume/lib
	mkdir -p $(PKG_DIR)/lib/perfume/lib/pdoc
	mkdir -p $(PKG_DIR)/lib/perfume/lib/Tasklet
	mkdir -p $(PKG_DIR)/lib/perfume/lib/dot-pmacs
	install -m 555 perfumesh  $(PKG_DIR)/bin
	install -m 444 setup.prfm $(PKG_DIR)/lib/perfume
	install -m 444 lib/*.prfm $(PKG_DIR)/lib/perfume/lib
	install -m 444 lib/*.conf $(PKG_DIR)/lib/perfume/lib
	install -m 444 lib/*.key  $(PKG_DIR)/lib/perfume/lib
	install -m 444 lib/*.logo $(PKG_DIR)/lib/perfume/lib
	install -m 444 lib/*.keymap $(PKG_DIR)/lib/perfume/lib
	install -m 444 lib/pdoc/* $(PKG_DIR)/lib/perfume/lib/pdoc
	install -m 444 lib/Tasklet/* $(PKG_DIR)/lib/perfume/lib/Tasklet
	install -m 444 lib/dot-pmacs/* $(PKG_DIR)/lib/perfume/lib/dot-pmacs
	install -m 444 misc/default.fcab $(PKG_DIR)/lib/perfume/lib
	install -m 555 pkg/install.sh $(PKG_DIR)
	install -m 444 pkg/INSTALL    $(PKG_DIR)
	install -m 444 pkg/pmacs.in   $(PKG_DIR)/bin
	install -m 444 pkg/pmacs-client.in   $(PKG_DIR)/bin
	install -m 444 RELEASE $(PKG_DIR)/lib/perfume
	(cd $(PKG_TMP); tar cvzf $(PKG_TAR_NAME) ./pmacs-install)

#eof
