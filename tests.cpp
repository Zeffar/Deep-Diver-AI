#include <gtest/gtest.h>
#include <algorithm>
#include "environment.hpp"

class DeepSeaAdventureTest : public ::testing::Test 
{
    protected:
        void SetUp() override // runs before every TEST_F
        {
            srand(42);
        }

        bool hasMove(const std::vector<MoveType>& moves, MoveType target) 
        {
            return std::find(moves.begin(), moves.end(), target) != moves.end();
        }
};

TEST_F(DeepSeaAdventureTest, StateImmutabilityCriticalForMCTS) {
    State rootState(3);
    int originalPos = rootState.getCurrentPlayer().getPosition();
    
    auto moves = rootState.getPossibleMoves(false);
    ASSERT_FALSE(moves.empty());
    
    State childState = rootState.doMove(moves[0]);
    
    EXPECT_EQ(rootState.getCurrentPlayer().getPosition(), originalPos);
    
    EXPECT_NE(&rootState.getCurrentPlayer(), &childState.getCurrentPlayer());
}

TEST_F(DeepSeaAdventureTest, BoardShrinksAndHandlesBounds) {
    Board b;
    b.flipTile(31);
    b.flipTile(30);
    
    size_t initialSize = b.getTiles().size();
    b.updateBoard();
    
    EXPECT_LT(b.getTiles().size(), initialSize);
    EXPECT_EQ(b.getTiles().size(), 30);
    
    EXPECT_ANY_THROW(b.isTileFlipped(31));
}

TEST_F(DeepSeaAdventureTest, GreedyGameSimulation) {
    State s(3); // 3 players
    int safetyInterlock = 0;

    while (!s.isTerminal() && safetyInterlock < 1000) {
        safetyInterlock++;
        
        auto moveChoices = s.getPossibleMoves(false);
        if (moveChoices.empty()) break;

        MoveType m = hasMove(moveChoices, CONTINUE) ? CONTINUE : moveChoices[0];
        s = s.doMove(m);

        if (s.isTerminal()) break;

        auto actionChoices = s.getPossibleMoves(true);
        MoveType a = hasMove(actionChoices, COLLECT_TREASURE) ? COLLECT_TREASURE : LEAVE_TREASURE;
        s = s.doMove(a);
    }

    EXPECT_EQ(s.getOxygen(), 0) << "Oxygen should be depleted in a greedy game.";
    for (auto& p : s.getPlayers()) {
        EXPECT_TRUE(p.getIsDead()) << "Greedy players who never return should die.";
        EXPECT_EQ(p.getPoints(), 0) << "Dead players should score 0 points.";
    }
}

TEST_F(DeepSeaAdventureTest, NoCollectionAtSubmarine) {
    State s(2); 
    ASSERT_EQ(s.getCurrentPlayer().getPosition(), 0);

    auto actions = s.getPossibleMoves(true);

    for (auto a : actions) {
        EXPECT_NE(a, COLLECT_TREASURE) << "Should not be able to collect treasure at position 0.";
    }
}

TEST_F(DeepSeaAdventureTest, MovementSkipsOccupiedTiles) {
    State s(2);
    
    s = s.doMove(CONTINUE); // Roll
    
    int p2_start = s.getCurrentPlayer().getPosition();
    s = s.doMove(CONTINUE); 
    
    int p2_end = s.getCurrentPlayer().getPosition();
    EXPECT_NE(p2_end, 1) << "Player 2 should have skipped the tile occupied by Player 1.";
}

TEST_F(DeepSeaAdventureTest, ManualTerminalStateReset) {
    State s(2);
    
    auto& players = s.getPlayers();
    
    TreasureStack loot1 = {0, 1}; // Level 0 and Level 1
    players[0].getTreasures().push_back(loot1);
    players[0].returnToSubmarine(); 

    TreasureStack loot2 = {2, 3}; 
    players[1].getTreasures().push_back(loot2);
    players[1].move(20, const_cast<Board&>(s.getBoard())); // Move them out deep

    s.reset();

    EXPECT_EQ(s.getOxygen(), 25);
    EXPECT_EQ(s.getPlayers()[0].getPosition(), 0);
    EXPECT_EQ(s.getPlayers()[1].getPosition(), 0);
    
    EXPECT_GT(s.getPlayers()[0].getPoints(), 0);
    
    EXPECT_EQ(s.getPlayers()[1].getTreasures().size(), 0);
    EXPECT_FALSE(s.getPlayers()[1].getIsDead());
}

TEST_F(DeepSeaAdventureTest, TreasureRedistributionStacksCorrectly) {
    State s(1); 
    auto& p = s.getPlayers()[0];
    
    p.getTreasures().push_back({1});
    p.getTreasures().push_back({1});
    p.getTreasures().push_back({1});
    p.getTreasures().push_back({1});
    
    p.move(10, const_cast<Board&>(s.getBoard()));

    s.redistributeTreasure();

    auto& tiles = const_cast<Board&>(s.getBoard()).getTiles();
    
    ASSERT_GE(tiles.size(), 2) << "Board should have at least 2 new tiles.";
    
    EXPECT_EQ(tiles.back().treasure.size(), 1) << "The last stack should have the remainder (1).";
    
    EXPECT_EQ(tiles[tiles.size() - 2].treasure.size(), 3) << "The penultimate stack should be full (3).";
    
    EXPECT_EQ(p.getTreasures().size(), 0);
}
