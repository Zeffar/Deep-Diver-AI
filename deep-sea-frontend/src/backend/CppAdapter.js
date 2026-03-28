import createMyModule from './deep_sea_backend.js';
import wasmUrl from './deep_sea_backend.wasm?url';

// Move type constants for player actions
export const MoveType = {
    CONTINUE: 'CONTINUE',
    RETURN: 'RETURN',
    COLLECT_TREASURE: 'COLLECT_TREASURE',
    LEAVE_TREASURE: 'LEAVE_TREASURE',
    DROP_TREASURE: 'DROP_TREASURE',
    END: 'END'
};

// Adapter bridging C++ Wasm backend with React frontend
// Manages game state synchronization and move execution
export class CppBackendAdapter {
    constructor() {
        this.module = null;
        this.state = null;
        this.players = [];
        this.board = { tiles: [] };

        // Game state
        this.currentPlayerIndex = 0;
        this.currentRound = 1;
        this.oxygen = 25;
        this.turnPhase = 'ROLL'; // ROLL | ACTION | ROUND_TRANSITION | END
        this.log = ["Welcome to Deep Sea Adventure!"];
        this.isGameOver = false;

        // Dice state
        this.diceRoll = 2;
        this.lastMoveDistance = 0;

        // Internal state tracking
        this.turnId = 0;
        this.pendingReturn = false;

        // Round transition state
        this.maskSync = false;
        this.serverRound = 1;
        this.lastReturningPlayerIndex = 0;
        this.sealedBoardIndex = -1;
    }

    // Initialize Wasm module and game state
    async init(wasmModuleFactory, playerConfigs) {
        const numPlayers = playerConfigs.length;
        this.playerConfigs = playerConfigs;
        const moduleArgs = {
            print: (text) => console.log(`[CPP] ${text}`),
            printErr: (text) => console.error(`[CPP ERR] ${text}`),
            locateFile: (path, scriptDirectory) => {
                if (path.endsWith('.wasm')) {
                    return wasmUrl + "?t=" + Date.now();
                }
                return scriptDirectory + path;
            }
        };
        this.module = await wasmModuleFactory(moduleArgs);
        this.state = new this.module.State(numPlayers);

        this.bots = [];
        for (let i = 0; i < numPlayers; i++) {
            const botType = playerConfigs[i];
            let botInst = null;
            try {
                if (botType === 'HeuristicBot') {
                    botInst = new this.module.HeuristicBot(numPlayers);
                } else if (botType === 'MCTS') {
                    botInst = new this.module.MCTS(numPlayers, 100000, 1.41); // fast setup
                } else if (botType === 'PureMCTS') {
                    botInst = new this.module.PureMCTS(numPlayers, 1000);
                } else if (botType === 'ParallelMCTS') {
                    botInst = new this.module.ParallelMCTS(numPlayers, 50000, 1.41, 0);
                }
            } catch (e) {
                console.error(`Failed to initialize ${botType} for player ${i}:`, e);
            }
            this.bots.push(botInst);
        }

        this.syncState();
    }

    // Get best move from the current player's bot
    getBotMove() {
        if (!this.state || !this.bots) return null;

        const currentBot = this.bots[this.currentPlayerIndex];
        if (!currentBot) return null; // Human player or failed to init

        const movedThisTurn = (this.turnPhase === 'ACTION');
        const bestMove = currentBot.findBestMove(this.state, this.currentPlayerIndex, movedThisTurn);

        // Map C++ enum to adapter strings
        if (bestMove === this.module.MoveType.CONTINUE) return 'ROLL';
        if (bestMove === this.module.MoveType.RETURN) return 'TURN_BACK';
        if (bestMove === this.module.MoveType.COLLECT_TREASURE) return 'COLLECT_TREASURE';
        if (bestMove === this.module.MoveType.DROP_TREASURE) return 'DROP_TREASURE';
        if (bestMove === this.module.MoveType.LEAVE_TREASURE) return 'LEAVE_TREASURE';

        return 'LEAVE_TREASURE';
    }

    // Get current player with optimistic UI state
    getCurrentPlayer() {
        if (!this.players.length) return { id: 0, position: 0 };
        const p = this.players[this.currentPlayerIndex];

        // Apply pending return state for immediate UI feedback
        if (this.pendingReturn && p && p.id === this.currentPlayerIndex) {
            p.isReturning = true;
        }
        return p;
    }

