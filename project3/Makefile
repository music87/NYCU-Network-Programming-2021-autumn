CXX=g++
CXXFLAGS=-std=c++11 -Wall -pedantic -pthread -lboost_system -g
CXX_INCLUDE_DIRS=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))
TARGET1 = http_server
TARGET2 = console
HEADER = global_variables.h

all: $(TARGET1) $(TARGET2).cgi
#	cp $(TARGET1).cpp $(TARGET2).cpp $(HEADER) ../np_project3_demo_sample/src/309551177/

run:
	./$(TARGET1) 7086

gdb:
	gdb --args ./$(TARGET1) 7086

copy:
	cp $(TARGET1).cpp $(TARGET2).cpp $(HEADER) ../np_project3_demo_sample/src/309551177/

$(TARGET1): $(TARGET1).cpp $(HEADER)
	$(CXX)  -o $@ $< $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

$(TARGET2).cgi: $(TARGET2).cpp $(HEADER)
	$(CXX)  -o $@ $< $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)
clean:
	rm -f $(TARGET1) $(TARGET2).cgi
