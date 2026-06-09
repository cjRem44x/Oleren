CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g
TARGET  = olrn
VERSION = 0.1.0

SRCS = \
    src/main.c          \
    src/lexer/lexer.c   \
    src/ast/ast.c       \
    src/parser/parser.c \
    src/codegen/codegen.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -DOLRN_VERSION=\"$(VERSION)\" -c -o $@ $<

# compile hello_world.olrn -> C++ -> binary -> run
test: $(TARGET)
	./$(TARGET) tests/hello_world.olrn > /tmp/olrn_hw.cpp
	g++ -std=c++17 -o /tmp/olrn_hw /tmp/olrn_hw.cpp
	/tmp/olrn_hw

clean:
	rm -f $(OBJS) $(TARGET)
