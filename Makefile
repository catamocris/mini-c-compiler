CC = gcc
CFLAGS = -Wall -Wextra -std=c99

EXEC = build/compiler

all: $(EXEC)

$(EXEC): build/main.o build/lexer.o build/utils.o
	$(CC) build/main.o build/lexer.o build/utils.o -o $(EXEC)

build/main.o: src/main.c
	mkdir -p build
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o

build/lexer.o: src/lexer/lexer.c
	mkdir -p build
	$(CC) $(CFLAGS) -c src/lexer/lexer.c -o build/lexer.o

build/utils.o: src/utils/utils.c
	mkdir -p build
	$(CC) $(CFLAGS) -c src/utils/utils.c -o build/utils.o

run: all
	./$(EXEC) tests/testlex.c

clean:
	rm -rf build