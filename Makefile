# ==========================================
# Deep Sea Adventure GTest Makefile
# ==========================================

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O3 -Icpp/include
# GTest requires pthread and the gtest libraries
LDFLAGS  = -lgtest -lgtest_main -pthread

SRCDIR   = cpp/src
APPDIR   = cpp/apps
BUILDDIR = build
BINDIR   = bin

# Executable names
TARGET = $(BINDIR)/run_tests
CLI    = $(BINDIR)/deep_sea_cli
BENCH  = $(BINDIR)/benchmark
TIMING = $(BINDIR)/timing_benchmark

# Common source groups
CORE_SRCS = \
	$(SRCDIR)/environment.cpp \
	$(SRCDIR)/mcts.cpp \
	$(SRCDIR)/pure_mcts.cpp \
	$(SRCDIR)/parallel_mcts.cpp \
	$(SRCDIR)/heuristic_bot.cpp

TEST_SRCS  = $(APPDIR)/tests.cpp $(SRCDIR)/environment.cpp
CLI_SRCS   = $(APPDIR)/deep_sea_cli.cpp $(CORE_SRCS)
BENCH_SRCS = $(APPDIR)/benchmark.cpp $(SRCDIR)/environment.cpp $(SRCDIR)/pure_mcts.cpp $(SRCDIR)/heuristic_bot.cpp
TIMING_SRCS = $(APPDIR)/timing_benchmark.cpp $(SRCDIR)/environment.cpp $(SRCDIR)/mcts.cpp $(SRCDIR)/parallel_mcts.cpp

HEADERS = $(wildcard cpp/include/*.hpp)

TEST_OBJS = $(patsubst %.cpp,$(BUILDDIR)/%.o,$(TEST_SRCS))
CLI_OBJS = $(patsubst %.cpp,$(BUILDDIR)/%.o,$(CLI_SRCS))
BENCH_OBJS = $(patsubst %.cpp,$(BUILDDIR)/%.o,$(BENCH_SRCS))

TIMING_AVAILABLE := $(wildcard $(APPDIR)/timing_benchmark.cpp)
ifneq ($(TIMING_AVAILABLE),)
TIMING_OBJS = $(patsubst %.cpp,$(BUILDDIR)/%.o,$(TIMING_SRCS))
ALL_TARGETS = $(TARGET) $(CLI) $(BENCH) $(TIMING)
else
ALL_TARGETS = $(TARGET) $(CLI) $(BENCH)
endif

# Default rule: build both executables
all: $(ALL_TARGETS)

# Rule to link test executable
$(TARGET): $(TEST_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_OBJS) $(LDFLAGS)

# Rule to link CLI game executable
$(CLI): $(CLI_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(CLI_OBJS) -pthread

# Rule to link benchmark executable
$(BENCH): $(BENCH_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(BENCH_OBJS) -pthread

# Rule to link timing benchmark executable
ifneq ($(TIMING_AVAILABLE),)
$(TIMING): $(TIMING_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $(TIMING_OBJS) -pthread
endif

# Rule to compile .cpp files into .o files
$(BUILDDIR)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BINDIR):
	@mkdir -p $(BINDIR)

# Clean up build files
clean:
	rm -rf $(BUILDDIR) $(BINDIR)
	rm -f *.o run_tests deep_sea_cli benchmark timing_benchmark

# Run tests
run: $(TARGET)
	./$(TARGET)

# Play the game
play: $(CLI)
	./$(CLI)

# Run benchmark
bench: $(BENCH)
	./$(BENCH)

# Run timing benchmark
timing: $(TIMING)
	./$(TIMING)

# Phony targets
.PHONY: all clean run play bench timing
