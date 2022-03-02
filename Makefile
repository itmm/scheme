.PHONY: clean

CXXFLAGS += -g -Wall

scheme:

scheme.cpp: $(wildcard *.h)
	touch $@

clean:
	rm -f scheme
