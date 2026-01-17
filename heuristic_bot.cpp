#include "heuristic_bot.hpp"
#include <algorithm>
#include <iostream>
MoveType HeuristicBot::findBestMove(const State &state, int playerIndex, bool movedThisTurn)
{
    const Player &player = state.getPlayers()[playerIndex];
    int oxygen = state.getOxygen();
    bool isReturning = player.getIsReturning();
    int treasureCount = static_cast<int>(const_cast<Player &>(player).getTreasures().size());

    std::vector<MoveType> possibleMoves = state.getPossibleMoves(movedThisTurn);

    if (possibleMoves.empty())
    {
        std::cout << "unexpected\n";
        return LEAVE_TREASURE;
    }

    auto hasMove = [&possibleMoves](MoveType move)
    {
        return std::find(possibleMoves.begin(), possibleMoves.end(), move) != possibleMoves.end();
    };

    if (movedThisTurn)
    {

        int position = player.getPosition();
        int boardSize = static_cast<int>(state.getBoard().getTiles().size());
        // Rule 1: Never take treasure on the way down (not returning yet)
        if (!isReturning)
        {
            if (hasMove(COLLECT_TREASURE))
            {
                // Rule 2: Exception - if O2 is under 23, or you're farther than half the distance
                // and someone else took a treasure last round, take the next treasure
                if (oxygen < 23)
                {
                    return COLLECT_TREASURE;
                }

                if (position > boardSize / 2 && oxygen < 25)
                {
                    return COLLECT_TREASURE;
                }
            }
            // Otherwise, leave the treasure and continue diving
            return LEAVE_TREASURE;
        }

        // We are returning
        // Rule 4: Pick at most 1 other treasure
        if (hasMove(COLLECT_TREASURE) &&
            treasureCount < 2 &&
            oxygen > position)
        {
            return COLLECT_TREASURE;
        }
        // Rule 5: If not surviving (possibly), drop treasures (when possible)
        if (treasureCount > 1 &&
            hasMove(DROP_TREASURE) &&
            oxygen < position)
        {
            return DROP_TREASURE;
        }

        return LEAVE_TREASURE;
    }

    // decide continue/return

    if (isReturning)
    {
        return RETURN;
    }

    // Rule 3: After taking the first treasure, immediately return
    if (treasureCount > 0)
    {
        if (hasMove(RETURN))
        {
            return RETURN;
        }
    }

    if (hasMove(CONTINUE))
    {
        return CONTINUE;
    }

    // If at the bottom, return
    if (hasMove(RETURN))
    {
        return RETURN;
    }

    // fallback
    return possibleMoves[0];
}
