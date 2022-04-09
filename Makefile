.PHONY: tests clean lines

SOURCEs = $(wildcard *.cpp)
OBJECTs = $(SOURCEs:.cpp=.o)

CXXFLAGS += -g -Wall

tests: scheme
	./tests.scm

# err.h: obj.h
# int.h: num_types.h err.h
# num_types.h: value.h
# dynamic.h: obj.h
# num.h: num_types.h int.h value.h
# types.h: dynamic.h num.h
# parser.h: obj.h
# frame.h: obj.h
# eval.h: dynamic.h frame.h
# string.h: value.h dynamic.h
# primitives.h: frame.h

obj.o: obj.cpp obj.h eval.h dynamic.h frame.h
err.o: err.cpp err.h obj.h
int.o: int.cpp int.h num_types.h value.h err.h dynamic.h obj.h string.h
num.o: num.cpp num.h int.h num_types.h value.h err.h dynamic.h obj.h parser.h
types.o: types.cpp types.h dynamic.h obj.h num.h num_types.h value.h int.h err.h
parser.o: parser.cpp parser.h obj.h types.h dynamic.h num.h num_types.h value.h int.h err.h string.h
frame.o: frame.cpp frame.h types.h dynamic.h obj.h num.h num_types.h value.h int.h err.h
eval.o: eval.cpp eval.h dynamic.h obj.h frame.h parser.h
primitives.o: primitives.cpp primitives.h frame.h obj.h eval.h dynamic.h string.h
scheme.o: scheme.cpp parser.h obj.h err.h eval.h dynamic.h frame.h primitives.h scheme.scm.h

scheme: $(OBJECTs)
	$(CXX) $^ -o $@

scheme.scm.h: scheme.scm
	which text2c >/dev/null && text2c <$^ >$@  || true
	
clean:
	rm -f scheme $(OBJECTs)
	which text2c >/dev/null && rm -f scheme.scm.h || true

lines:
	cat *.cpp *.h | wc -l
