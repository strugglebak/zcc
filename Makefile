CC= cc
DEBUG= -T

COMMON= parser.c interpreter.c main.c \
	scan.c ast.c generator.c  statement.c \
	helper.c symbol_table.c types.c declaration.c \
	optimizer.c

HSRCS= data.h parser.h interpreter.h  \
	scan.h ast.h generator.h  statement.h \
	helper.h symbol_table.h types.h declaration.h \
	optimizer.h

SRCS= $(COMMON) generator_core.c
ARM_SRCS= $(COMMON) generator_core_arm.c
TEST_CASE_NAME= 151
TEST_CASE= test/input$(TEST_CASE_NAME).zc
INCLUDE_DIRECTORY= /tmp/include
BINARAY_DIRECTORY= /tmp

incdir.h:
	echo "#define INCDIR \"$(INCLUDE_DIRECTORY)\"" > incdir.h

clean:
	rm -f parser parser0 parser1 parser2 parser_arm *.o *.s out test/out *.out

install: parser
	sudo mkdir -p $(INCLUDE_DIRECTORY)
	sudo rsync -a include/. $(INCLUDE_DIRECTORY)
	sudo cp parser $(BINARAY_DIRECTORY)
	sudo chmod +x $(BINARAY_DIRECTORY)/parser

parser: $(SRCS) $(HSRCS)
	$(CC) -o parser -g $(SRCS)

parser_arm: $(ARM_SRCS) $(HSRCS)
	$(CC) -o parser -g $(ARM_SRCS)
	cp parser_arm parser

gen: test/make_test
	(cd test; chmod +x make_test; ./make_test)

test: install test/run_test
	(cd test; chmod +x run_test; ./run_test)

test_arm: parser_arm test/run_test
	(cd test; chmod +x run_test; ./run_test)

t: parser $(TEST_CASE)
	./parser -o out $(TEST_CASE)
	./out

t0: parser0 $(TEST_CASE)
	./parser0 -o out $(TEST_CASE)
	./out

t_arm: parser_arm $(TEST_CASE) $(LIB)
	./parser -o out $(TEST_CASE)
	./out

# 编译自举
test0: install test/run_test parser0
	(cd test; chmod +x run_test; ./run_test 0)

triple: parser1
	size parser[01]

quad: parser2
	size parser[012]

parser2: parser1 $(SRCS) $(HSRCS)
	./parser1 -o parser2 $(SRCS)

parser1: parser0 $(SRCS) $(HSRCS)
	./parser0 -o parser1 $(SRCS)

parser0: install $(SRCS) $(HSRCS)
	./parser -o parser0 $(SRCS)
