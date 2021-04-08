CC= cc
CFLAGS+= -D _linux
SRCS= parser.c interpreter.c main.c scan.c ast.c generator.c generator_core.c statement.c helper.c symbol_table.c
parser: $(SRCS)
	$(CC) $(CFLAGS) -o parser -g $(SRCS)

parser2: $(SRCS)
	$(CC) -o parser2 -g $(SRCS)

clean:
	rm -f parser parser2 *.o *.s out

test: parser ./test/*
		./parser ./test/input00
	  cc -o out out.s
		./out

		./parser ./test/input01
	  cc -o out out.s
		./out

		./parser ./test/input02
	  cc -o out out.s
		./out

		./parser ./test/input03
	  cc -o out out.s
		./out

		./parser ./test/input04
	  cc -o out out.s
		./out

		./parser ./test/input05
	  cc -o out out.s
		./out

test2: parser2 ./test/*
		./parser2 ./test/input00
	  cc -o out out.s
		./out

		./parser2 ./test/input01
	  cc -o out out.s
		./out

		./parser2 ./test/input02
	  cc -o out out.s
		./out

		./parser2 ./test/input03
	  cc -o out out.s
		./out

		./parser2 ./test/input04
	  cc -o out out.s
		./out

		./parser2 ./test/input05
	  cc -o out out.s
		./out
