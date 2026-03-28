#include "../src/environment.cpp"
#include "../src/heuristic_bot.cpp"
#include "../src/mcts.cpp"
#include "../src/pure_mcts.cpp"
#include "../src/parallel_mcts.cpp"
#include <emscripten/bind.h>

using namespace emscripten;

// Helper to convert vectors to JS arrays if needed, but Embind handles
// std::vector automatically with register_vector.

EMSCRIPTEN_BINDINGS(deep_sea_adventure)
{
    // Enum binding
    enum_<MoveType>("MoveType")
        .value("CONTINUE", CONTINUE)
        .value("RETURN", RETURN)
        .value("COLLECT_TREASURE", COLLECT_TREASURE)
        .value("LEAVE_TREASURE", LEAVE_TREASURE)
        .value("DROP_TREASURE", DROP_TREASURE)
        .value("END", END);

    // Bind std::vector<int> for TreasureStack
    register_vector<int>("TreasureStack");
    // Bind std::vector<TreasureStack> for Inventory
    register_vector<TreasureStack>("Inventory");
    // Bind std::vector<Tile>
    register_vector<Tile>("TileVector");
    // Bind std::vector<Player>
    register_vector<Player>("PlayerVector");
    // Bind std::vector<MoveType>
    register_vector<MoveType>("MoveTypeVector");

    class_<Tile>("Tile")
        .constructor<>()
        .property("level", &Tile::level)
        .property("flipped", &Tile::flipped)
        .property("occupied", &Tile::occupied)
        .property("droppedTreasureCount", &Tile::droppedTreasureCount)
        .property("treasure", &Tile::treasure)
        .function("isFlipped", &Tile::isFlipped)
        .function("isOccupied", &Tile::isOccupied)
        .class_function("calculateTreasureValue", &Tile::calculateTreasureValue);

    class_<Board>("Board")
        .constructor<>()
        .function("getTiles",
                  optional_override([](Board &self) -> std::vector<Tile>
                                    { return self.getTiles(); }))
        .function("flipTile", &Board::flipTile)
        .function("isTileFlipped", &Board::isTileFlipped)
        .function("isTileOccupied", &Board::isTileOccupied);

    class_<Player>("Player")
        .constructor<>()
        .property("position", &Player::getPosition)
        .property("isDead", &Player::getIsDead)
        .property("isReturning", &Player::getIsReturning)
        .function("getPoints", &Player::getPoints)
        .function("getTreasures",
                  optional_override([](Player &self) -> Inventory
                                    { return self.getTreasures(); }))
        .function("reset", &Player::reset);

    class_<State>("State")
        .constructor<int>()
        .function("getOxygen", &State::getOxygen)
        .function("getPlayers",
                  optional_override([](State &self) -> std::vector<Player>
                                    { return self.getPlayers(); }))
        .function("getBoard", optional_override([](State &self) -> Board
                                                { return self.getBoard(); }))
        .function("reset", &State::reset)
        .function("isLastRound", &State::isLastRound)
        .function("isTerminal", &State::isTerminal)
        .function("getCurrentPlayer",
                  optional_override([](State &self) -> Player
                                    { return self.getCurrentPlayer(); }))
        .function("getPossibleMoves", &State::getPossibleMoves)
        .function("getCurrentPlayerIndex", &State::getCurrentPlayerIndex)
        .function("getCurrentRound", &State::getCurrentRound)
        .function("doMove", &State::doMove);

    class_<HeuristicBot>("HeuristicBot")
        .constructor<int>()
        .function("findBestMove", &HeuristicBot::findBestMove);

    class_<MCTS>("MCTS")
        .constructor<int, int, double>()
        .function("findBestMove", &MCTS::findBestMove);

    class_<PureMCTS>("PureMCTS")
        .constructor<int, int>()
        .function("findBestMove", &PureMCTS::findBestMove);

    class_<ParallelMCTS>("ParallelMCTS")
        .constructor<int, int, double, int>()
        .function("findBestMove", &ParallelMCTS::findBestMove);
}
