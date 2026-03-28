#ifndef HEURISTIC_BOT_HPP
#define HEURISTIC_BOT_HPP

#include "environment.hpp"

class HeuristicBot
{
private:
    int numPlayers;

public:
    HeuristicBot(int numPlayers)
        : numPlayers(numPlayers)
    {
    }

    MoveType findBestMove(const State &state, int playerIndex, bool movedThisTurn);
};

#endif // HEURISTIC_BOT_HPP
