flags=-Wall -02 -std= c2x
ldflags=
all :clean tree
tree: tree.o
	cc ${flags} $^ -o $@ ${ldflags}
tree.o: tree.cc
	cc ${flags} -c $^
clean:
	rm -f *.o tree
