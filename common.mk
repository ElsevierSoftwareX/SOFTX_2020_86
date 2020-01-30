LIBS = -lvulkan
CC = g++
CFLAGS = -g -Wall --std=c++11
VKCOMPDIR = ../vkcomp
VKHEADERSDIR = /usr/include/vulkan
INCLUDE = -I$(VKHEADERSDIR) -I$(VKCOMPDIR)

.PHONY: default all clean

#SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
#OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

#OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
#HEADERS = $(wildcard *.h)

SRC_FILES = $(wildcard $(VKCOMPDIR)/*.cpp)
OBJECTS = $(patsubst $(VKCOMPDIR)/%.cpp, %.o, $(SRC_FILES))
HEADERS = $(wildcard *.h)

%.o: $(VKCOMPDIR)/%.cpp #$(HEADERS)
	$(CC) $(CFLAGS) $(CDEFINES) -c $< -I$(VKHEADERSDIR) -o $(VKCOMPDIR)/$@

all: default
default: $(TARGET)

#$(info $$SRC_FILES is [${SRC_FILES}])
#$(info $$OBJECTS is [${OBJECTS}])

#.PRECIOUS: $(TARGET)

$(TARGET): $(OBJECTS) $(TARGET).o
	$(CC) -o $@ $(OBJECTS) $(TARGET).o $(CDEFINES) $(INCLUDE) -Wall $(LIBS) -L$(VKCOMPDIR)

$(TARGET).o: $(TARGET).cpp
	$(CC) $(CFLAGS) -o $@ -c $(TARGET).cpp  $(CDEFINES) $(INCLUDE) -Wall $(LIBS) -L$(VKCOMPDIR)


clean:
	-rm -f *.o
	-rm -f $(TARGET)

#all:
#	nvcc -O3 ${CUFILES} -o ${EXECUTABLE} 
#clean:
#	rm -f *~ *.exe