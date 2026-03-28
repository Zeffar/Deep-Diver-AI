#ifndef ENVIRONMENT_HPP
#define ENVIRONMENT_HPP

#include <vector>
#include <random>

using TreasureStack = std::vector<int>;
using Inventory = std::vector<TreasureStack>;

enum MoveType
{
    CONTINUE,
    RETURN,
    COLLECT_TREASURE,
    LEAVE_TREASURE, // don't collect the treasure
    DROP_TREASURE,  // drop treasure you already have
    END,
};

class Tile
{
public:
    int level;
    bool flipped = false;
    bool occupied = false;
    int droppedTreasureCount = 0;
    TreasureStack treasure;

    void flip()
    {
        this->flipped = true;
    }

    bool isFlipped() const
    {
        return flipped;
    }

    bool isOccupied() const
    {
        return occupied;
    }

    void changeOccupationStatus();

    static std::vector<int> tileValues0; // possible values for tiles with level 0
    static std::vector<int> tileValues1; // possible values for tiles with level 1
    static std::vector<int> tileValues2; // possible values for tiles with level 2
    static std::vector<int> tileValues3; // possible values for tiles with level 3
    static void resetValuePools();

    // For MCTS: save and restore value pools to avoid exhaustion during simulations
    struct ValuePoolSnapshot
    {
        std::vector<int> v0, v1, v2, v3;
    };
    static ValuePoolSnapshot saveValuePools();
    static void restoreValuePools(const ValuePoolSnapshot &snapshot);

    // Flag to use deterministic values during MCTS (avoids pool exhaustion)
    static bool useDeterministicValues;

    static int calculateTreasureValue(TreasureStack stack); // convert tile level to an actual value
};

class Board
{
private:
    std::vector<Tile> tiles;

public:
    Board()
    {
        tiles = {
            {0, false, false, 0, {}}, {0, false, false, 0, {}}, {0, false, false, 0, {}}, {0, false, false, 0, {}}, {0, false, false, 0, {}}, {0, false, false, 0, {}}, {0, false, false, 0, {}}, {0, false, false, 0, {}}, {1, false, false, 0, {}}, {1, false, false, 0, {}}, {1, false, false, 0, {}}, {1, false, false, 0, {}}, {1, false, false, 0, {}}, {1, false, false, 0, {}}, {1, false, false, 0, {}}, {1, false, false, 0, {}}, {2, false, false, 0, {}}, {2, false, false, 0, {}}, {2, false, false, 0, {}}, {2, false, false, 0, {}}, {2, false, false, 0, {}}, {2, false, false, 0, {}}, {2, false, false, 0, {}}, {2, false, false, 0, {}}, {3, false, false, 0, {}}, {3, false, false, 0, {}}, {3, false, false, 0, {}}, {3, false, false, 0, {}}, {3, false, false, 0, {}}, {3, false, false, 0, {}}, {3, false, false, 0, {}}, {3, false, false, 0, {}}};
    }

    std::vector<Tile> &getTiles()
    {
        return this->tiles;
    }

    const std::vector<Tile> &getTiles() const
    {
        return this->tiles;
    }

    void updateBoard();
    void flipTile(int index);
    bool isTileFlipped(int index) const;
    bool isTileOccupied(int index);
};

class Player
{
private:
    Inventory inventory;
    int points = 0;
    int position = 0; // 0 means the submarine for all intents and purposes
    bool isDead = false;
    bool isReturning = false;

public:
    int getPosition() const
    {
        return this->position;
    }

    bool getIsDead() const
    {
        return this->isDead;
    }

    void setIsDead()
    {
        this->isDead = true;
    }

    bool getIsReturning() const
    {
        return this->isReturning;
    }

    Inventory &getTreasures()
    {
        return this->inventory;
    }

    int getPoints() const
    {
        return this->points;
    }

    void addPoints(int points)
    {
        this->points += points;
    }

    void returnToSubmarine()
    {
        this->isReturning = true;
    }

    void reset();
    void move(int distance, Board &board);
    void getTreasure(Tile &tile);
    int calculateValue(TreasureStack stack);
};

class RNG
{
private:
    thread_local static std::random_device rd;

public:
    thread_local static std::mt19937 gen;
    static std::uniform_int_distribution<int> dist1;
    static std::uniform_int_distribution<int> dist2;

    RNG() = delete;
};

class State
{
private:
    int currentPlayer = 0;
    int currentRound = 0;
    int oxygen = 25;
    std::vector<Player> players;
    Board board;
    int lastPlayer = 0; // last player to arrive at submarine
    int throwDice();

public:
    State(int nPlayers)
    {
        players.resize(nPlayers);
    }

    State(const State &other)
        : currentPlayer(other.currentPlayer),
          currentRound(other.currentRound),
          oxygen(other.oxygen),
          players(other.players),
          board(other.board),
          lastPlayer(other.lastPlayer)
    {
    }

    State &operator=(const State &other)
    {
        if (this != &other)
        {
            currentPlayer = other.currentPlayer;
            currentRound = other.currentRound;
            oxygen = other.oxygen;
            players = other.players;
            board = other.board;
            lastPlayer = other.lastPlayer;
        }
        return *this;
    }

    int getOxygen() const
    {
        return this->oxygen;
    }

    Board &getBoard()
    {
        return this->board;
    }

    const Board &getBoard() const
    {
        return this->board;
    }

    int getCurrentPlayerIndex() const
    {
        return this->currentPlayer;
    }

    int getCurrentRound() const
    {
        return this->currentRound;
    }

    std::vector<Player> &getPlayers()
    {
        return this->players;
    }

    const std::vector<Player> &getPlayers() const
    {
        return this->players;
    }

    void reset();
    void redistributeTreasure();
    bool isLastRound() const;
    void calculatePlayerScores();
    bool isTerminal() const;
    Player &getCurrentPlayer();
    std::vector<MoveType> getPossibleMoves(bool movedThisTurn) const;
    State doMove(MoveType move) const;
};

#endif // ENVIRONMENT_HPP
