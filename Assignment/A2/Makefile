LOGFILE='"gc.log"'
FLAGS = -DLOGFILE=${LOGFILE} -Wall -g

all : mktrans do_list do_fstree

do_list : do_list.o list.o gc.o gc_list.o
	gcc ${FLAGS} -o $@ $^

mktrans : mktrans.o list.o gc.o
	gcc ${FLAGS} -o $@ $^

do_fstree : do_fstree.o fstree.o gc.o gc_fstree.o
	gcc ${FLAGS} -o $@ $^

%.o : %.c list.h gc.h fstree.h
	gcc ${FLAGS} -c $<

clean :
	rm *.o mktrans do_list do_fstree gc.log
