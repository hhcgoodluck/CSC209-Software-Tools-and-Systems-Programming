CC = gcc
CFLAGS = -Wall -Wextra -g

all: raid_sim

raid_sim: ../complied_object_files/raid_sim.o complied_object_files/controller.o complied_object_files/disk_sim.o
	$(CC) raid_sim.o controller.o disk_sim.o -o raid_sim


%.o: %.c raid.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f raid_sim.o controller.o disk_sim.o raid_sim disk_*.dat

.PHONY: all clean 
