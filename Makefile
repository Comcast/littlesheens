.PHONY: pkg demo matchtest

#CFLAGS=-Wall -std=c99 -flto -fno-asynchronous-unwind-tables -ffunction-sections -Wl,--gc-sections -m32 -g
CFLAGS=-Wall -std=c99 -fno-asynchronous-unwind-tables -ffunction-sections -Wl,--gc-sections -g -fPIC

DUKVERSION=duktape-2.2.0
DUK ?= ${DUKVERSION}

all: mdemo sheens

duk:	${DUK}

${DUK}:
	wget http://duktape.org/${DUKVERSION}.tar.xz
	tar xf ${DUKVERSION}.tar.xz

libduktape.a: ${DUK} ${DUK}/src/duktape.c ${DUK}/extras/print-alert/duk_print_alert.c Makefile
	gcc $(CFLAGS) -c -I${DUK}/src ${DUK}/src/duktape.c 
	gcc $(CFLAGS) -c -I${DUK}/src -I${DUK}/extras/print-alert ${DUK}/extras/print-alert/duk_print_alert.c
	ar rcs libduktape.a duk_print_alert.o duktape.o

libduktape.so: CFLAGS += -fPIC
libduktape.so: libduktape.a
	$(CC) -shared -o $@ -Wl,--whole-archive $^ -Wl,--no-whole-archive -lm

libmachines.a: ${DUK} machines.c machines.h machines_js.c Makefile
	gcc $(CFLAGS) -c -I${DUK}/src -I${DUK}/extras/print-alert machines.c machines_js.c
	ar rcs libmachines.a machines.o	machines_js.o 

libmachines.so: CFLAGS += -fPIC
libmachines.so: libmachines.a
	$(CC) -shared -o $@ -Wl,--whole-archive $^ -Wl,--no-whole-archive

machines.js: js/match.js js/sandbox.js js/step.js js/prof.js driver.js Makefile
	cat js/match.js js/sandbox.js js/step.js js/prof.js driver.js > machines.js

machines_js.c: machines.js
	minify machines.js > machines.js.terminated
	truncate -s +1 machines.js.terminated
	./embedstr.sh mach_machines_js machines.js.terminated machines_js.c
	rm machines.js.terminated

mdemo:	libmachines.a libduktape.a main.c Makefile
	gcc $(CFLAGS) -o mdemo -I${DUK}/src main.c -L. -lmachines -lduktape -lm
	ls -l ./mdemo

specs/double.js: specs/double.yaml
	cat specs/double.yaml | yaml2json | jq . > specs/double.js

demo:	mdemo specs/double.js


matchtest: mdemo match_test.js
	./mdemo match_test.js | tee match_test.results.js | jq -r '.[]|select(.happy == false)|"\(.n): \(.case.title); wanted: \(.case.w) got: \(.got)"'
	cat match_test.results.js | jq -r '.[]|"\(.n): elapsed \(.bench.elapsed)ms (\(.bench.rounds) rounds) \(.case.title)"'

test:	demo sheens matchtest
	valgrind --leak-check=full --error-exitcode=1 ./mdemo

mdemo.shared: libmachines.so libduktape.so main.c Makefile
	gcc $(CFLAGS) -o mdemo.shared -I${DUK}/src main.c -L. -l:libmachines.so -l:libduktape.so -lm
	ls -l ./mdemo.shared

test.shared:	mdemo.shared specs/double.js
	export LD_LIBRARY_PATH=`pwd`; valgrind --leak-check=full ./mdemo.shared

specs/turnstile.js: specs/turnstile.yaml
	cat specs/turnstile.yaml | yaml2json | jq . > specs/turnstile.js

sheens:	libmachines.a libduktape.a sheens.c Makefile
	gcc $(CFLAGS) -o sheens -I${DUK}/src sheens.c -L. -lmachines -lduktape -lm
	ls -l ./sheens


clean:
	rm -f *.a *.o *.so mdemo machines.js machines_js.c sheens

distclean: clean
	rm -f ${DUKVERSION}.tar.xz
	rm -rf ${DUKVERSION}

tags:
	etags *.c *.h js/*.js demo.js driver.js
