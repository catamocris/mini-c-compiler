CC = gcc
CFLAGS = -Wall -std=c99

EXEC = build/compiler

all: $(EXEC)

$(EXEC): build/main.o build/lexer.o build/utils.o build/parser.o
	$(CC) build/main.o build/lexer.o build/utils.o build/parser.o -o $(EXEC)

build/main.o: src/main.c
	mkdir -p build
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o

build/lexer.o: src/lexer/lexer.c
	mkdir -p build
	$(CC) $(CFLAGS) -c src/lexer/lexer.c -o build/lexer.o

build/utils.o: src/utils/utils.c
	mkdir -p build
	$(CC) $(CFLAGS) -c src/utils/utils.c -o build/utils.o

build/parser.o: src/parser/parser.c
	mkdir -p build
	$(CC) $(CFLAGS) -c src/parser/parser.c -o build/parser.o

run: all
	./$(EXEC) tests/testparser.c

clean:
	rm -rf build