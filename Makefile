# ==========================================
# Deep Sea Adventure GTest Makefile
# ==========================================

CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O3
# GTest requires pthread and the gtest libraries
LDFLAGS  = -lgtest -lgtest_main -pthread

# Executable names
TARGET = run_tests
CLI    = deep_sea_cli
BENCH  = benchmark
TIMING = timing_benchmark

# Source files
TEST_SRCS = tests.cpp environment.cpp
CLI_SRCS  = deep_sea_cli.cpp environment.cpp mcts.cpp pure_mcts.cpp parallel_mcts.cpp heuristic_bot.cpp
BENCH_SRCS = benchmark.cpp environment.cpp pure_mcts.cpp heuristic_bot.cpp
TIMING_SRCS = timing_benchmark.cpp environment.cpp mcts.cpp parallel_mcts.cpp

# Object files
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
CLI_OBJ   = deep_sea_cli.o
ENV_OBJ   = environment.o
MCTS_OBJ  = mcts.o
PURE_MCTS_OBJ = pure_mcts.o
PARALLEL_MCTS_OBJ = parallel_mcts.o
HEURISTIC_BOT_OBJ = heuristic_bot.o

HEADERS     = environment.hpp mcts.hpp pure_mcts.hpp parallel_mcts.hpp heuristic_bot.hpp

# Default rule: build both executables
all: $(TARGET) $(CLI) $(BENCH) $(TIMING)

# Rule to link test executable
$(TARGET): tests.o environment.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) tests.o environment.o $(LDFLAGS)

# Rule to link CLI game executable
$(CLI): deep_sea_cli.o environment.o mcts.o pure_mcts.o parallel_mcts.o heuristic_bot.o
	$(CXX) $(CXXFLAGS) -o $(CLI) deep_sea_cli.o environment.o mcts.o pure_mcts.o parallel_mcts.o heuristic_bot.o -pthread

# Rule to link benchmark executable
$(BENCH): benchmark.o environment.o pure_mcts.o heuristic_bot.o
	$(CXX) $(CXXFLAGS) -o $(BENCH) benchmark.o environment.o pure_mcts.o heuristic_bot.o -pthread

# Rule to link timing benchmark executable
$(TIMING): timing_benchmark.o environment.o mcts.o parallel_mcts.o
	$(CXX) $(CXXFLAGS) -o $(TIMING) timing_benchmark.o environment.o mcts.o parallel_mcts.o -pthread

# Rule to compile .cpp files into .o files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f *.o $(TARGET) $(CLI) $(BENCH)

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
