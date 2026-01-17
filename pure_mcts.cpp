#include "pure_mcts.hpp"
#include <algorithm>
#include <limits>

double PureMCTS::rollout(State &state, bool movedThisTurn, int playerIndex)
{
    int maxSteps = 10000;
    int steps = 0;

    while (!(state.isTerminal() && state.isLastRound()) && steps < maxSteps)
    {
        std::vector<MoveType> moves = state.getPossibleMoves(movedThisTurn);

        if (moves.empty())
        {
            break;
        }

        if (moves[0] == END)
        {
            state = state.doMove(END);
            movedThisTurn = false;
            steps++;
            continue;
        }

        std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
        MoveType randomMove = moves[dist(rng)];

        state = state.doMove(randomMove);

        if (randomMove == CONTINUE || randomMove == RETURN)
        {
            movedThisTurn = true;
        }
        else
        {
            movedThisTurn = false;
        }

        steps++;
    }

    const auto &players = state.getPlayers();
    int bestScore = -1;
    int winnerIndex = 0;

    for (int i = 0; i < numPlayers; i++)
    {
        int score = players[i].getPoints();
        if (score > bestScore)
        {
            bestScore = score;
            winnerIndex = i;
        }
    }

    return (winnerIndex == playerIndex) ? 1.0 : 0.0;
}

MoveType PureMCTS::findBestMove(const State &state, int playerIndex, bool movedThisTurn)
{
    Tile::useDeterministicValues = true;

    std::vector<MoveType> moves = state.getPossibleMoves(movedThisTurn);

    if (moves.empty())
    {
        Tile::useDeterministicValues = false;
        return LEAVE_TREASURE;
    }

    if (moves.size() == 1)
    {
        Tile::useDeterministicValues = false;
        return moves[0];
    }

    size_t bestMoveIndex = 0;
    double bestWinRate = -1.0;

    for (size_t i = 0; i < moves.size(); i++)
    {
        MoveType move = moves[i];
        double totalWins = 0.0;

        bool nextMovedThisTurn;
        if (move == CONTINUE || move == RETURN)
        {
            nextMovedThisTurn = true;
        }
        else
        {
            nextMovedThisTurn = false;
        }

        for (int r = 0; r < rolloutsPerMove; r++)
        {
            State nextState = state.doMove(move);

            if (nextState.isTerminal() && nextState.isLastRound())
            {
                const auto &players = nextState.getPlayers();
                int bestScore = -1;
                int winnerIndex = 0;
                for (int p = 0; p < numPlayers; p++)
                {
                    if (players[p].getPoints() > bestScore)
                    {
                        bestScore = players[p].getPoints();
                        winnerIndex = p;
                    }
                }
                if (winnerIndex == playerIndex)
                {
                    totalWins += 1.0;
                }
            }
            else
            {
                totalWins += rollout(nextState, nextMovedThisTurn, playerIndex);
            }
        }

        double winRate = totalWins / rolloutsPerMove;
        if (winRate > bestWinRate)
        {
            bestWinRate = winRate;
            bestMoveIndex = i;
        }
    }

    Tile::useDeterministicValues = false;
    return moves[bestMoveIndex];
}
