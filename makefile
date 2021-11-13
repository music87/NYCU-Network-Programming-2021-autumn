CXX = g++
HEADERS1 = commands_np_simple.h npshell_np_simple.h parsing_line_np_simple.h
HEADERS2 = commands_np_single_proc.h npshell_np_single_proc.h parsing_line_np_single_proc.h client_units.h
TARGET1 = np_simple
TARGET2 = np_single_proc
TARGET3 = np_multi_proc
CFLAG = -g -Wall  
DEMODIR = ../np_project2_demo_sample
PORT = 7086

all: $(TARGET1) $(TARGET2) $(TARGET3)

run: $(TARGET2)
	./$(TARGET2) $(PORT)

gdb: $(TARGET1)
	gdb --args ./$(TARGET2) $(PORT)

$(TARGET1):$(TARGET1).cpp $(HEADERS1)
$(TARGET2):$(TARGET2).cpp $(HEADERS2)
$(TARGET3):$(TARGET3).cpp $(HEADERS)
$(TARGET1) $(TARGET2) $(TARGET3):
	$(CXX) -o $@ $< $(CFLAG)
	cp $@ $(DEMODIR)


clean:
	rm $(TARGET1)  $(TARGET2) $(TARGET3)
