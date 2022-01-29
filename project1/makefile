CXX = g++
TARGET = npshell
HEADERS = commands.h commands.cpp parsing_line.h units.h
CFLAG = -g -Wall  
DEMODIR = ../np_project1_demo_sample

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)
gdb: $(TARGET)
	gdb ./$(TARGET)
$(TARGET): $(TARGET).cpp $(HEADERS) 
	$(CXX) -o $@ $< $(CFLAG)
	#cp $@ $(DEMODIR)
clean:
	rm $(TARGET) 
