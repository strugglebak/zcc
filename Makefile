CC= cc
CFLAGS+= -D _linux
LIB= lib/print_int.c

COMMON= parser.c interpreter.c main.c scan.c ast.c generator.c  statement.c helper.c symbol_table.c types.c declaration.c
SRCS= $(COMMON) generator_core.c
ARM_SRCS= $(COMMON) generator_core_arm.c

parser: $(SRCS)
	$(CC) $(CFLAGS) -o parser -g $(SRCS)

parser2: $(SRCS)
	$(CC) -o parser2 -g $(SRCS)
	cp parser2 parser

parser_arm: $(ARM_SRCS)
	$(CC) -o parser_arm -g -Wall $(ARMSRCS)
	cp parser_arm parser

clean:
	rm -f parser parser2 parser_arm *.o *.s out test/out.input*

test: parser test/run_test
	(cd test; chmod +x make_test; chmod +x run_test; ./make_test && ./run_test)

test_arm: parser_arm test/run_test
	(cd test; chmod +x make_test; chmod +x run_test; ./make_test && ./run_test)

test11: parser test/input11 $(LIB)
	./parser test/input11
	cc -o out out.s $(LIB)
	./out

test11_arm: parser_arm test/input11 $(LIB)
	./parser test/input11
	cc -o out out.s $(LIB)
	./out
