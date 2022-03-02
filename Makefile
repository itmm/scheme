.PHONY: clean

CXXFLAGS += -g -Wall

scheme:

scheme.scm.h: scheme.scm
	which text2c >/dev/null && text2c <$^ >$@  || true
	
scheme.cpp: $(wildcard *.h)
	touch $@

clean:
	rm -f scheme
