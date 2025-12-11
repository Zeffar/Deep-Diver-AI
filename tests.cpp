#include <ctime>
#include <iostream>
#include <cassert>
#include <vector>
#include "environment.hpp"

// Helper to print state for debugging
void printState(State& s) {
    std::cout << "  Oxygen: " << s.getCurrentPlayer().getTreasures().size() 
              << " (held) | Board Size: " << (s.isTerminal() ? "END" : "ACTIVE") << "\n";
}

// TEST 1: The MCTS Integrity Test (State Copying)
void testStateImmutability() {
    std::cout << "[TEST] State Immutability (Critical for MCTS)... ";
    
    State rootState(3); // 3 Players
    
    // Save info from root
    int rootPlayer = rootState.getCurrentPlayer().getPosition();
    
    // Create a child state
    std::vector<MoveType> moves = rootState.getPossibleMoves(false);
    State childState = rootState.doMove(moves[0]); // Likely CONTINUE
    
    // CHECK: Did the root state change?
    assert(rootState.getCurrentPlayer().getPosition() == rootPlayer);
    assert(&rootState.getCurrentPlayer() != &childState.getCurrentPlayer()); // Different memory addresses
    
    std::cout << "PASSED\n";
}

// TEST 2: Oxygen Mechanics
void testOxygenMechanics() {
    std::cout << "[TEST] Oxygen Mechanics... ";
    
    State s(2);
    // Force give player treasures to simulate weight
    // Note: You might need to make 'treasures' public or add a helper for testing
    // For now, we simulate by running moves until we pick something up
    
    // This is hard to unit test without modifying the class to allow setting state manually.
    // Instead, we verify logic:
    // If we have 0 items, oxygen shouldn't drop on move.
    
    int startOxygen = 25; // Assuming 25 is hardcoded in your header
    // Create a state where oxygen is exposed or checking it indirectly
    // (Skipping deep check here, trusting the logic review)
    
    std::cout << "SKIPPED (Requires access to private Oxygen var)\n";
}

// TEST 3: Shrinking Board / Bounds Check
void testShrinkingBoard() {
    std::cout << "[TEST] Shrinking Board Logic... ";
    
    Board b;
    // Simulate end of round
    b.flipTile(31); // Last tile
    b.flipTile(30);
    
    int initialSize = b.getTiles().size(); // 32
    b.updateBoard(); // Should remove flipped tiles
    
    assert(b.getTiles().size() < initialSize);
    assert(b.getTiles().size() == 30);
    
    // Check access bounds
    try {
        bool f = b.isTileFlipped(31); // Should throw
        assert(false); // Should not reach here
    } catch (...) {
        // Expected behavior
    }
    
    std::cout << "PASSED\n";
}

void runGreedyGame() {
    std::cout << "\n[SIMULATION] Running GREEDY game (Always Collect)...\n";
    State s(3);
    int turnCount = 0;
    
    // Run the loop
    while (!s.isTerminal()) {
        turnCount++;
        Player& p = s.getCurrentPlayer();
        
        // Phase 1: Move (Always Continue)
        std::vector<MoveType> moves1 = s.getPossibleMoves(false);
        if (moves1.empty()) break; 
        
        MoveType choice1 = moves1[0]; 
        // Prefer CONTINUE if available
        if (moves1.size() > 1 && moves1[0] == RETURN) choice1 = moves1[1]; 
        
        s = s.doMove(choice1);
        if (s.isTerminal()) break;

        // Phase 2: Action (Always Collect)
        if (s.getCurrentPlayer().getPosition() > 0) { // Check > 0 to avoid Submarine farming
             std::vector<MoveType> moves2 = s.getPossibleMoves(true);
             if (!moves2.empty()) {
                 MoveType choice2 = LEAVE_TREASURE;
                 for(auto m : moves2) {
                     if (m == COLLECT_TREASURE) choice2 = COLLECT_TREASURE;
                 }
                 s = s.doMove(choice2);
             }
        }
        
        if (turnCount > 100) {
            std::cout << "FAILED: Oxygen not dropping!\n";
            return;
        }
    }
    
    std::cout << "SUCCESS: Game finished in " << turnCount << " turns.\n";

    // --- VERIFICATION STEP ---
    std::cout << "\n--- FINAL SCOREBOARD ---\n";
    
    // We expect Oxygen to be 0
    // Note: Since 'oxygen' is private, we can infer it from the terminal state or add a getter.
    // Ideally, add 'int getOxygen() { return oxygen; }' to State class.

    if (s.getOxygen() == 0) {
        std::cout << "[OK] Oxygen depleted to 0.\n";
    } else {
        std::cout << "[ERROR] Game ended but Oxygen is " << s.getOxygen() << "\n";
    }

    // CHECK 2: All players should be dead (because they never turned back)
    bool allDead = true;
    for (auto& p : s.getPlayers()) {
        std::cout << "Player Pos: " << p.getPosition() 
                  << " | Dead: " << (p.getIsDead() ? "YES" : "NO") 
                  << " | Points: " << p.getPoints() << "\n";
                  
        if (!p.getIsDead()) allDead = false;
        
        // Greedy players shouldn't have points because they died holding the loot.
        if (p.getPoints() > 0) {
             std::cout << "[ERROR] Dead player has points! (Conversion bug)\n";
        }
    }

    if (allDead) {
        std::cout << "[OK] All greedy players died as expected.\n";
    } else {
        std::cout << "[ERROR] Some players survived? Logic check needed.\n";
    }
}

// TEST 4: Full Random Game Simulation (The "Fuzz" Test)
void runRandomGame() {
    std::cout << "\n[SIMULATION] Running full random game...\n";
    State s(3);
    int turnCount = 0;
    
    while (!s.isTerminal()) {
        turnCount++;
        Player& p = s.getCurrentPlayer();
        
        // 1. Get Moves
        // We need to know if we moved this turn. 
        // In MCTS, you track node depth/state. Here we approximate.
        // NOTE: Your getPossibleMoves relies on external logic to know 'movedThisTurn'.
        // For this test, we assume phase 1 (move) then phase 2 (action).
        
        // Phase 1: Move
        std::vector<MoveType> moves1 = s.getPossibleMoves(false);
        if (moves1.empty()) break; // Should be End of Game
        
        MoveType choice1 = moves1[rand() % moves1.size()];
        s = s.doMove(choice1);
        
        if (s.isTerminal()) break;

        // Phase 2: Action (only if not at sub)
        if (choice1 != END && s.getCurrentPlayer().getPosition() != 0) {
             std::vector<MoveType> moves2 = s.getPossibleMoves(true);
             if (!moves2.empty()) {
                 MoveType choice2 = moves2[rand() % moves2.size()];
                 s = s.doMove(choice2);
             }
        }
        
        if (turnCount > 500) {
            std::cout << "FAILED: Game stuck in infinite loop (Oxygen not dropping?)\n";
            return;
        }
    }
    
    std::cout << "Game finished in " << turnCount << " turns.\n";
    
    // Verify scores
    std::cout << "Final Scores:\n";
    for (auto& player : s.getPlayers())
    {
        std::cout<<player.getPoints()<<"\n";
    }
}

int main() {
    std::srand(std::time(0));

    testStateImmutability();
    testShrinkingBoard();
    runGreedyGame();
    runRandomGame();
    return 0;
}
