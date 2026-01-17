#include "parallel_mcts.hpp"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <future>
#include <numeric>
#include <unordered_map>

std::vector<MoveStats> MCTSWorker::search(const State &state, int playerIndex, bool movedThisTurn)
{
    pool.reset();

    ParallelMCTSNode *root = pool.allocateNode();
    root->init(state, nullptr, LEAVE_TREASURE, movedThisTurn, numPlayers);

    for (int i = 0; i < iterations; i++)
    {
        ParallelMCTSNode *selected = select(root);

        ParallelMCTSNode *expanded = selected;
        if (!selected->isTerminal() && !selected->unexpandedMoves.empty())
            expanded = expand(selected);

        std::array<double, MAX_PLAYERS> rewards = simulate(expanded);
        backpropagate(expanded, rewards);
    }

    std::vector<MoveStats> results;
    for (int i = 0; i < root->childCount; i++)
    {
        ParallelMCTSNode *child = root->children[i];
        MoveStats stats(child->moveFromParent);
        stats.totalVisits = child->visits;
        stats.totalWins = child->wins[playerIndex];
        results.push_back(stats);
    }

    return results;
}

ParallelMCTSNode *MCTSWorker::select(ParallelMCTSNode *node)
{
    while (!node->isTerminal())
    {
        if (!node->isFullyExpanded())
            return node;

        if (node->childCount == 0)
            return node;

        node = selectBestChild(node);
    }

    return node;
}

ParallelMCTSNode *MCTSWorker::selectBestChild(ParallelMCTSNode *node)
{
    int currentPlayer = node->state.getCurrentPlayerIndex();

    double parentLogVisits = node->logVisits;

    ParallelMCTSNode *best = nullptr;
    double bestScore = -std::numeric_limits<double>::infinity();

    for (int i = 0; i < node->childCount; i++)
    {
        ParallelMCTSNode *child = node->children[i];
        double score = child->getUCB1(currentPlayer, explorationConstant, parentLogVisits);
        if (score > bestScore)
        {
            bestScore = score;
            best = child;
        }
    }

    return best;
}

ParallelMCTSNode *MCTSWorker::expand(ParallelMCTSNode *node)
{
    if (node->unexpandedMoves.empty())
        return node;

    size_t moveIndex;
    if (node->unexpandedMoves.size() == 1)
    {
        moveIndex = 0;
    }
    else
    {
        dist.param(std::uniform_int_distribution<size_t>::param_type(0, node->unexpandedMoves.size() - 1));
        moveIndex = dist(rng);
    }

    MoveType move = node->unexpandedMoves[moveIndex];

    if (moveIndex != node->unexpandedMoves.size() - 1)
        std::swap(node->unexpandedMoves[moveIndex], node->unexpandedMoves.back());
    node->unexpandedMoves.pop_back();

    State newState = node->state.doMove(move);

    bool newMovedThisTurn;
    if (move == CONTINUE || move == RETURN)
    {
        newMovedThisTurn = (newState.getCurrentPlayerIndex() == node->state.getCurrentPlayerIndex());
    }
    else
    {
        newMovedThisTurn = false;
    }

    ParallelMCTSNode *child = pool.allocateNode();
    child->init(newState, node, move, newMovedThisTurn, numPlayers);

    if (node->childCount < node->childCapacity)
    {
        node->children[node->childCount++] = child;
    }

    return child;
}

std::array<double, MAX_PLAYERS> MCTSWorker::simulate(ParallelMCTSNode *node)
{
    State simState = node->state;
    bool movedThisTurn = node->movedThisTurn;

    int maxSteps = 500; 
    int steps = 0;

    while (!(simState.isTerminal() && simState.isLastRound()) && steps < maxSteps)
    {
        std::vector<MoveType> moves = simState.getPossibleMoves(movedThisTurn);

        if (moves.empty())
            break;

        if (moves[0] == END)
        {
            simState = simState.doMove(END);
            movedThisTurn = false;
            steps++;
            continue;
        }

        MoveType move = getRandomMove(moves);

        int prevPlayer = simState.getCurrentPlayerIndex();
        simState = simState.doMove(move);
        int newPlayer = simState.getCurrentPlayerIndex();

        if (move == CONTINUE || move == RETURN)
        {
            movedThisTurn = (newPlayer == prevPlayer);
        }
        else
        {
            movedThisTurn = false;
        }

        steps++;
    }

    return getRewards(simState);
}