    // Execute a player move based on current phase
    doMove(moveType) {
        this.turnId++;
        if (this.isGameOver) return this;

        // Roll phase: handle direction and dice roll
        if (this.turnPhase === 'ROLL') {
            if (moveType === 'TURN_BACK') {
                if (this.pendingReturn) return this;
                const p = this.getCurrentPlayer();
                if (!p.isReturning && p.roundTreasures.length > 0) {
                    this.pendingReturn = true;
                    p.isReturning = true;
                    this.log.push(`P${p.id + 1} turns back.`);
                }
                return this;
            }

            if (moveType === 'ROLL') {
                return this.handleRoll();
            }
        }

        // Action phase: pick up, drop, or leave treasure
        if (this.turnPhase === 'ACTION') {
            return this.handleAction(moveType);
        }

        return this;
    }

    // Handle dice roll and movement
    handleRoll() {
        const cppMove = this.pendingReturn ? this.module.MoveType.RETURN : this.module.MoveType.CONTINUE;
        const p = this.getCurrentPlayer();
        const startPos = p.position;
        const weight = p.roundTreasures.length;
        const wasReturning = p.isReturning || this.pendingReturn;

        if (weight > 0 && (p.position > 0 || wasReturning)) {
            this.log.push(`P${p.id + 1} consumes ${weight} Air.`);
        }

        const oldPlayerIndex = this.state.getCurrentPlayerIndex();
        const oldRound = this.state.getCurrentRound();

        // Execute move in C++
        const newState = this.state.doMove(cppMove);
        this.state = newState;

        // Allow action phase to complete even on round change (Last Gasp mechanic)
        this.syncState();

        // Handle game over - return early to prevent phase changes
        if (this.isGameOver) {
            this.pendingReturn = false;
            return this;
        }

        // Handle round transition
        if (this.maskSync) {
            this.turnPhase = 'ROUND_TRANSITION';
            return this;
        }

        // Infer dice roll for logging
        const newP = this.players[p.id];
        const dist = Math.abs(newP.position - startPos);
        const skips = this.calculateSkips(startPos, newP.position, wasReturning);
        const inferredRoll = (dist - skips) + weight;

        if (weight <= 0 && skips <= 0) {
            this.log.push(`P${p.id + 1} rolls ${inferredRoll}, moves ${dist}.`);
        } else if (weight > 0 && skips <= 0) {
            this.log.push(`P${p.id + 1} rolls ${inferredRoll}, moves ${dist} (-${weight} trs).`);
        } else if (weight <= 0 && skips > 0) {
            this.log.push(`P${p.id + 1} rolls ${inferredRoll}, moves ${dist} (+${skips} skips).`);
        } else {
            this.log.push(`P${p.id + 1} rolls ${inferredRoll}, moves ${dist} (-${weight} trs, +${skips} skips).`);
        }

        // Check if player reached submarine
        if (newP.position === 0 && newP.isReturning) {
            this.log.push(`P${p.id + 1} returns to Submarine!`);
            this.lastReturningPlayerIndex = p.id;

            // Skip action phase if turn passed to next player
            const newIndex = this.state.getCurrentPlayerIndex();
            if (newIndex !== oldPlayerIndex) {
                this.currentPlayerIndex = newIndex;
                this.pendingReturn = false;
                this.turnPhase = 'ROLL';
                return this;
            }
        } else if (dist === 0 && startPos === newP.position) {
            this.log.push(`P${p.id + 1} is too heavy to move.`);
        }

        this.pendingReturn = false;
        this.turnPhase = 'ACTION';
        return this;
    }

    // Handle treasure pick up, drop, or leave actions
    handleAction(moveType) {
        let cppMove = this.module.MoveType.LEAVE_TREASURE;
        const p = this.getCurrentPlayer();
        const oxygenBeforeAction = this.state.getOxygen();

        if (moveType === 'COLLECT_TREASURE') {
            cppMove = this.module.MoveType.COLLECT_TREASURE;
            this.log.push(`P${p.id + 1} picked up treasure.`);

            // Handle last gasp: manually update JS state since C++ resets on round end
            if (oxygenBeforeAction === 0 && p.position > 0) {
                const tile = this.board.tiles[p.position - 1];
                if (tile && !tile.flipped) {
                    tile.flipped = true;
                    const newTreasure = { level: tile.level, value: tile.level };
                    p.roundTreasures.push(newTreasure);
                }
            }
        } else if (moveType === 'DROP_TREASURE') {
            cppMove = this.module.MoveType.DROP_TREASURE;
            this.log.push(`P${p.id + 1} dropped treasure.`);

            // Handle last gasp: manually update JS state since C++ resets on round end
            if (oxygenBeforeAction === 0 && p.position > 0 && p.roundTreasures.length > 0) {
                const tile = this.board.tiles[p.position - 1];
                if (tile && tile.flipped) {
                    // Find lowest value treasure to drop
                    let minIndex = 0;
                    let minValue = Infinity;
                    p.roundTreasures.forEach((t, i) => {
                        const val = t.type === 'stack'
                            ? t.children.reduce((sum, c) => sum + c.level, 0)
                            : t.level;
                        if (val < minValue) {
                            minValue = val;
                            minIndex = i;
                        }
                    });

                    const dropped = p.roundTreasures.splice(minIndex, 1)[0];
                    tile.flipped = false;
                    tile.level = dropped.type === 'stack' ? dropped.children[0].level : dropped.level;
                    tile.stack = dropped.type === 'stack' ? dropped.children.slice(1) : [];
                }
            }
        } else {
            this.log.push(`P${p.id + 1} did nothing.`);
        }

        const newState = this.state.doMove(cppMove);
        this.state = newState;
        this.syncState();

        if (this.isGameOver) {
            this.turnPhase = 'END';
        } else if (this.maskSync) {
            this.turnPhase = 'ROUND_TRANSITION';
        } else {
            this.currentPlayerIndex = this.state.getCurrentPlayerIndex();
            this.turnPhase = 'ROLL';
        }
        return this;
    }

