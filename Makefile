.PHONY: tests clean lines

SOURCEs = $(wildcard *.cpp)
OBJECTs = $(SOURCEs:.cpp=.o)

CXXFLAGS += -g -Wall

tests: scheme
	./tests.scm

obj.o: obj.cpp obj.h

err.o: err.cpp err.h obj.h

int.o: int.cpp int.h num_base.h type_base.h

scheme: $(OBJECTs)
	$(CXX) $^ -o $@

scheme.scm.h: scheme.scm
	which text2c >/dev/null && text2c <$^ >$@  || true
	
scheme.cpp: $(wildcard *.h)
	touch $@

clean:
	rm -f scheme $(OBJECTs)

lines:
	cat *.cpp *.h | wc -l
