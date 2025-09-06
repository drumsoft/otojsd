# otojsd

V8_ROOT_DIR = $(HOME)/dev/v8/v8

OPTFLAG = -O3
CFLAGS = $(OPTFLAG) -Wall -Wextra -MMD -MP
CXXFLAGS = -std=c++20 -fno-rtti $(CFLAGS)
CC = g++

INCLUDE = -I$(V8_ROOT_DIR)/include
LIBDIR = -L$(V8_ROOT_DIR)/out.gn/arm64.release.sample/obj
LIBS = -lv8_monolith -pthread -ldl
OS_SPECIFIC_LIBS = -framework CoreFoundation -framework CoreServices -framework CoreAudio -framework AudioUnit
V8_FLAGS = -DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX
APP = otojsd

SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
DEPENDS := $(SOURCES:.cpp=.d)

all: $(APP)

debug: 
	make $(APP) "OPTFLAG=-g"

# Build the executable
$(APP): $(OBJECTS)
	$(CC) -o $(APP) $(OBJECTS) $(LIBDIR) $(LIBS) $(OS_SPECIFIC_LIBS) $(CXXFLAGS) $(V8_FLAGS)

# Dependencies
-include $(DEPENDS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(INCLUDE) $(CXXFLAGS) $(V8_FLAGS) -c $< -o $@

test: $(APP)
	./$(APP)

clean:
	-rm $(APP)
	-rm src/*.o
	-rm src/*.d

.PHONY: all clean test
