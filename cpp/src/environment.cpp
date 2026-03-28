#include <stdexcept>
#include <algorithm>
#include "environment.hpp"

thread_local std::random_device RNG::rd;
thread_local std::mt19937 RNG::gen(rd());
std::uniform_int_distribution<int> RNG::dist1(1, 3);
std::vector<int> Tile::tileValues0 = {0, 0, 1, 1, 2, 2, 3, 3};
std::vector<int> Tile::tileValues1 = {4, 4, 5, 5, 6, 6, 7, 7};
std::vector<int> Tile::tileValues2 = {8, 8, 9, 9, 10, 10, 11, 11};
std::vector<int> Tile::tileValues3 = {12, 12, 13, 13, 14, 14, 15, 15};
bool Tile::useDeterministicValues = false;

template <typename T>
T pickAndRemoveRandomElement(std::vector<T> &vec)
{
    if (vec.empty())
    {
        throw std::runtime_error("Cannot pick from an empty vector");
    }

    std::uniform_int_distribution<size_t> dist(0, vec.size() - 1); // pick a random element
    size_t randomIndex = dist(RNG::gen);

    T pickedValue = vec[randomIndex]; // save the value

    vec[randomIndex] = vec.back(); // move a value from the back in that slot

    vec.pop_back(); // remove the value from the back

    return pickedValue; // return picked value
}

void Board::updateBoard()
{
    std::vector<Tile> newTiles;
    for (auto tile : tiles)
    {
        if (!tile.flipped)
        {
            if (tile.isOccupied())
                tile.changeOccupationStatus();
            newTiles.push_back(tile);
        }
    }

    this->tiles = newTiles;
}

// flip tile when collecting treasure
void Board::flipTile(int index)
{
    if (index > tiles.size())
        throw std::runtime_error("Out of bounds access in tile list");

    if (index == 0)
        return;

    tiles[index - 1].flipped = true;
}

bool Board::isTileFlipped(int index) const
{
    if (index <= 0)
        return false;

    if (index > static_cast<int>(tiles.size()))
        return true; // Treat out of bounds as already flipped (no treasure)

    return tiles[index - 1].flipped;
}

bool Board::isTileOccupied(int index)
{
    if (index <= 0)
        return false;

    if (index > static_cast<int>(tiles.size()))
        return false; // Treat out of bounds as unoccupied instead of crashing

    return tiles[index - 1].occupied;
}

void Tile::resetValuePools()
{
    tileValues0 = {0, 0, 1, 1, 2, 2, 3, 3};
    tileValues1 = {4, 4, 5, 5, 6, 6, 7, 7};
    tileValues2 = {8, 8, 9, 9, 10, 10, 11, 11};
    tileValues3 = {12, 12, 13, 13, 14, 14, 15, 15};
}

Tile::ValuePoolSnapshot Tile::saveValuePools()
{
    return {tileValues0, tileValues1, tileValues2, tileValues3};
}

void Tile::restoreValuePools(const ValuePoolSnapshot &snapshot)
{
    tileValues0 = snapshot.v0;
    tileValues1 = snapshot.v1;
    tileValues2 = snapshot.v2;
    tileValues3 = snapshot.v3;
}

/**
 *  Calculate player scores at end of turn
 */
void State::calculatePlayerScores()
{
    for (auto &player : this->players)
    {
        if (!player.getIsDead())
        {
            for (auto &treasure : player.getTreasures())
            {
                player.addPoints(Tile::calculateTreasureValue(treasure));
            }
        }
    }
}

/**
 * Collects all treasure from players who drowned and
 * places them in stacks of up to 3 at the bottom of the ocean.
 */
void State::redistributeTreasure()
{
    TreasureStack allDroppedLoot;

    for (auto &player : players) // collect treasure from dead players
    {
        if (player.getPosition() != 0)
        {
            auto &pTreasures = player.getTreasures();

            for (auto &treasure : pTreasures)
                allDroppedLoot.insert(allDroppedLoot.end(), treasure.begin(), treasure.end());

            pTreasures.clear();
        }
    }

    auto &tiles = this->board.getTiles();

    while (!allDroppedLoot.empty()) // redistribute the treasure
    {
        Tile newTile;
        newTile.level = 4; // Mark as fallen treasure
        while (newTile.droppedTreasureCount < 3 && !allDroppedLoot.empty())
        {
            newTile.treasure.push_back(allDroppedLoot.back());
            allDroppedLoot.pop_back();
            newTile.droppedTreasureCount++;
        }

        tiles.push_back(newTile);
    }
}

