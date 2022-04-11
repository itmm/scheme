.PHONY: tests clean lines

SOURCEs = $(wildcard *.cpp)
OBJECTs = $(addprefix build/,$(SOURCEs:.cpp=.o))

CXXFLAGS += -g -Wall -std=c++17

tests: scheme
	./tests.scm

include $(wildcard deps/*.dep)

build/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(notdir $(@:.o=.cpp)) -o $@ -MMD -MF deps/$(notdir $(@:.o=.dep))

build/scheme.o: scheme.scm.h

scheme: $(OBJECTs)
	$(CXX) $^ -o $@

scheme.scm.h: scheme.scm
	which text2c >/dev/null && text2c <$^ >$@  || true
	
clean:
	rm -Rf scheme build deps
	which text2c >/dev/null && rm -f scheme.scm.h || true
	mkdir -p build deps

lines:
	cat *.cpp *.h | wc -l
