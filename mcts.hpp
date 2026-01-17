#ifndef MCTS_HPP
#define MCTS_HPP

#include <vector>
#include <memory>
#include <cmath>
#include <limits>
#include <random>
#include "environment.hpp"

enum class NodeType
{
    DECISION,  
    CHANCE     
};

class MCTSNode
{
public:
    State state;
    MoveType moveFromParent;
    MCTSNode *parent;
    std::vector<std::unique_ptr<MCTSNode>> children;

    int visits = 0;
    std::vector<double> wins;  

    std::vector<MoveType> unexpandedMoves;
    bool movedThisTurn; 

    NodeType nodeType = NodeType::DECISION;

    MCTSNode(const State &state, MCTSNode *parent, MoveType move, bool movedThisTurn, int numPlayers)
        : state(state), moveFromParent(move), parent(parent), movedThisTurn(movedThisTurn)
    {
        wins.resize(numPlayers, 0.0);
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

    double getUCB1(int playerIndex, double explorationConstant = 1.41) const
    {
        if (visits == 0)
            return std::numeric_limits<double>::infinity();

        double exploitation = wins[playerIndex] / visits;
        double exploration = explorationConstant * std::sqrt(std::log(parent->visits) / visits);

        return exploitation + exploration;
    }
};

class MCTS
{
private:
    int numPlayers;
    int iterations;
    double explorationConstant;

    std::mt19937 rng;

public:
    MCTS(int numPlayers, int iterations = 10000000, double explorationConstant = 1.41)
        : numPlayers(numPlayers), iterations(iterations), explorationConstant(explorationConstant)
    {
        std::random_device rd;
        rng = std::mt19937(rd());
    }

    MoveType findBestMove(const State &state, int playerIndex, bool movedThisTurn);

private:
    MCTSNode *select(MCTSNode *node);
    MCTSNode *expand(MCTSNode *node);
    std::vector<double> simulate(MCTSNode *node);
    void backpropagate(MCTSNode *node, const std::vector<double> &rewards);

    MCTSNode *selectBestChild(MCTSNode *node);

    std::vector<double> getRewards(const State &terminalState);

    MoveType getRandomMove(const std::vector<MoveType> &moves);
};

#endif // MCTS_HPP
