CC      ?= cc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra
LDLIBS  ?= -lncurses

GAME  = rime
TESTS = test_sim
OBJS  = sim.o gen.o main.o

.PHONY: all clean test run

all: $(GAME)

$(GAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

sim.o: sim.c sim.h
gen.o: gen.c sim.h
main.o: main.c sim.h

$(TESTS): test_sim.c sim.c gen.c sim.h
	$(CC) $(CFLAGS) -o $@ test_sim.c sim.c gen.c

test: $(TESTS)
	./$(TESTS)

run: $(GAME)
	./$(GAME)

clean:
	rm -f $(GAME) $(TESTS) *.o
