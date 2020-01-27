TARGET = 2DConvolution
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
	$(CC) $(CFLAGS) $(CDEFINES) -c $< -I$(VKHEADERSDIR) -o $@

all: default
default: $(TARGET)

$(info $$SRC_FILES is [${SRC_FILES}])
$(info $$OBJECTS is [${OBJECTS}])

#.PRECIOUS: $(TARGET)

$(TARGET): $(OBJECTS) 2DConvolution.o
	$(CC) -o $@ $(OBJECTS) 2DConvolution.o $(CDEFINES) $(INCLUDE) -Wall $(LIBS) -L.

2DConvolution.o: 2DConvolution.cpp
	$(CC) $(CFLAGS) -o $@ -c 2DConvolution.cpp  $(CDEFINES) $(INCLUDE) -Wall $(LIBS) -L.


clean:
	-rm -f *.o
	-rm -f $(TARGET)

