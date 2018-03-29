CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -O3 -flto -g # -fprofile-arcs -ftest-coverage -pg -no-pie
LEX=flex
LFLAGS=-8 -F
YACC=bison
YFLAGS=-y -W

TEST_CXX=g++
TEST_CXXFLAGS=-std=c++11 -Wall -O2 --coverage

FUZZ_CC=afl-gcc
FUZZ_CFLAGS=-std=c99 -pedantic -Wall -O2

SONAR=/mnt/work/sonar/sonarqube-6.7.2/bin/linux-x86-64/sonar.sh
SONAR_RUNNER=/mnt/work/sonar/sonar-runner-2.4/bin/sonar-runner

all: csolve test coverage

src/lexer.c: src/lexer.l src/parser.h
	${LEX} ${LFLAGS} -o $@ $<

src/parser.c src/parser.h: src/parser.y
	${YACC} ${YFLAGS} -o src/parser.c --defines=src/parser.h $<

csolve: src/csolve.c src/arith.c src/eval.c src/propagate.c src/normalize.c src/lexer.c src/parser.c src/print.c src/objective.c
	${CC} ${CFLAGS} -o $@ $^

googletest/googlemock/libgooglemock.a: /usr/src/googletest
	mkdir -p googletest; cd googletest; cmake /usr/src/googletest; ${MAKE}

test/test: test/*.c googletest/googlemock/libgooglemock.a csolve
	${TEST_CXX} ${TEST_CXXFLAGS} -o $@ test/*.c -Lgoogletest/googlemock -lgmock -lpthread

test/xunit-report.xml: test/test
	./$< --gtest_output=xml:$@

test: test/xunit-report.xml

coverage: test
	gcovr --root `pwd` -e "test/*" -x > test/coverage-report.xml

fuzz/csolve: src/csolve.c src/arith.c src/eval.c src/propagate.c src/normalize.c src/lexer.c src/parser.c src/print.c src/objective.c
	${FUZZ_CC} ${FUZZ_CFLAGS} -o $@ $^

fuzz: fuzz/csolve
	afl-fuzz -m 128 -i fuzz/inputs -o fuzz/findings -- $<

sonar_start:
	${SONAR} start
sonar_stop:
	${SONAR} stop
sonar_run:
	${SONAR_RUNNER}