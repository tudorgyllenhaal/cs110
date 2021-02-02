CXX=/usr/local/Cellar/gcc@6/6.5.0_5.reinstall/bin/g++-6
CXXFLAGS=-c
LIB=string-utils.a
LIB_SRC=string-utils.cpp
LIB_OBJ=$(patsubst %.a, %.o,$(LIB))

default: $(LIB)

$(LIB_OBJ): $(LIB_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(LIB): $(LIB_OBJ)
	rm -f $@
	ar r $@ $^
	ranlib $@

clean:
	rm -f $(LIB) $(LIB_OBJ)
  

