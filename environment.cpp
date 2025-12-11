#include <stdexcept>
#include <algorithm>
#include "environment.hpp"

thread_local std::random_device RNG::rd;
thread_local std::mt19937 RNG::gen(rd());
std::uniform_int_distribution<int> RNG::dist1(1, 3);
std::uniform_int_distribution<int> RNG::dist2(0, 3);

void Board::updateBoard()
{
    std::vector<Tile> newTiles;
    for (auto tile: tiles)
    {
        if (!tile.flipped)
            newTiles.push_back(tile);
    }

    this->tiles = newTiles;
}

void Board::flipTile(int index)
{
    if (index >= tiles.size())
        throw std::runtime_error("Out of bounds access in tile list");
    tiles[index].flipped = true;
}

bool Board::isTileFlipped(int index)
{
    if (index >= tiles.size())
        throw std::runtime_error("Out of bounds access in tile list");
    return tiles[index].flipped;
}

bool Board::isTileOccupied(int index)
{
    if (index >= tiles.size())
        throw std::runtime_error("Out of bounds access in tile list");
    return tiles[index].occupied;
}

void State::calculatePlayerScores()
{
    for (auto& player : this->players)
    {
        if (!player.getIsDead())
        {
            for (auto& treasure : player.getTreasures())
            {
                player.addPoints(treasure);
            }
        }
    }
}

void State::redistributeTreasure()
{
    std::vector<int> allDroppedLoot;

    for (auto& player : players)
    {
        if (player.getPosition() != 0)
        {
            std::vector<int>& pTreasures = player.getTreasures();
            
            allDroppedLoot.insert(allDroppedLoot.end(), pTreasures.begin(), pTreasures.end());
            
            pTreasures.clear();
        }
    }

    if (allDroppedLoot.empty()) return;

    Tile currentTile;
    currentTile.level = 0;       
    currentTile.flipped = false; 

    int count = 0;
    for (int val : allDroppedLoot)
    {
        currentTile.treasure.push_back(val);
        count++;

        if (count % 3 == 0)
        {
            board.getTiles().push_back(currentTile);
            currentTile.treasure.clear(); 
        }
    }

    if (!currentTile.treasure.empty())
    {
        board.getTiles().push_back(currentTile);
    }
}

bool State::isLastRound()
{
    return currentRound > 2;
}

bool State::isTerminal()
{
    if (oxygen == 0) 
    {
        return true;
    }

    for (auto& p : players) {
        if (!p.getIsDead()) {
             if (p.getPosition() > 0 || !p.getIsReturning()) {
                 return false; 
             }
        }
    }

    return true;
}

Player& State::getCurrentPlayer()
{
    return players[currentPlayer];
}

int State::throwDice()
{
    return RNG::dist1(RNG::gen) + RNG::dist1(RNG::gen);
}

int Player::getTreasure(Tile& tile)
{
    int baseValue;

    tile.flip();

    switch (tile.level)
    {
        case 0:
            baseValue = 0;
            break;
        case 1:
            baseValue = 4;
            break;
        case 2:
            baseValue = 8;
            break;
        case 3:
            baseValue = 12;
            break;
        default:
            throw std::runtime_error("Invalid tile type");
    }

    for (auto treasure : tile.treasure)
    {
        baseValue += treasure;
    }

    int value = baseValue + RNG::dist2(RNG::gen);

    this->treasures.push_back(value);
    return value;
}

void Board::changeOccupationStatus(int index)
{
    if (index >= tiles.size())
        throw std::runtime_error("Out of bounds access in tile list");
    tiles[index].occupied = !tiles[index].occupied;
}

void Player::move(int distance, Board& board)
{
    distance = std::max<int>(0, distance - treasures.size());

    if (this->position > 0)
        board.changeOccupationStatus(this->position);

    while (distance > 0)
    {
        if (!isReturning)
            this->position += 1;
        else
            this->position -= 1;

        if (!board.isTileOccupied(this->position))
            distance--;
    }

    if (this->position > board.getTiles().size()) // went beyond the edge of the map, which you can't do (I believe)
    {
        while(board.isTileOccupied(this->position))
            this->position--; // go to the last unoccupied position
    }

    if (this->position > 0)
        board.changeOccupationStatus(this->position);
}

