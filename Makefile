parser: parser.c interpreter.c main.c scan.c ast.c
	cc -o parser -g parser.c interpreter.c main.c scan.c ast.c

clean:
	rm -f parser *.o

test: parser
	-(./parser ./test/input01; \
		./parser ./test/input02; \
		./parser ./test/input03; \
		./parser ./test/input04; \
		./parser ./test/input05)
