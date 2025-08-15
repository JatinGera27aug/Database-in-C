
#runned by  : mingw32-make

# Compiler & flags
flags = -Wall -O2 -std=c2x
ldflags =

# Default target
all: clean tree

# Link object file into executable
tree: tree.o
	cc $(flags) $^ -o $@.exe $(ldflags)

# Compile source file into object file
tree.o: tree.c
	cc $(flags) -c $^

# Clean build files (Windows PowerShell / CMD style)
clean:
	del /Q *.o 2>nul
	del /Q tree.exe 2>nul

