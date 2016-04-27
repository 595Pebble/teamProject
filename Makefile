

all: server

server: project.c
	clang -o output -lpthread project.c

clean:
	rm -rf server *.o