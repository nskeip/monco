# Default compiler
COMPILER ?= clang

# Compilation flags
CFLAGS = -Wall -Wextra -g

# Source files
SRC_FILES = main.c

# Object files
OBJ_FILES = $(SRC_FILES:.c=.o)

# Output executable
EXECUTABLE = monco-exe

# Targets
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ_FILES)
	$(COMPILER) $(CFLAGS) $^ -o $@

%.o: %.c
	$(COMPILER) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_FILES) $(EXECUTABLE)

test: clean $(EXECUTABLE)
	./$(EXECUTABLE) -t

valgrind-test: clean $(EXECUTABLE)
	valgrind --leak-check=full ./$(EXECUTABLE) -t

run: $(EXECUTABLE)
	./$(EXECUTABLE)

.PHONY: all clean test run
