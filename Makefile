CC= cc
DEBUG= -T

COMMON= data.c parser.c interpreter.c main.c \
	scan.c ast.c generator.c  statement.c \
	helper.c symbol_table.c types.c declaration.c \
	optimizer.c

SRCS= $(COMMON) generator_core.c
ARM_SRCS= $(COMMON) generator_core_arm.c
TEST_CASE_NAME= 142
TEST_CASE= test/input$(TEST_CASE_NAME).zc
INCLUDE_DIRECTORY= /tmp/include
BINARAY_DIRECTORY= /tmp

clean:
	rm -f parser parser_arm *.o *.s out test/out

install: parser
	sudo mkdir -p $(INCLUDE_DIRECTORY)
	sudo rsync -a include/. $(INCLUDE_DIRECTORY)
	sudo cp parser $(BINARAY_DIRECTORY)
	sudo chmod +x $(BINARAY_DIRECTORY)/parser

parser: $(SRCS)
	$(CC) -o parser -g -DINCDIR=\"$(INCLUDE_DIRECTORY)\" $(SRCS)

parser_arm: $(ARM_SRCS)
	$(CC) -o parser -g -DINCDIR=\"$(INCLUDE_DIRECTORY)\" $(ARM_SRCS)
	cp parser_arm parser

gen: test/make_test
	(cd test; chmod +x make_test; ./make_test)

test: parser test/run_test
	(cd test; chmod +x run_test; ./run_test)

test_arm: parser_arm test/run_test
	(cd test; chmod +x run_test; ./run_test)

t: parser $(TEST_CASE)
	./parser -o out $(TEST_CASE)
	./parser -S $(TEST_CASE)
	./out

t_arm: parser_arm $(TEST_CASE) $(LIB)
	./parser -o out $(TEST_CASE)
	./out