    // Calculate number of tiles skipped due to occupied spaces
    calculateSkips(start, end, wasReturning) {
        if (start === end) return 0;
        let skips = 0;
        const dir = wasReturning ? -1 : 1;
        let pos = start;
        let steps = 0;

        while (pos !== end && steps < 50) {
            steps++;
            pos += dir;
            if (pos <= 0 || pos > this.board.tiles.length) break;
            if (pos !== end && this.board.tiles[pos - 1]?.occupied) {
                skips++;
            }
        }
        return skips;
    }

    // Synchronize JS state with C++ backend
    syncState() {
        const newOxygen = this.state.getOxygen();

        // Detect round end: oxygen reset indicates new round in C++
        if (!this.maskSync && newOxygen > this.oxygen && this.oxygen < 25) {
            // Preserve JS state since C++ has already reset
            this.maskSync = true;
            this.oxygen = 0;
            this.serverRound++;

            // Process round end using current JS state (NOT C++ which has reset)
            this.players.forEach(p => {
                if (p.position > 0) {
                    // Player drowned - mark dead, keep treasures where they are
                    p.isDead = true;
                } else if (p.position === 0 && p.isReturning) {
                    // Player returned safely - bank their treasures
                    const flat = [];
                    p.roundTreasures.forEach(t => {
                        if (t.type === 'stack') flat.push(...t.children);
                        else flat.push(t);
                    });

                    // Calculate and add values for each treasure
                    flat.forEach(treasure => {
                        treasure.value = treasure.value || treasure.level; // Use existing or default
                        p.bankedTreasures.push(treasure);
                        p.totalScore += treasure.value;
                    });

                    p.roundTreasures = []; // Clear held after banking
                }
            });

            this.turnPhase = 'ROUND_TRANSITION';
            this.log.push("Round Ended.");
            return;
        }

        if (this.maskSync) return;

        // Standard sync from C++ state
        this.oxygen = newOxygen;

        // Check for game over (last round terminal)
        const wasGameOver = this.isGameOver;
        this.isGameOver = (this.state.isLastRound() && this.state.isTerminal());

        if (this.isGameOver && !wasGameOver) {
            // Game just ended - preserve JS state same as round transition
            this.turnPhase = 'END';

            // Process game end using current JS state
            this.players.forEach(p => {
                if (p.position > 0) {
                    // Player drowned - mark dead
                    p.isDead = true;
                } else if (p.position === 0 && p.isReturning) {
                    // Player returned safely - bank their treasures
                    const flat = [];
                    p.roundTreasures.forEach(t => {
                        if (t.type === 'stack') flat.push(...t.children);
                        else flat.push(t);
                    });

                    flat.forEach(treasure => {
                        treasure.value = treasure.value || treasure.level;
                        p.bankedTreasures.push(treasure);
                        p.totalScore += treasure.value;
                    });

                    p.roundTreasures = [];
                }
            });

            this.log.push("Game Over!");
            return;
        }

        if (this.isGameOver) {
            this.turnPhase = 'END';
            return;
        }

        this._syncPlayers();
        this._syncBoard();

        // Sync player index
        const backendPlayer = this.state.getCurrentPlayerIndex();
        if (this.currentPlayerIndex !== backendPlayer) {
            this.currentPlayerIndex = backendPlayer;
        }
    }

