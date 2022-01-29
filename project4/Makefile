CXX=g++
CXXFLAGS=-std=c++11 -Wall -pedantic -pthread -lboost_system -g
CXX_INCLUDE_DIRS=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))
TARGET1 = socks_server
TARGET2 = console
HEADER = global_variables.h socks4_helper.h 

all: $(TARGET1) hw4.cgi
	cp hw4.cgi ~/public_html
run:
	./$(TARGET1) 7414

gdb:
	gdb --args ./$(TARGET1) 7414

$(TARGET1): $(TARGET1).cpp $(HEADER)
	$(CXX)  -o $@ $< $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

hw4.cgi: $(TARGET2).cpp $(HEADER)
	$(CXX)  -o $@ $< $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)
clean:
	rm -f $(TARGET1) hw4.cgi
