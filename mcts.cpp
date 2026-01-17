#include "mcts.hpp"
#include <algorithm>
#include <stdexcept>
#include <iostream> // Add at top

MoveType MCTS::findBestMove(const State &state, [[maybe_unused]] int playerIndex, bool movedThisTurn)
{
    auto moves = state.getPossibleMoves(movedThisTurn);
    if (moves.empty())
        return LEAVE_TREASURE;
    if (moves.size() == 1)
    {
        std::cerr << "[MCTS] Only 1 move available, skipping search\n";
        return moves[0];
    }
    std::cerr << "[MCTS] Running " << iterations << " iterations...\n";

    Tile::useDeterministicValues = true;

    auto root = std::make_unique<MCTSNode>(state, nullptr, LEAVE_TREASURE, movedThisTurn, numPlayers);

    for (int i = 0; i < iterations; i++)
    {
        MCTSNode *selected = select(root.get());

        MCTSNode *expanded = selected;
        if (!selected->isTerminal() && !selected->unexpandedMoves.empty())
            expanded = expand(selected);

        std::vector<double> rewards = simulate(expanded);

        backpropagate(expanded, rewards);
    }

    Tile::useDeterministicValues = false;

    MCTSNode *bestChild = nullptr;
    int bestVisits = -1;

    for (auto &child : root->children)
    {
        if (child->visits > bestVisits)
        {
            bestVisits = child->visits;
            bestChild = child.get();
        }
    }

    if (bestChild == nullptr)
    {
        auto moves = state.getPossibleMoves(movedThisTurn);
        return moves.empty() ? LEAVE_TREASURE : moves[0];
    }

    return bestChild->moveFromParent;
}

MCTSNode *MCTS::select(MCTSNode *node)
{
    while (!node->isTerminal())
    {
        if (!node->isFullyExpanded())
            return node;

        if (node->children.empty())
            return node;

        node = selectBestChild(node);
    }

    return node;
}

MCTSNode *MCTS::selectBestChild(MCTSNode *node)
{
    int currentPlayer = node->state.getCurrentPlayerIndex();

    MCTSNode *best = nullptr;
    double bestScore = -std::numeric_limits<double>::infinity();

    for (auto &child : node->children)
    {
        double score = child->getUCB1(currentPlayer, explorationConstant);
        if (score > bestScore)
        {
            bestScore = score;
            best = child.get();
        }
    }

    return best;
}

MCTSNode *MCTS::expand(MCTSNode *node)
{
    if (node->unexpandedMoves.empty())
        return node;

    std::uniform_int_distribution<size_t> dist(0, node->unexpandedMoves.size() - 1);
    size_t moveIndex = dist(rng);
    MoveType move = node->unexpandedMoves[moveIndex];

    node->unexpandedMoves.erase(node->unexpandedMoves.begin() + moveIndex);

    State newState = node->state.doMove(move);

    bool newMovedThisTurn;

    if (move == CONTINUE || move == RETURN)
    {
        if (newState.getCurrentPlayerIndex() != node->state.getCurrentPlayerIndex())
            newMovedThisTurn = false;
        else
            newMovedThisTurn = true;
    }
    else
        newMovedThisTurn = false;

    auto child = std::make_unique<MCTSNode>(newState, node, move, newMovedThisTurn, numPlayers);
    MCTSNode *childPtr = child.get();
    node->children.push_back(std::move(child));

    return childPtr;
}

std::vector<double> MCTS::simulate(MCTSNode *node)
{
    State simState = node->state;
    bool movedThisTurn = node->movedThisTurn;

    int maxSteps = 100000;
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
            if (newPlayer != prevPlayer)
                movedThisTurn = false;
            else
                movedThisTurn = true;
        }
        else
            movedThisTurn = false;

        steps++;
    }

    return getRewards(simState);
}

void MCTS::backpropagate(MCTSNode *node, const std::vector<double> &rewards)
{
    while (node != nullptr)
    {
        node->visits++;

        for (int i = 0; i < numPlayers; i++)
            node->wins[i] += rewards[i];

        node = node->parent;
    }
}

std::vector<double> MCTS::getRewards(const State &terminalState)
{
    std::vector<double> rewards(numPlayers, 0.0);

    int maxPoints = -1;
    for (int i = 0; i < numPlayers; i++)
    {
        int points = terminalState.getPlayers()[i].getPoints();
        if (points > maxPoints)
            maxPoints = points;
    }

    for (int i = 0; i < numPlayers; i++)
        if (terminalState.getPlayers()[i].getPoints() == maxPoints)
            rewards[i] = 1.0;

    return rewards;
}

MoveType MCTS::getRandomMove(const std::vector<MoveType> &moves)
{
    if (moves.empty())
        return LEAVE_TREASURE;

    std::uniform_int_distribution<size_t> dist(0, moves.size() - 1);
    return moves[dist(rng)];
}