    // Sync player data from C++ to JS
    _syncPlayers() {
        const cppPlayersVector = this.state.getPlayers();
        this.players = [];
        for (let i = 0; i < cppPlayersVector.size(); i++) {
            const p = cppPlayersVector.get(i);
            this.players.push({
                id: i,
                position: p.position,
                isDead: p.isDead,
                isReturning: p.isReturning,
                totalScore: p.getPoints(),
                roundTreasures: this._convertInventory(p.getTreasures()),
                bankedTreasures: [] // Not returned/used by C++ backend
            });
        }

        if (this.pendingReturn && this.players[this.currentPlayerIndex]) {
            this.players[this.currentPlayerIndex].isReturning = true;
        }
    }

    // Sync board data from C++ to JS
    _syncBoard() {
        const cppBoard = this.state.getBoard();
        const tilesVector = cppBoard.getTiles();
        this.board.tiles = [];
        for (let i = 0; i < tilesVector.size(); i++) {
            const t = tilesVector.get(i);
            const treasures = [];
            for (let j = 0; j < t.treasure.size(); j++) {
                treasures.push({ level: t.treasure.get(j), value: 0 });
            }
            this.board.tiles.push({
                level: t.level,
                flipped: t.flipped,
                occupied: t.occupied,
                stack: treasures
            });
        }
    }

    // Convert C++ inventory to JS format
    _convertInventory(cppInventory) {
        const result = [];
        for (let i = 0; i < cppInventory.size(); i++) {
            const stack = cppInventory.get(i);
            const children = [];

            for (let j = 0; j < stack.size(); j++) {
                let lvl = stack.get(j);

                // Validate level data
                if (typeof lvl !== 'number') {
                    console.error("CppAdapter: Invalid level from C++", lvl);
                    lvl = 0;
                }
                lvl = Math.max(0, Math.min(4, lvl));

                children.push({ level: lvl, value: lvl });
            }

            if (children.length === 1) {
                result.push(children[0]);
            } else {
                result.push({
                    type: 'stack',
                    children: children,
                    value: children.reduce((sum, c) => sum + c.value, 0)
                });
            }
        }
        return result;
    }

    // Convert banked chips from C++ to JS format
    _convertBankedChips(cppLevels, cppValues) {
        const res = [];
        const hasValues = cppValues && typeof cppValues.get === 'function';

        for (let i = 0; i < cppLevels.size(); i++) {
            const lvl = cppLevels.get(i);
            const val = hasValues ? cppValues.get(i) : 0;
            res.push({ level: lvl, value: val });
        }
        return res;
    }

    // Convert C++ vector to JS array
    _vectorToJsArray(cppVector) {
        const res = [];
        for (let i = 0; i < cppVector.size(); i++) {
            res.push(cppVector.get(i));
        }
        return res;
    }

    // Start the next round after transition
    startNextRound() {
        if (!this.maskSync) return;
        this.prepareRoundEnd();
        this.finalizeRound();
    }

    checkRoundEnd() { return this.maskSync || this.state.isTerminal(); }

    // Prepare board for round transition
    prepareRoundEnd() {
        this.board.tiles.forEach(t => t.occupied = false);
        this.board.tiles = this.board.tiles.filter(t => !(t.flipped && t.stack.length === 0));
        this.sealedBoardIndex = this.board.tiles.length - 1;
        return this;
    }

    // Reset all player positions to submarine
    resetAllPositions() { this.players.forEach(p => p.position = 0); }

    // Drop drowned player's loot to end of board
    dropPlayerLoot(playerId) {
        if (!this.maskSync) return;
        const p = this.players[playerId];
        if (!p || p.roundTreasures.length === 0) return;

        // Flatten stacked treasures
        const flat = [];
        p.roundTreasures.forEach(t => {
            if (t.type === 'stack') flat.push(...t.children);
            else flat.push(t);
        });
        p.roundTreasures = [];

        // Shuffle treasures
        for (let i = flat.length - 1; i > 0; i--) {
            const j = Math.floor(Math.random() * (i + 1));
            [flat[i], flat[j]] = [flat[j], flat[i]];
        }

        // Distribute to board tail
        flat.forEach(item => {
            const lastTile = this.board.tiles[this.board.tiles.length - 1];
            const isNewStack = (this.board.tiles.length - 1) > (this.sealedBoardIndex);

            if (isNewStack && lastTile && lastTile.flipped && lastTile.stack.length < 3) {
                lastTile.stack.push(item);
            } else {
                this.board.tiles.push({
                    level: item.level,
                    flipped: true,
                    occupied: false,
                    stack: [item]
                });
            }
        });
    }

    // Finalize round transition and sync state
    finalizeRound() {
        this.maskSync = false;
        this.oxygen = this.state.getOxygen();
        this.syncState();

        this.currentPlayerIndex = this.state.getCurrentPlayerIndex();
        this.turnPhase = 'ROLL';

        const uiRound = this.state.getCurrentRound() + 1;
        this.log.push(`Round ${uiRound} Begins!`);
        return this;
    }
}