CC= cc
CFLAGS+= -D _linux
SRCS= parser.c interpreter.c main.c scan.c ast.c generator.c generator_core.c statement.c helper.c symbol_table.c types.c declaration.c
parser: $(SRCS)
	$(CC) $(CFLAGS) -o parser -g $(SRCS)

parser2: $(SRCS)
	$(CC) -o parser2 -g $(SRCS)

clean:
	rm -f parser parser2 *.o *.s out test/out.input*


test: parser test/run_test
	(cd test; chmod +x make_test; chmod +x run_test; ./make_test && ./run_test)

test10: parser test/input10
	./parser test/input10
	cc -o out out.s
	./out
