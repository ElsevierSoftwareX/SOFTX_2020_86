CC = g++
CFLAGS = -Wall -O3 --std=gnu++11
VKCOMPDIR = ../vkcomp
ifeq ($(OS),Windows_NT)
VKHEADERSDIR = $(VULKAN_SDK)/Include/vulkan
LIBS = -L$(VULKAN_SDK)\Bin -lVulkan-1
#$(info vksdk is $(VKHEADERSDIR))
else 
VKHEADERSDIR = /usr/include/vulkan
LIBS = -lvulkan
endif

INCLUDE = -I$(VKHEADERSDIR) -I$(VKCOMPDIR)

.PHONY: default all clean

#SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
#OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

#OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))
#HEADERS = $(wildcard *.h)

SRC_FILES = $(wildcard $(VKCOMPDIR)/*.cpp)
OBJECTS = $(patsubst $(VKCOMPDIR)/%.cpp, $(VKCOMPDIR)/%.o, $(SRC_FILES))
HEADERS = $(wildcard *.h)

%.o: $(VKCOMPDIR)/%.cpp #$(HEADERS)
	$(CC) $(CFLAGS) $(CDEFINES) -c $< -I$(VKHEADERSDIR) -o $@

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
	-rm -f *.spv
	-rm -f *.o
	-rm -f $(TARGET)

#all:
#	nvcc -O3 ${CUFILES} -o ${EXECUTABLE} 
#clean:
#	rm -f *~ *.exe
