#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <limits>
#include <sstream>
#include "environment.hpp"
#include "mcts.hpp"
#include "pure_mcts.hpp"
#include "parallel_mcts.hpp"
#include "heuristic_bot.hpp"

// Player types: 0 = Human, 1 = MCTS AI, 2 = Pure MCTS AI, 3 = Parallel MCTS AI, 4 = Heuristic Bot
std::vector<int> playerTypes;
std::vector<HeuristicBot *> heuristicBots; // Track heuristic bots for state management

namespace Color
{
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    const std::string BG_BLUE = "\033[44m";
}

const std::vector<std::string> PLAYER_SYMBOLS = {"1", "2", "3", "4", "5", "6"};
const std::vector<std::string> PLAYER_COLORS = {
    Color::RED, Color::GREEN, Color::YELLOW,
    Color::BLUE, Color::MAGENTA, Color::CYAN};

void clearScreen()
{
    std::cout << "\033[2J\033[H";
}

void pressEnterToContinue()
{
    std::cout << Color::CYAN << "\nENTER to continue..." << Color::RESET;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void printOxygenBar(int oxygen)
{
    std::cout << "\n  " << Color::BOLD << Color::CYAN << "OXYGEN: " << Color::RESET;
    std::cout << "[";

    for (int i = 0; i < 25; i++)
    {
        if (i < oxygen)
        {
            if (oxygen > 15)
                std::cout << Color::GREEN << "#";
            else if (oxygen > 7)
                std::cout << Color::YELLOW << "#";
            else
                std::cout << Color::RED << "#";
        }
        else
        {
            std::cout << Color::WHITE << ".";
        }
    }
    std::cout << Color::RESET << "] " << oxygen << "/25\n";
}

void printBoard(State &state, int numPlayers)
{
    Board &board = state.getBoard();
    std::vector<Tile> &tiles = board.getTiles();
    int boardSize = static_cast<int>(tiles.size());

    std::cout << "\n";

    std::cout << "  " << Color::BG_BLUE << Color::WHITE << Color::BOLD;
    std::cout << " [SUBMARINE] ";

    std::cout << "Players: ";
    bool hasPlayersAtSub = false;
    for (int p = 0; p < numPlayers; p++)
    {
        Player &player = state.getPlayers()[p];
        if (player.getPosition() == 0)
        {
            std::cout << PLAYER_COLORS[p] << "P" << PLAYER_SYMBOLS[p] << " ";
            hasPlayersAtSub = true;
        }
    }
    if (!hasPlayersAtSub)
        std::cout << "none";
    std::cout << Color::RESET << "\n";

    std::cout << "\n  " << Color::BOLD << "THE OCEAN DEPTHS:" << Color::RESET << "\n";
    std::cout << "  +";
    for (int i = 0; i < boardSize; i++)
        std::cout << "---";
    std::cout << "+\n";

    std::cout << "  |";
    for (int i = 1; i <= boardSize; i++)
    {
        std::cout << std::setw(2) << i << " ";
    }
    std::cout << "|\n";

    std::cout << "  |";
    for (int i = 0; i < boardSize; i++)
    {
        int level = tiles[i].level;
        std::string color;
        switch (level)
        {
        case 0:
            color = Color::WHITE;
            break;
        case 1:
            color = Color::CYAN;
            break;
        case 2:
            color = Color::YELLOW;
            break;
        case 3:
            color = Color::MAGENTA;
            break;
        case 4:
            color = Color::RED; // Fallen treasure from drowned players
            break;
        default:
            color = Color::RESET;
        }

        char symbol = tiles[i].isFlipped() ? 'o' : '*';
        if (level == 4)
        {
            // For level 4 (fallen treasure), show the sum of contained chip levels
            int valueSum = 0;
            for (int chipLevel : tiles[i].treasure)
            {
                valueSum += chipLevel;
            }
            std::cout << color << " " << symbol << valueSum << Color::RESET;
        }
        else
        {
            std::cout << color << " " << symbol << level << Color::RESET;
        }
    }
    std::cout << "|\n";

    // Print player positions
    std::cout << "  |";
    for (int i = 1; i <= boardSize; i++)
    {
        bool hasPlayer = false;
        for (int p = 0; p < numPlayers; p++)
        {
            if (state.getPlayers()[p].getPosition() == i)
            {
                std::cout << PLAYER_COLORS[p] << " P" << PLAYER_SYMBOLS[p] << Color::RESET;
                hasPlayer = true;
                break;
            }
        }
        if (!hasPlayer)
        {
            std::cout << "   ";
        }
    }
    std::cout << "|\n";

    std::cout << "  +";
    for (int i = 0; i < boardSize; i++)
        std::cout << "---";
    std::cout << "+\n";

    // Legend
    std::cout << "  " << Color::BOLD << "Legend:" << Color::RESET << " ";
    std::cout << "*=treasure, o=collected | ";
    std::cout << Color::WHITE << "L0(0-3pts)" << Color::RESET << " ";
    std::cout << Color::CYAN << "L1(4-7pts)" << Color::RESET << " ";
    std::cout << Color::YELLOW << "L2(8-11pts)" << Color::RESET << " ";
    std::cout << Color::MAGENTA << "L3(12-15pts)" << Color::RESET << " ";
    std::cout << Color::RED << "L4=fallen(sum of chips)" << Color::RESET << "\n";
}

void printPlayerStatus(State &state, int numPlayers, int currentPlayer)
{
    std::cout << "  | " << Color::BOLD << "PLAYER STATUS" << Color::RESET
              << "  (Round " << (state.getCurrentRound() + 1) << "/3) |\n";

    for (int p = 0; p < numPlayers; p++)
    {
        Player &player = state.getPlayers()[p];

        std::string marker = (p == currentPlayer) ? ">>>" : "   ";
        std::string status;

        if (player.getIsDead())
        {
            status = Color::RED + "DROWNED" + Color::RESET;
        }
        else if (player.getPosition() == 0 && player.getIsReturning())
        {
            status = Color::GREEN + "SAFE" + Color::RESET;
        }
        else if (player.getIsReturning())
        {
            status = Color::YELLOW + "RETURNING" + Color::RESET;
        }
        else
        {
            status = Color::CYAN + "DIVING" + Color::RESET;
        }

        int treasureCount = static_cast<int>(player.getTreasures().size());

        std::cout << "  | " << marker << " " << PLAYER_COLORS[p]
                  << "Player " << (p + 1) << Color::RESET;
        std::cout << " | Pos:" << std::setw(2) << player.getPosition();
        std::cout << " | Treasure:" << treasureCount;
        std::cout << " | Score:" << std::setw(3) << player.getPoints();
        std::cout << " | " << std::setw(9) << status;
        std::cout << " |\n";
    }

    std::cout << "  +---------------------------------------------------------+\n";
}

std::string moveTypeToString(MoveType move)
{
    switch (move)
    {
    case CONTINUE:
        return "DIVE DEEPER (roll dice, move forward)";
    case RETURN:
        return "TURN BACK (roll dice, head to submarine)";
    case COLLECT_TREASURE:
        return "COLLECT TREASURE from this tile";
    case LEAVE_TREASURE:
        return "PASS";
    case DROP_TREASURE:
        return "DROP LOWEST TREASURE";
    case END:
        return "END";
    default:
        return "UNKNOWN";
    }
}

MoveType getAIMove(State &state, int playerNum, int numPlayers, bool movedThisTurn)
{
    int aiType = playerTypes[playerNum];

    if (aiType == 1)
    {
        // Full MCTS
        std::cout << "\n  " << PLAYER_COLORS[playerNum] << Color::BOLD
                  << "=== AI Player " << (playerNum + 1) << " (MCTS) is thinking... ===" << Color::RESET << "\n";

        MCTS mcts(numPlayers, 50000); // 50k iterations
        MoveType bestMove = mcts.findBestMove(state, playerNum, movedThisTurn);

        std::cout << "  AI chooses: " << moveTypeToString(bestMove) << "\n";
        return bestMove;
    }
    else if (aiType == 2)
    {
        // Pure MCTS (flat Monte Carlo)
        std::cout << "\n  " << PLAYER_COLORS[playerNum] << Color::BOLD
                  << "=== AI Player " << (playerNum + 1) << " (Pure MC) is thinking... ===" << Color::RESET << "\n";

        PureMCTS pureMcts(numPlayers, 10000); // 10k rollouts per move
        MoveType bestMove = pureMcts.findBestMove(state, playerNum, movedThisTurn);

        std::cout << "  AI chooses: " << moveTypeToString(bestMove) << "\n";
        return bestMove;
    }
    else if (aiType == 3)
    {
        // Parallel MCTS
        std::cout << "\n  " << PLAYER_COLORS[playerNum] << Color::BOLD
                  << "=== AI Player " << (playerNum + 1) << " (Parallel MCTS) is thinking... ===" << Color::RESET << "\n";

        ParallelMCTS parallelMcts(numPlayers, 200000); // 200k total iterations across threads
        MoveType bestMove = parallelMcts.findBestMove(state, playerNum, movedThisTurn);

        std::cout << "  AI chooses: " << moveTypeToString(bestMove) << "\n";
        return bestMove;
    }
    else if (aiType == 4)
    {
        // Heuristic Bot
        std::cout << "\n  " << PLAYER_COLORS[playerNum] << Color::BOLD
                  << "=== AI Player " << (playerNum + 1) << " (Heuristic Bot) deciding... ===" << Color::RESET << "\n";

        MoveType bestMove = heuristicBots[playerNum]->findBestMove(state, playerNum, movedThisTurn);

        std::cout << "  AI chooses: " << moveTypeToString(bestMove) << "\n";
        return bestMove;
    }

    // fallback
    return LEAVE_TREASURE;
}

int getPlayerChoice(std::vector<MoveType> &moves, int playerNum)
{
    std::cout << "\n  " << PLAYER_COLORS[playerNum] << Color::BOLD
              << "=== Player " << (playerNum + 1) << "'s turn! ===" << Color::RESET << "\n\n";
    std::cout << "  Available actions:\n";

    for (size_t i = 0; i < moves.size(); i++)
    {
        std::cout << "    [" << (i + 1) << "] " << moveTypeToString(moves[i]) << "\n";
    }

    int choice = 0;
    while (choice < 1 || choice > static_cast<int>(moves.size()))
    {
        std::cout << "\n  Enter choice (1-" << moves.size() << "): ";
        std::cin >> choice;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            choice = 0;
        }
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    return choice - 1;
}

void printRoundStart(int round)
{
    clearScreen();
    std::cout << Color::BOLD << Color::CYAN;
    std::cout << "  |          ROUND " << (round + 1) << " OF 3              |\n";
    std::cout << Color::RESET;
    std::cout << "\n  All divers start at the submarine with 25 oxygen.\n";
    std::cout << "  Dive deep, grab treasure, but return before air runs out!\n";
}

void printGameOver(State &state, int numPlayers)
{
    clearScreen();
    std::cout << Color::BOLD << Color::YELLOW;
    std::cout << R"(
    +===============================================================+
    |                                                               |
    |     ####    ###   ##   ## #####        ###   ##   ## ##### ####|
    |    ##      ## ##  ### ### ##          ## ##  ##   ## ##    ##  |
    |    ## ### ###### ## # ## ####        ##   ## ### ## #### ###   |
    |    ##  ## ##  ## ##   ## ##          ##   ##  ## ##  ##    ##  |
    |     ####  ##  ## ##   ## #####        #####    ###   ##### ####|
    |                                                               |
    +===============================================================+
)" << Color::RESET
              << "\n";

    std::cout << "  " << Color::BOLD << "FINAL SCORES:" << Color::RESET << "\n";
    std::cout << "  =========================================\n";

    // Find winner
    int maxPoints = -1;
    int winner = 0;
    for (int p = 0; p < numPlayers; p++)
    {
        if (state.getPlayers()[p].getPoints() > maxPoints)
        {
            maxPoints = state.getPlayers()[p].getPoints();
            winner = p;
        }
    }

    // Print scores
    for (int p = 0; p < numPlayers; p++)
    {
        Player &player = state.getPlayers()[p];
        std::string trophy = (p == winner) ? " <-- WINNER!" : "";
        std::cout << "  " << PLAYER_COLORS[p]
                  << "Player " << (p + 1) << ": " << Color::BOLD
                  << player.getPoints() << " points" << Color::RESET
                  << Color::YELLOW << trophy << Color::RESET << "\n";
    }
}

