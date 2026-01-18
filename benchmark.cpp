#include <iostream>
#include <iomanip>
#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>
#include "environment.hpp"
#include "parallel_mcts.hpp"
#include "heuristic_bot.hpp"

// Player types: 0 = Parallel MCTS, 1 = Heuristic Bot
struct GameResult
{
    int mctsScore;
    int heuristicScore;
    int winner; // 0 = MCTS, 1 = Heuristic, 2 = Tie
};

MoveType getAIMove(State &state, int playerNum, int numPlayers, bool movedThisTurn,
                   ParallelMCTS &mcts, HeuristicBot &heuristic, int playerType)
{
    if (playerType == 0)
    {
        // Parallel MCTS
        return mcts.findBestMove(state, playerNum, movedThisTurn);
    }
    else
    {
        // Heuristic Bot
        return heuristic.findBestMove(state, playerNum, movedThisTurn);
    }
}

GameResult runGame(int mctsPlayerIndex, int heuristicPlayerIndex)
{
    const int numPlayers = 2;
    Tile::resetValuePools();

    State state(numPlayers);

    // Player types: mctsPlayerIndex gets MCTS (0), heuristicPlayerIndex gets Heuristic (1)
    std::vector<int> playerTypes(numPlayers);
    playerTypes[mctsPlayerIndex] = 0;
    playerTypes[heuristicPlayerIndex] = 1;

    ParallelMCTS mcts(numPlayers, 100000);
    HeuristicBot heuristic(numPlayers);

    while (true)
    {
        int currentP = state.getCurrentPlayerIndex();
        Player &player = state.getPlayers()[currentP];

        if (state.isTerminal())
        {
            if (state.isLastRound())
            {
                break;
            }
            state = state.doMove(END);
            continue;
        }

        if (player.getIsDead() || (player.getPosition() == 0 && player.getIsReturning()))
        {
            state = state.doMove(LEAVE_TREASURE);
            continue;
        }

        std::vector<MoveType> moves = state.getPossibleMoves(false);

        if (moves.empty())
        {
            state = state.doMove(LEAVE_TREASURE);
            continue;
        }

        if (moves[0] == END)
        {
            state = state.doMove(END);
            continue;
        }

        // Get AI move (first move - continue/return decision)
        MoveType chosenMove = getAIMove(state, currentP, numPlayers, false,
                                        mcts, heuristic, playerTypes[currentP]);

        int oldRound = state.getCurrentRound();
        state = state.doMove(chosenMove);
        int newRound = state.getCurrentRound();
        bool roundReset = (newRound != oldRound);

        if (state.isTerminal())
        {
            continue;
        }

        // Second decision (treasure collection/drop)
        if (state.getPlayers()[currentP].getPosition() > 0 && !roundReset)
        {
            std::vector<MoveType> actions = state.getPossibleMoves(true);

            if (!actions.empty() && actions[0] != END)
            {
                MoveType chosenAction = getAIMove(state, currentP, numPlayers, true,
                                                  mcts, heuristic, playerTypes[currentP]);
                state = state.doMove(chosenAction);
            }
        }
    }

    // Get final scores
    GameResult result;
    result.mctsScore = state.getPlayers()[mctsPlayerIndex].getPoints();
    result.heuristicScore = state.getPlayers()[heuristicPlayerIndex].getPoints();

    if (result.mctsScore > result.heuristicScore)
        result.winner = 0;
    else if (result.heuristicScore > result.mctsScore)
        result.winner = 1;
    else
        result.winner = 2;

    return result;
}

