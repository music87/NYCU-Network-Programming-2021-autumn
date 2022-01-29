CXX = g++
FOLDER1 = np_simple_codes
FOLDER2 = np_single_proc_codes
FOLDER3 = np_multi_proc_codes
COMMON = np_utils

HEADERS1 = $(FOLDER1)/commands.h $(FOLDER1)/npshell.h $(FOLDER1)/parsing_line.h $(COMMON)/units.h $(COMMON)/passiveTCP.h
HEADERS2 = $(FOLDER2)/commands.h $(FOLDER2)/npshell.h $(FOLDER2)/client_units.h $(COMMON)/parsing_line_proc.h $(COMMON)/passiveTCP.h $(COMMON)/units.h
HEADERS3 = $(FOLDER3)/global_variables.h $(FOLDER3)/client_unit.h $(FOLDER3)/commands.h $(FOLDER3)/npshell.h $(COMMON)/parsing_line_proc.h $(COMMON)/passiveTCP.h $(COMMON)/units.h

TARGET1 = np_simple
TARGET2 = np_single_proc
TARGET3 = np_multi_proc

CFLAG = -g -Wall  
DEMODIR = ../np_project2_demo_sample
PORT = 7086

all: $(TARGET1) $(TARGET2) $(TARGET3)

run: $(TARGET3)
	./$(TARGET3) $(PORT)

gdb: $(TARGET3)
	gdb --args ./$(TARGET3) $(PORT)

$(TARGET1):$(FOLDER1)/$(TARGET1).cpp $(HEADERS1)
$(TARGET2):$(FOLDER2)/$(TARGET2).cpp $(HEADERS2)
$(TARGET3):$(FOLDER3)/$(TARGET3).cpp $(HEADERS3)
$(TARGET1) $(TARGET2) $(TARGET3):
	$(CXX) -o $@ $< $(CFLAG)
	#cp $@ $(DEMODIR)


clean:
	rm $(TARGET1)  $(TARGET2) $(TARGET3)