void MCTSWorker::backpropagate(ParallelMCTSNode *node, const std::array<double, MAX_PLAYERS> &rewards)
{
    while (node != nullptr)
    {
        node->visits++;
        node->updateLogVisits();

        for (int i = 0; i < numPlayers; i++)
            node->wins[i] += rewards[i];

        node = node->parent;
    }
}

std::array<double, MAX_PLAYERS> MCTSWorker::getRewards(const State &terminalState)
{
    std::array<double, MAX_PLAYERS> rewards;
    rewards.fill(0.0);

    const auto &players = terminalState.getPlayers();

    std::array<int, MAX_PLAYERS> scores;
    int maxScore = 0;
    int minScore = std::numeric_limits<int>::max();

    for (int i = 0; i < numPlayers; i++)
    {
        scores[i] = players[i].getPoints();
        maxScore = std::max(maxScore, scores[i]);
        minScore = std::min(minScore, scores[i]);
    }

    int scoreRange = maxScore - minScore;

    if (scoreRange == 0)
    {
        double equalReward = 1.0 / numPlayers;
        for (int i = 0; i < numPlayers; i++)
            rewards[i] = equalReward;
    }
    else
    {
        for (int i = 0; i < numPlayers; i++)
        {
            rewards[i] = static_cast<double>(scores[i] - minScore) / scoreRange;
        }
    }

    return rewards;
}

MoveType MCTSWorker::getRandomMove(const std::vector<MoveType> &moves)
{
    if (moves.empty())
        return LEAVE_TREASURE;

    if (moves.size() == 1)
        return moves[0];

    dist.param(std::uniform_int_distribution<size_t>::param_type(0, moves.size() - 1));
    return moves[dist(rng)];
}

MoveType ParallelMCTS::findBestMove(const State &state, int playerIndex, bool movedThisTurn)
{
    auto moves = state.getPossibleMoves(movedThisTurn);

    if (moves.empty())
        return LEAVE_TREASURE;

    if (moves.size() == 1)
    {
        std::cerr << "[ParallelMCTS] Only 1 move available, skipping search\n";
        return moves[0];
    }

    std::cerr << "[ParallelMCTS] Running " << (iterationsPerThread * numThreads)
              << " iterations across " << numThreads << " threads...\n";

    Tile::useDeterministicValues = true;

    std::vector<std::future<std::vector<MoveStats>>> futures;
    std::random_device rd;

    for (int t = 0; t < numThreads; t++)
    {
        unsigned int seed = rd() ^ (t * 0x9E3779B9);

        futures.push_back(std::async(std::launch::async, [=]()
                                     {
            MCTSWorker worker(numPlayers, iterationsPerThread, explorationConstant, seed);
            return worker.search(state, playerIndex, movedThisTurn); }));
    }

    std::unordered_map<MoveType, MoveStats> aggregated;

    for (MoveType m : moves)
    {
        aggregated[m] = MoveStats(m);
    }

    for (auto &future : futures)
    {
        std::vector<MoveStats> workerStats = future.get();
        for (const auto &stat : workerStats)
        {
            aggregated[stat.move].totalVisits += stat.totalVisits;
            aggregated[stat.move].totalWins += stat.totalWins;
        }
    }

    Tile::useDeterministicValues = false;

    MoveType bestMove = LEAVE_TREASURE;
    int bestVisits = -1;
    double bestWinRate = -1.0;

    std::cerr << "[ParallelMCTS] Move statistics:\n";
    for (const auto &[move, stats] : aggregated)
    {
        double winRate = stats.totalVisits > 0 ? stats.totalWins / stats.totalVisits : 0.0;

        const char *moveName;
        switch (move)
        {
        case CONTINUE:
            moveName = "CONTINUE";
            break;
        case RETURN:
            moveName = "RETURN";
            break;
        case COLLECT_TREASURE:
            moveName = "COLLECT";
            break;
        case LEAVE_TREASURE:
            moveName = "LEAVE";
            break;
        case DROP_TREASURE:
            moveName = "DROP";
            break;
        case END:
            moveName = "END";
            break;
        default:
            moveName = "UNKNOWN";
        }

        std::cerr << "  " << moveName << ": visits=" << stats.totalVisits
                  << ", winRate=" << (winRate * 100.0) << "%\n";

        if (stats.totalVisits > bestVisits ||
            (stats.totalVisits == bestVisits && winRate > bestWinRate))
        {
            bestVisits = stats.totalVisits;
            bestWinRate = winRate;
            bestMove = move;
        }
    }

    return bestMove;
}
