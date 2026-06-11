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

# compile every test -> C++ -> binary -> run; fail on first error
test: $(TARGET)
	@for t in tests/*.olrn; do \
		printf '== %s ==\n' $$t; \
		./$(TARGET) sac $$t -o=/tmp/olrn_test_bin || exit 1; \
		/tmp/olrn_test_bin || exit 1; \
	done
	@echo "all tests passed"

clean:
	rm -f $(OBJS) $(TARGET)
