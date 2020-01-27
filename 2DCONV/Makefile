TARGET = 2DConvolution
LIBS = -lvulkan
CC = g++
CFLAGS = -g -Wall --std=c++11
VKCOMPDIR = ../vkcomp
VKHEADERSDIR = /usr/include/vulkan
INCLUDE = -I$(VKHEADERSDIR) -I$(VKCOMPDIR)

.PHONY: default all clean

default: $(TARGET)
all: default

#SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
#OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

#OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
#HEADERS = $(wildcard *.h)

SRC_FILES := $(wildcard $(VKCOMPDIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, %.o, $(SRC_FILES))
HEADERS = $(wildcard *.h)

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) $(CDEFINES) -c $< $(VKHEADERSDIR) -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CDEFINES) $(INCLUDE) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)