void runGame(int numPlayers)
{
    Tile::resetValuePools();

    State state(numPlayers);
    int lastRound = -1;

    while (true)
    {
        int currentRound = state.getCurrentRound();
        if (currentRound != lastRound)
        {
            printRoundStart(currentRound);
            lastRound = currentRound;
            pressEnterToContinue();
        }

        int currentP = state.getCurrentPlayerIndex();
        Player &player = state.getPlayers()[currentP];

        clearScreen();
        printOxygenBar(state.getOxygen());
        printBoard(state, numPlayers);
        printPlayerStatus(state, numPlayers, currentP);

        if (state.isTerminal())
        {
            if (state.isLastRound())
            {
                break;
            }

            if (state.getOxygen() == 0)
            {
                std::cout << "\n  " << Color::RED << Color::BOLD
                          << "!!! OXYGEN DEPLETED! Round ended!"
                          << Color::RESET << "\n";

                for (int p = 0; p < numPlayers; p++)
                {
                    Player &pl = state.getPlayers()[p];
                    if (pl.getPosition() != 0 || pl.getIsDead())
                    {
                        std::cout << "  " << PLAYER_COLORS[p] << "Player " << (p + 1)
                                  << " drowned and lost their treasure!" << Color::RESET << "\n";
                    }
                }
            }
            else
            {
                std::cout << "\n  " << Color::GREEN << Color::BOLD
                          << "=== ALL PLAYERS RETURNED SAFELY! Round ended! ==="
                          << Color::RESET << "\n";
            }

            pressEnterToContinue();

            state = state.doMove(END);
            continue;
        }

        if (player.getIsDead() || (player.getPosition() == 0 && player.getIsReturning()))
        {
            std::cout << "\n  " << PLAYER_COLORS[currentP] << "Player " << (currentP + 1)
                      << " is safe in the submarine!" << Color::RESET << "\n";
            state = state.doMove(LEAVE_TREASURE);
            pressEnterToContinue();
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

        int treasureWeight = static_cast<int>(player.getTreasures().size());
        if (treasureWeight > 0)
        {
            std::cout << "\n  " << Color::YELLOW << "Warning: Carrying " << treasureWeight
                      << " treasure(s) - will cost " << treasureWeight
                      << " oxygen when you move!" << Color::RESET << "\n";
        }

        MoveType chosenMove;
        if (playerTypes[currentP] != 0)
        {
            chosenMove = getAIMove(state, currentP, numPlayers, false);
        }
        else
        {
            int choice = getPlayerChoice(moves, currentP);
            chosenMove = moves[choice];
        }

        int oldPos = player.getPosition();
        int oldRound = state.getCurrentRound();
        state = state.doMove(chosenMove);
        int newRound = state.getCurrentRound();
        bool roundReset = (newRound != oldRound);

        int newPos = roundReset ? 0 : state.getPlayers()[currentP].getPosition();

        if (chosenMove == CONTINUE || chosenMove == RETURN)
        {
            if (roundReset)
            {
                std::cout << "\n  Round ended! Either O2 ran out or everyone made it back safely!";
            }
            else
            {
                std::cout << "\n  Dice rolled! Moved from position " << oldPos
                          << " to position " << newPos;
                if (treasureWeight > 0)
                {
                    std::cout << " (slowed by " << treasureWeight << " treasure)";
                }
                std::cout << "\n";
            }
        }

        if (state.isTerminal())
        {
            continue;
        }

        if (state.getPlayers()[currentP].getPosition() > 0)
        {
            std::vector<MoveType> actions = state.getPossibleMoves(true);

            if (!actions.empty() && actions[0] != END)
            {
                clearScreen();
                printOxygenBar(state.getOxygen());
                printBoard(state, numPlayers);
                printPlayerStatus(state, numPlayers, currentP);

                MoveType chosenAction;
                if (playerTypes[currentP] != 0)
                {
                    chosenAction = getAIMove(state, currentP, numPlayers, true);
                }
                else
                {
                    int actionChoice = getPlayerChoice(actions, currentP);
                    chosenAction = actions[actionChoice];
                }

                if (chosenAction == COLLECT_TREASURE)
                {
                    int pos = state.getPlayers()[currentP].getPosition();
                    int level = state.getBoard().getTiles()[pos - 1].level;
                    std::cout << "\n  " << Color::GREEN << "Collected level " << level
                              << " treasure! (worth " << (level * 4) << "-" << (level * 4 + 3)
                              << " points)" << Color::RESET << "\n";
                }
                else if (chosenAction == DROP_TREASURE)
                {
                    std::cout << "\n  " << Color::YELLOW
                              << "Dropped your lowest treasure to swim faster!"
                              << Color::RESET << "\n";
                }

                state = state.doMove(chosenAction);
            }
        }
        else if (newPos == 0 && chosenMove == RETURN && !roundReset)
        {
            std::cout << "\n  " << Color::GREEN << Color::BOLD
                      << "Made it back to the submarine safely!" << Color::RESET << "\n";
            int pts = 0;
            for (auto &t : state.getPlayers()[currentP].getTreasures())
            {
                pts += static_cast<int>(t.size());
            }
            if (pts > 0)
            {
                std::cout << "  Your treasure will be scored at round end!\n";
            }
        }

        pressEnterToContinue();
    }

    printGameOver(state, numPlayers);
}

int main()
{
    clearScreen();

    std::cout << "\n  " << Color::BOLD << "Welcome to Deep Sea Adventure!" << Color::RESET << "\n";
    int numPlayers = 0;
    while (numPlayers < 2 || numPlayers > 6)
    {
        std::cout << "  How many players? (2-6): ";
        std::cin >> numPlayers;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            numPlayers = 0;
        }
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    playerTypes.resize(numPlayers, 0);

    std::cout << "\n  Configure each player:\n";
    std::cout << "    M = AI (Full MCTS - strong, slow)\n";
    std::cout << "    R = AI (Parallel MCTS - strong, fast)\n";
    std::cout << "    P = AI (Pure MCTS - simple, dumb)\n";
    std::cout << "    B = AI (Heuristic Bot - fast, predictable)\n";
    std::cout << "    H = Human (Complex, probably also dumb)\n\n";

    heuristicBots.resize(numPlayers, nullptr);

    for (int i = 0; i < numPlayers; i++)
    {
        char typeChar = ' ';
        while (typeChar != 'H' && typeChar != 'h' &&
               typeChar != 'M' && typeChar != 'm' &&
               typeChar != 'P' && typeChar != 'p' &&
               typeChar != 'R' && typeChar != 'r' &&
               typeChar != 'B' && typeChar != 'b')
        {
            std::cout << "  " << PLAYER_COLORS[i] << "Player " << (i + 1) << Color::RESET << " [H/M/R/P/B]: ";
            std::cin >> typeChar;

            if (std::cin.fail())
            {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                typeChar = ' ';
            }
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (typeChar == 'H' || typeChar == 'h')
        {
            playerTypes[i] = 0;
            std::cout << "    -> Human\n";
        }
        else if (typeChar == 'M' || typeChar == 'm')
        {
            playerTypes[i] = 1;
            std::cout << "    -> AI (Full MCTS)\n";
        }
        else if (typeChar == 'R' || typeChar == 'r')
        {
            playerTypes[i] = 3;
            std::cout << "    -> AI (Parallel MCTS)\n";
        }
        else if (typeChar == 'P' || typeChar == 'p')
        {
            playerTypes[i] = 2;
            std::cout << "    -> AI (Pure MCTS)\n";
        }
        else if (typeChar == 'B' || typeChar == 'b')
        {
            playerTypes[i] = 4;
            heuristicBots[i] = new HeuristicBot(numPlayers);
            std::cout << "    -> AI (Heuristic Bot)\n";
        }
    }

    std::cout << "\n  Starting game with " << numPlayers << " players...\n";
    pressEnterToContinue();

    runGame(numPlayers);

    // Cleanup heuristic bots
    for (int i = 0; i < numPlayers; i++)
    {
        if (heuristicBots[i] != nullptr)
        {
            delete heuristicBots[i];
            heuristicBots[i] = nullptr;
        }
    }

    return 0;
}
