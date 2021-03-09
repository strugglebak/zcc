parser: parser.c interpreter.c main.c scan.c ast.c generator.c generator_core.c statement.c helper.c symbol_table.c
	cc -o parser -g parser.c interpreter.c main.c scan.c ast.c generator.c generator_core.c statement.c helper.c symbol_table.c

clean:
	rm -f parser *.o *.s out

test: parser
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
