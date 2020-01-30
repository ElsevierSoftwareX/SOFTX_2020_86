TARGET = gramschmidt
LIBS = -lvulkan
CC = g++
CFLAGS = -g -Wall --std=c++11
VKCOMPDIR = ../vkcomp
VKHEADERSDIR = /usr/include/vulkan
INCLUDE = -I$(VKHEADERSDIR) -I$(VKCOMPDIR)

.PHONY: default all clean

SRC_FILES = $(wildcard $(VKCOMPDIR)/*.cpp)
OBJECTS = $(patsubst $(VKCOMPDIR)/%.cpp, %.o, $(SRC_FILES))
HEADERS = $(wildcard *.h)

%.o: $(VKCOMPDIR)/%.cpp #$(HEADERS)
	$(CC) $(CFLAGS) $(CDEFINES) -c $< -I$(VKHEADERSDIR) -o $@

all: default
default: $(TARGET)

$(info $$SRC_FILES is [${SRC_FILES}])
$(info $$OBJECTS is [${OBJECTS}])

$(TARGET): $(OBJECTS) $(TARGET).o
	$(CC) -o $@ $(OBJECTS) $(TARGET).o $(CDEFINES) $(INCLUDE) -Wall $(LIBS) -L.

$(TARGET).o: $(TARGET).cpp
	$(CC) $(CFLAGS) -o $@ -c $(TARGET).cpp  $(CDEFINES) $(INCLUDE) -Wall $(LIBS) -L.


clean:
	-rm -f *.o
	-rm -f $(TARGET)