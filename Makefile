CC= cc
LIB= lib/print_int.c lib/print_char.c
DEBUG= -T

COMMON= parser.c interpreter.c main.c scan.c ast.c generator.c  statement.c helper.c symbol_table.c types.c declaration.c
SRCS= $(COMMON) generator_core.c
ARM_SRCS= $(COMMON) generator_core_arm.c
TEST_CASE_NAME= 12
TEST_CASE= test/input$(TEST_CASE_NAME).zc

clean:
	rm -f parser parser_arm *.o *.s out test/out.input*

parser: $(SRCS)
	$(CC) -o parser -g $(SRCS)

parser_arm: $(ARM_SRCS)
	$(CC) -o parser_arm -g -Wall $(ARMSRCS)
	cp parser_arm parser

test: parser test/run_test
	(cd test; chmod +x make_test; ./make_test; chmod +x run_test; ./run_test)

test_arm: parser_arm test/run_test
	(cd test; chmod +x make_test; ./make_test; chmod +x run_test; ./run_test)

t: parser $(TEST_CASE) $(LIB)
	./parser $(TEST_CASE)
	cc -o out out.s $(LIB)
	./out

t_arm: parser_arm $(TEST_CASE) $(LIB)
	./parser $(TEST_CASE)
	cc -o out out.s $(LIB)
	./out
