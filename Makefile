### References ###

# * https://github.com/JackWetherell/c-project-structure/blob/master/Makefile
# * https://stackoverflow.com/questions/5178125/how-to-place-object-files-in-separate-subdirectory


### Configuration ###

PROJDIR  := $(realpath $(CURDIR))
SRCDIR   := src
BUILDDIR := build
TARGET 	 := bin/ubserver

CC 	     := clang -O3
CFLAGS   := -std=c17 -D_GNU_SOURCE=1 
#CFLAGS 	 += -ggdb3
CFLAGS 	 += -pthread
#CFLAGS 	 += -fsanitize=thread
CFLAGS 	 += -DNDEBUG
CFLAGS   += -MMD -Wall -Wextra -Wno-vla -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wshadow
CFLAGS   += -fsanitize=signed-integer-overflow -fsanitize=undefined -fno-common
## zlib https://stackoverflow.com/a/20206371/2046442 ##
CFLAGS   += -lz 

## mime types ## 
# sudo apt-get install libmagic-dev 
# #include <magic.h>
CFLAGS 	 += -lmagic
#compiler database
#CFLAGS 	 += -MJ compile_commands.json

# include headers files from lib directory
INCLUDE_LIB 	:= -I lib
# include headers files from include directory
INCLUDE_HEADERS := -I include

CFLAGS   += $(INCLUDE_LIB) $(INCLUDE_HEADERS)

# C source lib files
LIBRARY_SOURCES := $(wildcard lib/**/*.c)
# Objecst lib files dependencies
LIBRARY_OBJECTS := $(LIBRARY_SOURCES:.c=.o)
# C source files for the main program.
SOURCES			:= $(wildcard $(SRCDIR)/*.c)
# Object files corresponding to source files.
OBJECTS     	:= $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.c=.o))
ALL_OBJECTS 	:= $(OBJECTS) $(LIBRARY_OBJECTS)
# Dependency files corresponding to object files.
DEP 			:= $(OBJECTS:.o=.d)
# Stand-alone header
# DEPS 			:= include/http_status_code.h

### Rules ###

.PHONY: $(TARGET) all lib

all: $(TARGET) lib

lib: $(LIBRARY_OBJECTS) 

# The main server program
$(TARGET): $(OBJECTS) $(LIBRARY_OBJECTS)
	@echo "Compiling..."
	$(CC) $(CFLAGS) $^ -o $(TARGET)
	@echo "Run..."
	@./$(TARGET)

# Build object files from C source files.
$(BUILDDIR)/%.o:$(SRCDIR)/%.c FORCE
#$(ALL_OBJECTS): %.o: %.c FORCE
	@echo "Created object $@ ..."
	$(CC) $(CFLAGS) -c $< -o $@

#$(BUILDDIR)/%.o: $(DEPS)

# Build object files from C source lib files.
# $(LIBRARY_OBJECTS):$(LIBRARY_SOURCES) FORCE
# 	@echo "Created object $@ ..."
# 	$(CC) $(CFLAGS) -c $< -o $@

test-request: $(BUILDDIR)/test-request.o
	$(CC) $(CFLAGS) $^ -o bin/test-request

$(BUILDDIR)/test-request.o: tests/request.c FORCE
	$(CC) $(CFLAGS) -c $< -o $@

run-test-request:
	@./$(TARGET) & ./bin/test-request

.PHONY: FORCE clean
FORCE:

test-variables:
	@echo "Project directory: $(PROJDIR)"
	@echo "SOURCES:"
	@echo $(SOURCES)
	@echo "OBJECTS:"
	@echo $(OBJECTS)
	@echo "LIBRARY_OBJECTS"
	@echo $(LIBRARY_OBJECTS)
	@echo "ALL_OBJECTS"
	@echo $(ALL_OBJECTS)

# Clean up
clean:
	rm -f $(OBJECTS) $(DEP) $(TARGET)