bool State::isLastRound() const // meaning third round
{
    return currentRound >= 2;
}

bool State::isTerminal() const // oxygen is 0 or all players reached the submarine
{
    if (oxygen == 0)
    {
        return true;
    }

    for (const auto &p : players)
    { // check if the players are in the submarine
        if (!p.getIsDead())
        {
            if (p.getPosition() > 0 || !p.getIsReturning())
            {
                return false;
            }
        }
    }

    return true;
}

Player &State::getCurrentPlayer()
{
    return players[currentPlayer];
}

int State::throwDice()
{
    return RNG::dist1(RNG::gen) + RNG::dist1(RNG::gen);
}

int Tile::calculateTreasureValue(TreasureStack stack)
{
    int sum = 0;
    for (auto treasure : stack)
    {
        if (useDeterministicValues)
        {
            // Use average value for each level during MCTS to avoid pool exhaustion
            // Level 0: avg of 0-3 = 1.5, Level 1: avg of 4-7 = 5.5, etc.
            switch (treasure)
            {
            case 0:
                sum += 2; // midpoint of 0-3
                break;
            case 1:
                sum += 6; // midpoint of 4-7
                break;
            case 2:
                sum += 10; // midpoint of 8-11
                break;
            case 3:
                sum += 14; // midpoint of 12-15
                break;
            default:
                throw std::runtime_error("Invalid tile type");
            }
        }
        else
        {
            switch (treasure)
            {
            case 0:
                sum += pickAndRemoveRandomElement(tileValues0);
                break;
            case 1:
                sum += pickAndRemoveRandomElement(tileValues1);
                break;
            case 2:
                sum += pickAndRemoveRandomElement(tileValues2);
                break;
            case 3:
                sum += pickAndRemoveRandomElement(tileValues3);
                break;
            default:
                throw std::runtime_error("Invalid tile type");
            }
        }
    }

    return sum;
}

void Player::getTreasure(Tile &tile) // collects treasure from a tile and calculates its value
{
    tile.flip();

    TreasureStack new_treasures;

    for (auto treasure : tile.treasure)
    {
        new_treasures.push_back(treasure);
    }

    // Only add tile level if it's a regular tile (0-3), not a fallen treasure tile (4)
    if (tile.level < 4)
    {
        new_treasures.push_back(tile.level);
    }

    this->inventory.push_back(new_treasures);
}

void Tile::changeOccupationStatus()
{
    this->occupied = !this->occupied;
}

void Player::move(int distance, Board &board)
{
    distance = std::max<int>(0, distance - inventory.size());
    auto &tiles = board.getTiles();

    if (this->position > 0 && this->position <= static_cast<int>(tiles.size()))
        tiles[this->position - 1].changeOccupationStatus();

    while (distance > 0)
    {
        if (isReturning)
        {
            this->position -= 1;

            if (this->position <= 0)
            {
                this->position = 0;
                distance--; // Moving to submarine consumes distance
                break;      // Reached submarine
            }

            // Only consume distance if the tile we landed on is not occupied
            if (!board.isTileOccupied(this->position))
                distance--;
        }
        else
        {
            this->position += 1;
            if (this->position >= static_cast<int>(tiles.size()))
            {
                this->position = static_cast<int>(tiles.size());
                isReturning = true;
            }

            // Only consume distance if the tile we landed on is not occupied
            if (!board.isTileOccupied(this->position))
                distance--;
        }
    }

    if (this->position > 0 && this->position <= static_cast<int>(tiles.size()))
        tiles[this->position - 1].changeOccupationStatus();
}

