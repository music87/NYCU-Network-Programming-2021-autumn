CXX = g++
TARGET = npshell
HEADERS = builtin_commands.h parsing_line.h
CFLAG = -g -Wall -Wextra 

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)
gdb: $(TARGET)
	gdb ./$(TARGET)
$(TARGET): $(TARGET).cpp $(HEADERS) 
	$(CXX) -o $@ $< $(CFLAG)

clean:
	rm $(TARGET) 