std::vector<MoveType> State::getPossibleMoves(bool movedThisTurn)
{
    std::vector<MoveType> result;

    if (isTerminal())
    {
        result.push_back({END});

        return result;
    }

    Player currentPlayer = getCurrentPlayer();

    if (!movedThisTurn) // move this turn
    {
        if (currentPlayer.getPosition() == 0 && currentPlayer.getIsReturning())
            return result; // went back to the submarine, no options left, stay there

        if (currentPlayer.getIsReturning())
        {
            result.push_back(RETURN);
        }
        else if (currentPlayer.getPosition() < board.getTiles().size()) // not at submarine and not returning
        {
            result.push_back(CONTINUE);
            result.push_back(RETURN);
        }
        else
        {
            result.push_back(RETURN);
        }
        
        return result;
    }

    // if we reached this point, then the choice is simply between collecting a treasure or not
    if (currentPlayer.getPosition() < board.getTiles().size() && 
        !board.isTileFlipped(currentPlayer.getPosition())) // can collect treasure
    {
        result.push_back(COLLECT_TREASURE);
    }
   
    if (currentPlayer.getTreasures().size() > 0 && board.isTileFlipped(currentPlayer.getPosition()))
        result.push_back(DROP_TREASURE);

    result.push_back(LEAVE_TREASURE);

    return result;
}

void Player::reset()
{
    this->position = 0;
    this->isDead = false;
    this->isReturning = false;
    this->treasures.clear(); 
}

void State::reset()
{
        this->board.updateBoard();
        redistributeTreasure();

        for (auto& player : players)
        {
            player.reset();
        }
        
        this->oxygen = 25;
        this->currentRound++;
}

State State::doMove(MoveType move)
{
    State newState(*this);
    Player& currentPlayer = newState.getCurrentPlayer();

    if (move == CONTINUE || move == RETURN)
    {
        newState.oxygen -= currentPlayer.getTreasures().size();
        if (newState.oxygen < 0) newState.oxygen = 0;
    }

    switch (move)
    {
        case CONTINUE:
        {
            int diceResult = newState.throwDice();
            currentPlayer.move(diceResult, newState.board);
            break;
        }
        case RETURN:
        {
            int diceResult = newState.throwDice();
            currentPlayer.returnToSubmarine();
            currentPlayer.move(diceResult, newState.board);
            break;
        }
        case COLLECT_TREASURE:
        {
            if (currentPlayer.getPosition() < newState.board.getTiles().size()) 
            {
                newState.board.flipTile(currentPlayer.getPosition());
                currentPlayer.getTreasure(newState.board.getTiles()[currentPlayer.getPosition()]);
            }
            break;
        }
        case LEAVE_TREASURE:
        {
            // do nothing
            break;
        }
        case DROP_TREASURE:
        {
            int minIndex = 0;
            int minValue = 30;

            std::vector<int>& treasures = currentPlayer.getTreasures();
            for (int i = 0; i < treasures.size(); i++)
            {
                if (treasures[i] < minValue)
                {
                    minValue = treasures[i];
                    minIndex = i;
                }
            }

            treasures.erase(treasures.begin() + minIndex);
            break;
        }
        case END:
            break;
        default:
            throw std::runtime_error("Invalid action type");
    }

    if (newState.isTerminal())
    {
        for (auto& player : newState.players)
        {
            if (player.getPosition() != 0)
                player.setIsDead();
        }
      
        newState.calculatePlayerScores();

        if (!newState.isLastRound())
            newState.reset();
        
        return newState;
    }

    if (currentPlayer.getPosition() == 0 || move == COLLECT_TREASURE || move == LEAVE_TREASURE || move == DROP_TREASURE)
        newState.currentPlayer = (this->currentPlayer + 1) % (players.size());

    return newState;
}
