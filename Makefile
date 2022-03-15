.PHONY: tests clean lines

CXXFLAGS += -g -Wall

tests: scheme
	./tests.scm

scheme:

scheme.scm.h: scheme.scm
	which text2c >/dev/null && text2c <$^ >$@  || true
	
scheme.cpp: $(wildcard *.h)
	touch $@

clean:
	rm -f scheme

lines:
	cat *.cpp *.h | wc -l
