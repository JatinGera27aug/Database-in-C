# Compiler & flags
CC = gcc
CFLAGS = -O2 -Wall -std=c11
LDFLAGS_CACHE22 = -lws2_32
LDFLAGS_TREE =

OBJS_CACHE22 = cache22.o hash_table.o
OBJS_TREE = tree.o

all: clean tree cache22

# ---------- TREE ----------
tree: tree.exe

tree.exe: $(OBJS_TREE)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS_TREE)

tree.o: tree.c tree.h
	$(CC) $(CFLAGS) -c $<

# ---------- CACHE22 ----------
cache22: cache22.exe

cache22.exe: $(OBJS_CACHE22)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS_CACHE22)

cache22.o: cache22.c cache22.h hash_table.h
	$(CC) $(CFLAGS) -c $<

hash_table.o: hash_table.c hash_table.h cache22.h
	$(CC) $(CFLAGS) -c $<

# ---------- CLEAN ----------
clean:
	del /Q *.o 2>nul
	del /Q tree.exe 2>nul
	del /Q cache22.exe 2>nul

.PHONY: all clean tree cache22
