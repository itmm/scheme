.PHONY: tests clean lines

SOURCEs = $(wildcard *.cpp)
OBJECTs = $(SOURCEs:.cpp=.o)

CXXFLAGS += -g -Wall

tests: scheme
	./tests.scm

# err.h: obj.h
# int.h: num_base.h err.h
# num_base.h: type_base.h
# type_base.h: obj.h
# num.h: num_base.h int.h
# types.h: type_base.h num.h
# parser.h: obj.h
# frame.h: obj.h
# eval.h: type_base.h frame.h
# primitives.h: frame.h

obj.o: obj.cpp obj.h eval.h type_base.h frame.h
err.o: err.cpp err.h obj.h
int.o: int.cpp int.h num_base.h err.h type_base.h obj.h
num.o: num.cpp num.h int.h num_base.h err.h type_base.h obj.h parser.h
types.o: types.cpp types.h type_base.h obj.h num.h num_base.h type_base.h int.h err.h
parser.o: parser.cpp parser.h obj.h types.h type_base.h num.h num_base.h int.h err.h
frame.o: frame.cpp frame.h types.h type_base.h obj.h num.h num_base.h int.h err.h
eval.o: eval.cpp eval.h type_base.h obj.h frame.h parser.h
primitives.o: primitives.cpp primitives.h frame.h obj.h eval.h type_base.h
scheme.o: scheme.cpp parser.h obj.h err.h eval.h type_base.h frame.h primitives.h scheme.scm.h

scheme: $(OBJECTs)
	$(CXX) $^ -o $@

scheme.scm.h: scheme.scm
	which text2c >/dev/null && text2c <$^ >$@  || true
	
clean:
	rm -f scheme $(OBJECTs)
	which text2c >/dev/null && rm -f scheme.scm.h || true

lines:
	cat *.cpp *.h | wc -l
