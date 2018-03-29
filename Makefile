CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -O3 -flto -g # -fprofile-arcs -ftest-coverage -pg -no-pie
LEX=flex
LFLAGS=-8 -F
YACC=bison
YFLAGS=-y -W

TEST_CXX=g++
TEST_CXXFLAGS=-std=c++11 -Wall -O1 --coverage -g

FUZZ_CC=afl-gcc
FUZZ_CFLAGS=-std=c99 -pedantic -Wall -O2

SONAR=/mnt/work/sonar/sonarqube-6.7.2/bin/linux-x86-64/sonar.sh
SONAR_RUNNER=/mnt/work/sonar/sonar-runner-2.4/bin/sonar-runner

HEADERS= \
	src/csolve.h \
	src/parser.h \
	src/parser_support.h
SRC= \
	src/arith.c \
	src/csolve.c \
	src/eval.c \
	src/lexer.c \
	src/normalize.c \
	src/objective.c \
	src/parser.c \
	src/parser_support.c \
	src/print.c \
	src/propagate.c
TESTS= \
	test/test_alloc.c \
	test/test_arith.c \
	test/test_bind.c \
	test/test_eval.c \
	test/test_main.c \
	test/test_normalize.c \
	test/test_objective.c \
	test/test_print.c \
	test/test_propagate.c \
	test/test_update_expr.c

all: csolve test coverage

src/lexer.c: src/lexer.l src/parser.h
	${LEX} ${LFLAGS} -o $@ $<

src/parser.c src/parser.h: src/parser.y
	${YACC} ${YFLAGS} -o src/parser.c --defines=src/parser.h $<

csolve: ${SRC} ${HEADERS}
	${CC} ${CFLAGS} -o $@ ${SRC}

googletest/googlemock/libgooglemock.a: /usr/src/googletest
	mkdir -p googletest; cd googletest; cmake /usr/src/googletest; ${MAKE}

test/%.o: test/%.c ${SRC} ${HEADERS}
	${TEST_CXX} ${TEST_CXXFLAGS} -c -o $@ $<

test/test: $(patsubst %.c,%.o,${TESTS}) googletest/googlemock/libgooglemock.a
	${TEST_CXX} ${TEST_CXXFLAGS} -o $@ $(patsubst %.c,%.o,${TESTS}) -Lgoogletest/googlemock -lgmock -lpthread

test/xunit-report.xml: test/test
	./$< --gtest_output=xml:$@

test: test/xunit-report.xml

test/coverage-report.xml: test/xunit-report.xml
	gcovr -r ${CURDIR} -e "test/*" -x -o test/coverage-report.xml

coverage: test/coverage-report.xml

fuzz/csolve: ${SRC} ${HEADERS}
	${FUZZ_CC} ${FUZZ_CFLAGS} -o $@ ${SRC}

fuzz: fuzz/csolve
	afl-fuzz -m 128 -i fuzz/inputs -o fuzz/findings -- $<

sonar_start:
	${SONAR} start
sonar_stop:
	${SONAR} stop
sonar_run:
	${SONAR_RUNNER}

clean:
	rm -rf csolve fuzz/csolve test/test googletest test/xunit-report.xml test/coverage-report.xml test/*.o test/*.gcda test/*.gcno *.gcda *.gcno