CC=gcc
CFLAGS=-I.

LAB3_DIR = /workspaces/os_lab_2025/lab3/src

all: psum process_memory parallel_min_max

psum: utils.o sum_lib.o parallel_sum.o
	$(CC) -o psum parallel_sum.o sum_lib.o utils.o -pthread $(CFLAGS) 

parallel_sum.o: parallel_sum.c sum_lib.h
	$(CC) -o parallel_sum.o -c parallel_sum.c $(CFLAGS) 

sum_lib.o: sum_lib.c sum_lib.h
	$(CC) -o sum_lib.o -c sum_lib.c $(CFLAGS)

parallel_min_max: utils.o find_min_max.o
	$(CC) -o parallel_min_max $(LAB3_DIR)/parallel_min_max.c utils.o find_min_max.o $(CFLAGS)

utils.o: $(LAB3_DIR)/utils.h
	$(CC) -o utils.o -c $(LAB3_DIR)/utils.c $(CFLAGS)

find_min_max.o: $(LAB3_DIR)/utils.h $(LAB3_DIR)/find_min_max.h
	$(CC) -o find_min_max.o -c $(LAB3_DIR)/find_min_max.c $(CFLAGS)

process_memory: process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

clean:
	rm -f utils.o find_min_max.o parallel_min_max process_memory parallel_sum.o sum_lib.o psum