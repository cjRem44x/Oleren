CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g
TARGET  = olrn
VERSION = 0.1.6

SRCS = \
    src/main.c          \
    src/lexer/lexer.c   \
    src/ast/ast.c       \
    src/parser/parser.c \
    src/sema/check.c    \
    src/deps/deps.c     \
    src/module/resolver.c \
    src/codegen/codegen.c

OBJS = $(SRCS:.c=.o)
HDRS = $(wildcard src/*.h src/*/*.h)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -DOLRN_VERSION=\"$(VERSION)\" -c -o $@ $<

# compile every test -> C++ -> binary -> run; fail on first error
test: $(TARGET)
	@for t in tests/*.olrn; do \
		printf '== %s ==\n' $$t; \
		./$(TARGET) sac $$t -o=/tmp/olrn_test_bin || exit 1; \
		/tmp/olrn_test_bin || exit 1; \
	done
	@for t in tests/compile_fail/*.olrn; do \
		printf '== %s (must fail) ==\n' $$t; \
		if ./$(TARGET) sac $$t -o=/tmp/olrn_test_bin 2>/dev/null; then \
			echo "ERROR: $$t compiled but should have failed"; exit 1; \
		fi; \
	done
	@echo "all tests passed"

clean:
	rm -f $(OBJS) $(TARGET)
