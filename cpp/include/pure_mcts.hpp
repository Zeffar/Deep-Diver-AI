#ifndef PURE_MCTS_HPP
#define PURE_MCTS_HPP

#include "environment.hpp"
#include <vector>
#include <random>

class PureMCTS
{
private:
    int numPlayers;
    int rolloutsPerMove;
    std::mt19937 rng;

    double rollout(State &state, bool movedThisTurn, int playerIndex);

public:
    PureMCTS(int numPlayers, int rolloutsPerMove = 1000)
        : numPlayers(numPlayers), rolloutsPerMove(rolloutsPerMove)
    {
        std::random_device rd;
        rng = std::mt19937(rd());
    }

    MoveType findBestMove(const State &state, int playerIndex, bool movedThisTurn);
};

#endif // PURE_MCTS_HPP
