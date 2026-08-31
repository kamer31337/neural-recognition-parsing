CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O3 -Iinclude
LDFLAGS ?= -lm

SRCDIR = src
TESTDIR = tests
OBJDIR = obj
BINDIR = bin

LIB_SRCS = $(wildcard $(SRCDIR)/*.c)
LIB_SRCS := $(filter-out $(SRCDIR)/main.c, $(LIB_SRCS))
LIB_OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(LIB_SRCS))

TEST_SRCS = $(wildcard $(TESTDIR)/*.c)
TEST_OBJS = $(patsubst $(TESTDIR)/%.c, $(OBJDIR)/%.o, $(TEST_SRCS))

CLI_TARGET = $(BINDIR)/spatter_cli
TEST_TARGET = $(BINDIR)/spatter_tests

.PHONY: all clean test run

all: $(CLI_TARGET) $(TEST_TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(TESTDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(CLI_TARGET): $(LIB_OBJS) $(OBJDIR)/main.o | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(TEST_TARGET): $(LIB_OBJS) $(TEST_OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

run: $(CLI_TARGET)
	./$(CLI_TARGET)

clean:
	rm -rf $(OBJDIR) $(BINDIR)
