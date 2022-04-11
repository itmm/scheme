.PHONY: tests clean lines

SOURCEs = $(wildcard *.cpp)
OBJECTs = $(addprefix build/,$(SOURCEs:.cpp=.o))

CXXFLAGS += -g -Wall -std=c++17

tests: scheme
	@echo "run tests"
	@./tests.scm

include $(wildcard deps/*.dep)

build/%.o: %.cpp
	@echo "c++ $@"
	@mkdir -p build deps
	@$(CXX) $(CXXFLAGS) -c $(notdir $(@:.o=.cpp)) -o $@ -MMD -MF deps/$(notdir $(@:.o=.dep))

build/scheme.o: scheme.scm.h

scheme: $(OBJECTs)
	@echo "link $@"
	@$(CXX) $^ -o $@

scheme.scm.h: scheme.scm
	@echo "generate $@"
	@which text2c >/dev/null && text2c <$^ >$@  || true
	
clean:
	@echo "clean"
	@rm -Rf scheme build deps
	@which text2c >/dev/null && rm -f scheme.scm.h || true

lines:
	@cat *.cpp *.h | wc -l
