parser: parser.c interpreter.c main.c scan.c ast.c
	cc - o parser -g parser.c interpreter.c main.c scan.c ast.c

clean:
	rm -f parser *.o
