.PHONY: matchtest

#CFLAGS=-Wall -std=c99 -flto -fno-asynchronous-unwind-tables -ffunction-sections -Wl,--gc-sections -m32 -g
CFLAGS=-Wall -std=c99 -fno-asynchronous-unwind-tables -ffunction-sections -Wl,--gc-sections -g -fPIC

DUKVERSION=duktape-2.2.0
DUK ?= ${DUKVERSION}

all: demo sheensio driver demo.shared

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

specs/double.js: specs/double.yaml
	cat specs/double.yaml | yaml2json | jq . > specs/double.js

specs/turnstile.js: specs/turnstile.yaml
	cat specs/turnstile.yaml | yaml2json | jq . > specs/turnstile.js

sheensio:	libmachines.a libduktape.a sheensio.c Makefile
	gcc $(CFLAGS) -o sheensio -I${DUK}/src sheensio.c -L. -lmachines -lduktape -lm

demo:	libmachines.a libduktape.a demo.c util.h util.c Makefile
	gcc $(CFLAGS) -o demo -I${DUK}/src demo.c util.c -L. -lmachines -lduktape -lm

demo.shared: libmachines.so libduktape.so demo.c util.c util.h Makefile
	gcc $(CFLAGS) -o demo.shared -I${DUK}/src main.c -L. -l:libmachines.so -l:libduktape.so -lm
	ls -l ./demo.shared

driver:	libmachines.a libduktape.a driver.c util.h util.c Makefile
	gcc $(CFLAGS) -o driver -I${DUK}/src driver.c util.c -L. -lmachines -lduktape -lm

matchtest: driver match_test.js
	./driver match_test.js | tee match_test.results.json | jq -r '.[]|select(.happy == false)|"\(.n): \(.case.title); wanted: \(.case.w) got: \(.got)"'
	cat match_test.results.json | jq -r '.[]|"\(.n): elapsed \(.bench.elapsed)ms (\(.bench.rounds) rounds) \(.case.title)"'

test:	demo sheensio matchtest demo.shared
	valgrind --leak-check=full --error-exitcode=1 ./demo
	export LD_LIBRARY_PATH=`pwd`; valgrind --leak-check=full ./demo.shared

test.shared:	mdemo.shared specs/double.js

clean:
	rm -f *.a *.o *.so machines.js machines_js.c sheensio driver demo demo.shared

distclean: clean
	rm -f ${DUKVERSION}.tar.xz
	rm -rf ${DUKVERSION}

tags:
	etags *.c *.h js/*.js demo.js driver.js