int main(int argc, char *argv[])
{
    int numGames = 100;
    if (argc > 1)
    {
        numGames = std::atoi(argv[1]);
    }

    std::cout << "Running " << numGames << " games: Parallel MCTS vs Heuristic Bot\n";
    std::cout << "=========================================================\n\n";

    std::vector<GameResult> results;
    int mctsWins = 0;
    int heuristicWins = 0;
    int ties = 0;

    std::vector<int> mctsScores;
    std::vector<int> heuristicScores;

    for (int game = 0; game < numGames; game++)
    {
        // Alternate who goes first
        int mctsPlayerIndex = game % 2;
        int heuristicPlayerIndex = 1 - mctsPlayerIndex;

        std::cout << "Game " << std::setw(3) << (game + 1) << "/" << numGames;
        std::cout << " (MCTS=P" << (mctsPlayerIndex + 1) << ", Heuristic=P" << (heuristicPlayerIndex + 1) << ")";
        std::cout.flush();

        GameResult result = runGame(mctsPlayerIndex, heuristicPlayerIndex);
        results.push_back(result);

        mctsScores.push_back(result.mctsScore);
        heuristicScores.push_back(result.heuristicScore);

        if (result.winner == 0)
            mctsWins++;
        else if (result.winner == 1)
            heuristicWins++;
        else
            ties++;

        std::cout << " | MCTS: " << std::setw(3) << result.mctsScore;
        std::cout << " | Heuristic: " << std::setw(3) << result.heuristicScore;
        std::cout << " | Winner: ";
        if (result.winner == 0)
            std::cout << "MCTS";
        else if (result.winner == 1)
            std::cout << "Heuristic";
        else
            std::cout << "Tie";
        std::cout << "\n";
    }

    // Calculate statistics
    double mctsAvg = std::accumulate(mctsScores.begin(), mctsScores.end(), 0.0) / numGames;
    double heuristicAvg = std::accumulate(heuristicScores.begin(), heuristicScores.end(), 0.0) / numGames;

    double mctsVariance = 0.0, heuristicVariance = 0.0;
    for (int i = 0; i < numGames; i++)
    {
        mctsVariance += (mctsScores[i] - mctsAvg) * (mctsScores[i] - mctsAvg);
        heuristicVariance += (heuristicScores[i] - heuristicAvg) * (heuristicScores[i] - heuristicAvg);
    }
    mctsVariance /= numGames;
    heuristicVariance /= numGames;
    double mctsStdDev = std::sqrt(mctsVariance);
    double heuristicStdDev = std::sqrt(heuristicVariance);

    int mctsMin = *std::min_element(mctsScores.begin(), mctsScores.end());
    int mctsMax = *std::max_element(mctsScores.begin(), mctsScores.end());
    int heuristicMin = *std::min_element(heuristicScores.begin(), heuristicScores.end());
    int heuristicMax = *std::max_element(heuristicScores.begin(), heuristicScores.end());

    std::cout << "\n=========================================================\n";
    std::cout << "FINAL RESULTS (" << numGames << " games)\n";
    std::cout << "=========================================================\n\n";

    std::cout << "WIN RATES:\n";
    std::cout << "  Parallel MCTS:  " << mctsWins << " wins (" << std::fixed << std::setprecision(1)
              << (100.0 * mctsWins / numGames) << "%)\n";
    std::cout << "  Heuristic Bot:  " << heuristicWins << " wins (" << std::fixed << std::setprecision(1)
              << (100.0 * heuristicWins / numGames) << "%)\n";
    std::cout << "  Ties:           " << ties << " (" << std::fixed << std::setprecision(1)
              << (100.0 * ties / numGames) << "%)\n\n";

    std::cout << "SCORE STATISTICS:\n";
    std::cout << "  Parallel MCTS:\n";
    std::cout << "    Average: " << std::fixed << std::setprecision(2) << mctsAvg << "\n";
    std::cout << "    Std Dev: " << std::fixed << std::setprecision(2) << mctsStdDev << "\n";
    std::cout << "    Min/Max: " << mctsMin << " / " << mctsMax << "\n\n";

    std::cout << "  Heuristic Bot:\n";
    std::cout << "    Average: " << std::fixed << std::setprecision(2) << heuristicAvg << "\n";
    std::cout << "    Std Dev: " << std::fixed << std::setprecision(2) << heuristicStdDev << "\n";
    std::cout << "    Min/Max: " << heuristicMin << " / " << heuristicMax << "\n";

    return 0;
}
