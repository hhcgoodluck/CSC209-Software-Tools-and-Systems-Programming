all : compare truncate strip greeting 

compare : compare.c
	gcc -Wall -g -o compare compare.c

truncate : truncate.c
	gcc -Wall -g -o truncate truncate.c

strip : strip.c
	gcc -Wall -g -o strip strip.c

greeting : greeting.c
	gcc -Wall -g -o greeting greeting.c

clean : 
	-rm compare truncate strip greeting 
