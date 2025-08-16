# runned by : mingw32-make

#/*a Makefile in VS Code is basically your project’s build automation script — it saves you from manually typing long gcc commands every time you compile, 
#   and it also handles multi-file dependencies for you. */

# Compiler & flags
flags = -O2 -Wall -std=c2x
ldflags_cache22 = -lws2_32
ldflags_tree =

# Default target (builds everything fresh)
all: clean tree cache22

# ---------- TREE ----------  mingw32-make tree
tree: tree.exe   # alias

tree.exe: tree.o
	cc $(flags) $^ -o $@ $(ldflags_tree)

tree.o: tree.c
	cc $(flags) -c $^

# ---------- CACHE22 ----------  by:mingw32-make cache22
cache22: cache22.exe   # alias

cache22.exe: cache22.o
	cc $(flags) $^ -o $@ $(ldflags_cache22)

cache22.o: cache22.c
	cc $(flags) -c cache22.c

# ---------- CLEAN ----------
clean:
	del /Q *.o 2>nul
	del /Q tree.exe 2>nul
	del /Q cache22.exe 2>nul

# Mark phony targets
.PHONY: all clean tree cache22
