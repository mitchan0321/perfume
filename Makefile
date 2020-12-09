###
### Set options initial value
###
OPTIONS =
OPTLIBS =
OPTLIBS2 =

############################################################################
###
###   FIX ME
###
PREFIX		= /usr/local
CC		= cc
PKG_TMP		= $(HOME)/tmp
PKG_DIR		= $(PKG_TMP)/pmacs-install
PKG_EXTLIB_DIR	= /usr/local/lib
PKG_TAR_NAME	= pmacs-install.tar.gz

###
### SET ENABLE FEATURE OPTIONS
###

### Enable curses library (for use in perfume intepriter execute curs-* commands)
NCURSES = yes

ifeq ($(NCURSES),yes)
  OPTIONS	+= -DNCURSES
  OPTLIBS	+= -lncursesw
  OPTLIBS2	+= -lncursesw
endif


### for product build. (use BoehmGC)
CFLAGS		= -Wall -O2 -c -g $(OPTIONS)
INCLUDE		= -I/usr/local/include -I.
LIB		= -L/usr/lib -L/lib -L/usr/local/lib \
		  -lm -lpthread -lgmp -lgc -lonigmo -lpcl $(OPTLIBS)

### for normaly link option (BSD and Linux)
#		  -lm -lpthread -lgmp -lgc -lonigmo -lpcl $(OPTLIBS)
### for Linux static link options
#		  -static-libgcc -Wl,-Bdynamic,-lc,-ldl,-lm,-lpthread,-lgc,-Bstatic,-lgmp,-lonigmo,-lpcl,$(OPTLIBS2)
### for BSD static link options
#		  -static -lm -lpthread -lgmp -lgc -lonigmo -lpcl $(OPTLIBS)

### for memory debuging build.
#CFLAGS		= -Wall -c -g -DPROF $(OPTIONS)
#INCLUDE		= -I/usr/local/include -I.
#LIB		= -L/usr/local/lib -lm -lonigmo -lpcl -lgmp $(OPTLIBS)

### for profiling build.
#CFLAGS		= -Wall -c -g -pg -DPROF $(OPTIONS)
#INCLUDE		= -I/usr/local/include -I.
#LIB		= -pg -L/usr/local/lib -lonigmo -lpcl -lgmp $(OPTLIBS)

###
###   DONE
###
############################################################################

HDRS		= bulk.h binbulk.h cell.h array.h error.h hash.h interp.h parser.h \
		  toy.h types.h config.h global.h cstack.h util.h encoding.h
SRCS		= bulk.c binbulk.c cell.c array.c hash.c list.c parser.c types.c \
		  eval.c interp.c commands.c methods.c global.c cstack.c util.c \
		  encoding.c encoding-table.c
OBJS		= bulk.o binbulk.o cell.o array.o hash.o list.o parser.o types.o \
		  eval.o interp.o commands.o methods.o global.o cstack.o util.o \
		  encoding.o encoding-table.o

ifeq ($(NCURSES),yes)
  SRCS		+= ncurses.c
  OBJS		+= ncurses.o
endif

all:		perfumesh

install:
	if [ ! -d $(PREFIX)/lib/perfume ]; then mkdir $(PREFIX)/lib/perfume; fi
	if [ ! -d $(PREFIX)/lib/perfume/lib ]; then mkdir $(PREFIX)/lib/perfume/lib; fi
	install -m 755 perfumesh $(PREFIX)/bin
	install -m 644 setup.prfm $(PREFIX)/lib/perfume
	install -m 644 lib/*.prfm $(PREFIX)/lib/perfume/lib

perfumesh:	$(OBJS) perfumesh.o
	$(CC) $(OBJS) perfumesh.o $(LIB) -o perfumesh

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
		< config.h.in > config.h

clean:
	rm -f *.o perfumesh *~ lib/*~ tests/*~ *core* *.gmon config.h tests/setup.prfm a.out tests/test
	rm -f encoding-set-utoj.h encoding-set-jtou.h

pkg-make:
	make clean
	make
	rm -rf $(PKG_TMP)
	mkdir -p $(PKG_DIR)
	mkdir -p $(PKG_DIR)/lib
	cp -r $(PKG_EXTLIB_DIR)/libgc*     $(PKG_DIR)/lib
	cp -r $(PKG_EXTLIB_DIR)/libgmp*    $(PKG_DIR)/lib
	cp -r $(PKG_EXTLIB_DIR)/libonigmo* $(PKG_DIR)/lib
	cp -r $(PKG_EXTLIB_DIR)/libpcl*    $(PKG_DIR)/lib
	chmod 644 $(PKG_DIR)/lib/lib*
	mkdir -p $(PKG_DIR)/bin
	mkdir -p $(PKG_DIR)/lib/perfume
	mkdir -p $(PKG_DIR)/lib/perfume/lib
	install -m 755 perfumesh  $(PKG_DIR)/bin
	install -m 644 setup.prfm $(PKG_DIR)/lib/perfume
	install -m 644 lib/*.prfm $(PKG_DIR)/lib/perfume/lib
	install -m 755 pkg/install.sh $(PKG_DIR)
	install -m 644 pkg/INSTALL    $(PKG_DIR)
	install -m 644 pkg/pmacs.in   $(PKG_DIR)/bin
	(cd $(PKG_TMP); tar cvzf $(PKG_TAR_NAME) ./pmacs-install)

#eof
