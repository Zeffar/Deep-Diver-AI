#include <vector>
#include <random>

enum MoveType
{
    CONTINUE,
    RETURN,
    COLLECT_TREASURE,
    LEAVE_TREASURE, // don't collect the treasure
    DROP_TREASURE, // drop treasure you already have
    END,
};

class Tile
{
public:
    int level;
    bool flipped = false;
    bool occupied = false;
    std::vector<int> treasure;
    
    void flip()
    {
        this->flipped = true;
    }

    bool isFlipped()
    {
        return flipped;
    }

    bool isOccupied()
    {
        return occupied;
    }
};

class Board
{
private:
    std::vector<Tile> tiles; 
public:
    Board()
    {
        tiles = {{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
                 {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1},
                 {2}, {2}, {2}, {2}, {2}, {2}, {2}, {2},
                 {3}, {3}, {3}, {3}, {3}, {3}, {3}, {3}};
    }

    std::vector<Tile>& getTiles()
    {
        return this->tiles;
    }

    void updateBoard();
    void flipTile(int index);
    bool isTileFlipped(int index);
    bool isTileOccupied(int index);
    void changeOccupationStatus(int index);
};

class Player
{
private:
    std::vector<int> treasures;
    int points = 0;
    int position = 0; // 0 means the submarine for all intents and purposes
    bool isDead = false;
    bool isReturning = false;
public:
    int getPosition()
    {
        return this->position;
    }

    bool getIsDead()
    {
        return this->isDead;
    }

    void setIsDead()
    {
        this->isDead = true;
    }

    bool getIsReturning()
    {
        return this->isReturning;
    }

    std::vector<int>& getTreasures()
    {
        return this->treasures;
    }
    
    int getPoints()
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
    void move(int distance, Board& board);
    int getTreasure(Tile& tile);
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

    int throwDice();
    
public:
    State(int nPlayers) 
    {
        players.resize(nPlayers);
    }

    State(const State& other)
    : currentPlayer(other.currentPlayer),
      oxygen(other.oxygen),
      players(other.players),   
      board(other.board)        
{
}

    int getOxygen()
    {
        return this->oxygen;
    }

    std::vector<Player>& getPlayers()
    {
        return this->players;
    }
   
    void reset();
    void redistributeTreasure();
    bool isLastRound();
    void calculatePlayerScores();
    bool isTerminal();
    Player& getCurrentPlayer(); 
    std::vector<MoveType> getPossibleMoves(bool movedThisTurn);
    State doMove(MoveType move);
};
