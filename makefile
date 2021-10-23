CXX = g++
TARGET = npshell
HEADERS = commands.h commands.cpp parsing_line.h
CFLAG = -g -Wall -Wextra 
DEMODIR = ../np_project1_demo_sample

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)
gdb: $(TARGET)
	gdb ./$(TARGET)
$(TARGET): $(TARGET).cpp $(HEADERS) 
	$(CXX) -o $@ $< $(CFLAG)
	cp $@ $(DEMODIR)
clean:
	rm $(TARGET) 
