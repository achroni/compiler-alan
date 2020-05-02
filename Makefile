.PHONY: clean distclean default


CXXFLAGS=-Wall -g -std=c++11 `llvm-config-6.0 --cxxflags`
LDFLAGS=`llvm-config-6.0 --ldflags --system-libs --libs all`

default: alanc

stack.o: stack.cpp stack.hpp

parser.hpp parser.cpp: parser.y
	bison -dv -o parser.cpp parser.y

lexer.cpp: lexer.l
	flex -s -o lexer.cpp lexer.l

parser.o: parser.cpp parser.hpp ast.hpp

lexer.o: lexer.cpp parser.hpp

ast.o: ast.cpp ast.hpp 

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

alanc: stack.o lexer.o parser.o ast.o general.o error.o symbol.o 
	$(CXX) $(CXXFLAGS) -o alanc $^ $(LDFLAGS) -lfl

clean:
	$(RM) lexer.cpp parser.cpp parser.hpp parser.output a.* *.o *~

distclean: clean
	$(RM) alanc
