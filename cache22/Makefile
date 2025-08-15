# runned by : mingw32-make

#/*a Makefile in VS Code is basically your project’s build automation script — it saves you from manually typing long gcc commands every time you compile, 
#   and it also handles multi-file dependencies for you. */

# Compiler & flags
flags = -O2 -Wall -std=c2x
ldflags =

# Default target
all: clean cache22

# Link object file into executable
cache22: cache22.o
	cc $(flags) $^ -o $@.exe $(ldflags)




# Compile source file into object file
cache22.o: cache22.c
	cc $(flags) -c $^

# Clean build files (Windows style)
clean:
	del /Q *.o 2>nul
	del /Q cache22.exe 2>nul
