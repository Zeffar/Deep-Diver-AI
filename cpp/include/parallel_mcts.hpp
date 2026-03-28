#ifndef PARALLEL_MCTS_HPP
#define PARALLEL_MCTS_HPP

#include <vector>
#include <memory>
#include <cmath>
#include <limits>
#include <random>
#include <thread>
#include <atomic>
#include <array>
#include "environment.hpp"

constexpr int MAX_PLAYERS = 6;

class NodePool;

class ParallelMCTSNode
{
public:
    State state;
    MoveType moveFromParent;
    ParallelMCTSNode *parent;

    ParallelMCTSNode **children;
    int childCount;
    int childCapacity;

    int visits;
    std::array<double, MAX_PLAYERS> wins;
    int numPlayers;

    std::vector<MoveType> unexpandedMoves;
    bool movedThisTurn;

    double logVisits;

    ParallelMCTSNode()
        : state(1), parent(nullptr), children(nullptr), childCount(0), childCapacity(0),
          visits(0), numPlayers(0), movedThisTurn(false), logVisits(0.0)
    {
        wins.fill(0.0);
    }

    void init(const State &s, ParallelMCTSNode *p, MoveType move, bool moved, int nPlayers)
    {
        state = s;
        moveFromParent = move;
        parent = p;
        childCount = 0;
        visits = 0;
        wins.fill(0.0);
        numPlayers = nPlayers;
        movedThisTurn = moved;
        logVisits = 0.0;
        unexpandedMoves = state.getPossibleMoves(movedThisTurn);
    }

    bool isFullyExpanded() const
    {
        return unexpandedMoves.empty();
    }

    bool isTerminal() const
    {
        return state.isTerminal() && state.isLastRound();
    }

    double getUCB1(int playerIndex, double explorationConstant, double parentLogVisits) const
    {
        if (visits == 0)
            return std::numeric_limits<double>::infinity();

        double exploitation = wins[playerIndex] / visits;
        double exploration = explorationConstant * std::sqrt(parentLogVisits / visits);

        return exploitation + exploration;
    }

    void updateLogVisits()
    {
        if (visits > 0)
            logVisits = std::log(static_cast<double>(visits));
    }
};

class NodePool
{
private:
    std::vector<ParallelMCTSNode> nodes;
    std::vector<ParallelMCTSNode *> childArrayPool;
    size_t nextNode;
    size_t nextChildArray;
    static constexpr int CHILD_ARRAY_SIZE = 8;

public:
    NodePool(size_t capacity = 1000000)
        : nextNode(0), nextChildArray(0)
    {
        nodes.resize(capacity);
        childArrayPool.resize(capacity * CHILD_ARRAY_SIZE);
    }

    void reset()
    {
        nextNode = 0;
        nextChildArray = 0;
    }

    ParallelMCTSNode *allocateNode()
    {
        if (nextNode >= nodes.size())
        {
            nodes.resize(nodes.size() * 2);
            childArrayPool.resize(childArrayPool.size() * 2);
        }
        ParallelMCTSNode *node = &nodes[nextNode++];

        node->children = &childArrayPool[nextChildArray];
        node->childCapacity = CHILD_ARRAY_SIZE;
        nextChildArray += CHILD_ARRAY_SIZE;

        return node;
    }

    size_t getUsedNodes() const { return nextNode; }
};

struct MoveStats
{
    MoveType move;
    int totalVisits;
    double totalWins;

    MoveStats() : move(LEAVE_TREASURE), totalVisits(0), totalWins(0.0) {}
    MoveStats(MoveType m) : move(m), totalVisits(0), totalWins(0.0) {}
};

class MCTSWorker
{
private:
    int numPlayers;
    int iterations;
    double explorationConstant;
    std::mt19937 rng;
    NodePool pool;

    std::uniform_int_distribution<size_t> dist;

public:
    MCTSWorker(int numPlayers, int iterations, double explorationConstant, unsigned int seed)
        : numPlayers(numPlayers), iterations(iterations),
          explorationConstant(explorationConstant), rng(seed),
          pool(std::max(100000, iterations / 10))
    {
    }

    std::vector<MoveStats> search(const State &state, int playerIndex, bool movedThisTurn);

private:
    ParallelMCTSNode *select(ParallelMCTSNode *node);
    ParallelMCTSNode *expand(ParallelMCTSNode *node);
    std::array<double, MAX_PLAYERS> simulate(ParallelMCTSNode *node);
    void backpropagate(ParallelMCTSNode *node, const std::array<double, MAX_PLAYERS> &rewards);
    ParallelMCTSNode *selectBestChild(ParallelMCTSNode *node);
    std::array<double, MAX_PLAYERS> getRewards(const State &terminalState);
    MoveType getRandomMove(const std::vector<MoveType> &moves);
};

class ParallelMCTS
{
private:
    int numPlayers;
    int iterationsPerThread;
    int numThreads;
    double explorationConstant;

public:
    ParallelMCTS(int numPlayers, int totalIterations = 10000000,
                 double explorationConstant = 1.41, int numThreads = 0)
        : numPlayers(numPlayers), explorationConstant(explorationConstant)
    {
        if (numThreads <= 0)
            this->numThreads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
        else
            this->numThreads = numThreads;

        this->iterationsPerThread = totalIterations / this->numThreads;
    }

    MoveType findBestMove(const State &state, int playerIndex, bool movedThisTurn);

    int getNumThreads() const { return numThreads; }
};

#endif // PARALLEL_MCTS_HPP