std::vector<MoveType> State::getPossibleMoves(bool movedThisTurn) const
{
    std::vector<MoveType> result;

    if (isTerminal())
    {
        result.push_back({END});

        return result;
    }

    Player currentPlayerCopy = players[currentPlayer];

    if (!movedThisTurn) // move this turn
    {
        if (currentPlayerCopy.getPosition() == 0 && currentPlayerCopy.getIsReturning())
        {
            result.push_back(LEAVE_TREASURE);
            return result; // went back to the submarine, no options left, stay there
        }
        if (currentPlayerCopy.getIsReturning()) // if you decide to return you can move in only one direction
        {
            result.push_back(RETURN);
        }
        else
        {
            result.push_back(CONTINUE);

            if (currentPlayerCopy.getTreasures().size() > 0 ||
                currentPlayerCopy.getPosition() >= (int)board.getTiles().size()) // can return with treasure OR at end tile
                result.push_back(RETURN);                                        // decide to return
        }

        return result;
    }

    // if we reached this point, then the choice is simply between collecting a treasure or not
    if (currentPlayerCopy.getPosition() <= board.getTiles().size() &&
        currentPlayerCopy.getPosition() != 0 &&
        !board.isTileFlipped(currentPlayerCopy.getPosition())) // can collect treasure
    {
        result.push_back(COLLECT_TREASURE);
    }

    if (currentPlayerCopy.getTreasures().size() > 0 &&
        currentPlayerCopy.getPosition() != 0 &&
        board.isTileFlipped(currentPlayerCopy.getPosition()))
    {
        result.push_back(DROP_TREASURE);
    }

    result.push_back(LEAVE_TREASURE);

    return result;
}

void Player::reset()
{
    this->position = 0;
    this->isDead = false;
    this->isReturning = false;
    this->inventory.clear();
}

void State::reset()
{
    this->board.updateBoard();

    // Note: calculatePlayerScores() is called in doMove() before reset()
    // so we don't need to call it again here

    redistributeTreasure();

    // Reset value pools for the new round
    Tile::resetValuePools();

    for (auto &player : players)
    {
        player.reset();
    }

    this->oxygen = 25;
    this->currentRound++;
    this->currentPlayer = this->lastPlayer; // last one to arrive at submarine plays first
}

State State::doMove(MoveType move) const // apply move to current state
{
    State newState(*this);
    Player &currentPlayerRef = newState.getCurrentPlayer();

    if (move == CONTINUE || move == RETURN) // each time a player moves, reduce oxygen if they carry a treasure
    {
        newState.oxygen -= currentPlayerRef.getTreasures().size();
        if (newState.oxygen < 0)
            newState.oxygen = 0;
    }

    switch (move)
    {
    case CONTINUE:
    {
        int diceResult = newState.throwDice();
        currentPlayerRef.move(diceResult, newState.board);
        break;
    }
    case RETURN:
    {
        int diceResult = newState.throwDice();
        currentPlayerRef.returnToSubmarine(); // mark the player as returning to submarine
        currentPlayerRef.move(diceResult, newState.board);

        if (currentPlayerRef.getPosition() == 0) // set last player to arrive at submarine
            newState.lastPlayer = newState.currentPlayer;
        break;
    }
    case COLLECT_TREASURE:
    {
        int collectPos = currentPlayerRef.getPosition();
        if (collectPos > 0 && collectPos <= static_cast<int>(newState.board.getTiles().size()))
        {
            newState.board.flipTile(collectPos);
            currentPlayerRef.getTreasure(newState.board.getTiles()[collectPos - 1]);
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
        int minValue = 5; // value means level, since you can't see the values of the treasures
                          // levels are from 0 to 3 (0 having lowest value)

        std::vector<std::vector<int>> &treasures = currentPlayerRef.getTreasures();
        for (size_t i = 0; i < treasures.size(); i++) // drop treasure with lowest level(calculates sum of all levels if stack)
        {
            int sum = 0;
            for (size_t j = 0; j < treasures[i].size(); j++)
            {
                sum += treasures[i][j];
            }

            if (sum < minValue)
            {
                minValue = sum;
                minIndex = static_cast<int>(i);
            }
        }

        int dropPos = currentPlayerRef.getPosition();
        if (dropPos > 0 && dropPos <= static_cast<int>(newState.board.getTiles().size()))
        {
            for (auto &treasure : treasures[minIndex])
                newState.board.getTiles()[dropPos - 1].treasure.push_back(treasure);

            // Mark tile as having treasure again (unflip it)
            newState.board.getTiles()[dropPos - 1].flipped = false;
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
        for (auto &player : newState.players)
        {
            if (player.getPosition() != 0) // kill all players that did not reach the submarine
                player.setIsDead();
        }

        newState.calculatePlayerScores(); // add the captured treasures to the player score

        if (!newState.isLastRound()) // if it's not third round, reset state
            newState.reset();

        return newState;
    }

    if (currentPlayerRef.getPosition() == 0 || move == COLLECT_TREASURE || move == LEAVE_TREASURE || move == DROP_TREASURE)
        newState.currentPlayer = (this->currentPlayer + 1) % (players.size()); // it's the next player's turn

    return newState;
}
