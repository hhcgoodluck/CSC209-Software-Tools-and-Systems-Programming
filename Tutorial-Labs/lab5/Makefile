# A simple Makefile to build the bitmap_printer program. 
# To compile the program, run "make" 
# With no arguments the make program looks for a file called Makefile or
# makefile and evaluates the first rule in the file.

# We are compile each .c file separately to produce the .o files. The .o files
# are called object files and they are linked together to produce the executable

# The first rule below ensures that the object files are up to date and then
# runs gcc to build the exeuctable.

bitmap_printer : bitmap_printer.o bitmap.o
	gcc -Wall -g -o bitmap_printer bitmap_printer.o bitmap.o

# The next two rules build the object files if the source files have changed.
# Note the -c option in the compile command

bitmap_printer.o : bitmap_printer.c bitmap.h
	gcc -Wall -g -c bitmap_printer.c

bitmap.o : bitmap.c bitmap.h
	gcc -Wall -g -c bitmap.c

# This is a handy rule. Type "make clean" to remove the object files and
# executable
clean :
	-rm *.o bitmap_printer
