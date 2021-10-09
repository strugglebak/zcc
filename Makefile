CC= cc
LIB=
DEBUG= -T

COMMON= parser.c interpreter.c main.c scan.c ast.c generator.c  statement.c helper.c symbol_table.c types.c declaration.c
SRCS= $(COMMON) generator_core.c
ARM_SRCS= $(COMMON) generator_core_arm.c
TEST_CASE_NAME= 67
TEST_CASE= test/input$(TEST_CASE_NAME).zc
INCLUDE_DIRECTORY= /tmp/include
BINARAY_DIRECTORY= /tmp

clean:
	rm -f parser parser_arm *.o *.s out test/out

parser: $(SRCS)
	$(CC) -o parser -g -Wall -DINCDIR=\"$(INCDIR)\" $(SRCS)

parser_arm: $(ARM_SRCS)
	$(CC) -o parser -g -Wall -DINCDIR=\"$(INCDIR)\" $(ARM_SRCS)
	cp parser_arm parser

gen: test/make_test
	(cd test; chmod +x make_test; ./make_test)

test: parser test/run_test
	(cd test; chmod +x run_test; ./run_test)

test_arm: parser_arm test/run_test
	(cd test; chmod +x run_test; ./run_test)

t: parser $(TEST_CASE) $(LIB)
	./parser -o out $(TEST_CASE)
	./out

t_arm: parser_arm $(TEST_CASE) $(LIB)
	./parser -o out $(TEST_CASE)
	./out
