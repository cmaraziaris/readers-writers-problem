
PROGRAM = coordinator

CC = gcc
CFLAGS = -Wall -Wextra -I.

OBJS = semaphores.o shared_mem.o coordinator.o

$(PROGRAM): clean $(OBJS)
	$(CC) $(OBJS) -o $(PROGRAM) -lm

# remove obj + executable files
clean:
	rm -f $(PROGRAM) $(OBJS)

# run with default arguments
# 100 entries 30 processes 30 loops 75% readers/writers ratio
run: $(PROGRAM)
	./$(PROGRAM) 100 30 30 75