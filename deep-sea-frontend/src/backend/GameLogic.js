export const MoveType = {
    CONTINUE: 'CONTINUE',
    RETURN: 'RETURN',
    COLLECT_TREASURE: 'COLLECT_TREASURE',
    LEAVE_TREASURE: 'LEAVE_TREASURE',
    DROP_TREASURE: 'DROP_TREASURE',
    END: 'END'
};

class Tile {
    constructor(level) {
        this.level = level;
        this.flipped = false;
        this.occupied = false;
        this.width = 1;
        this.stack = [];
    }

    static tileValues = {
        0: [0, 0, 1, 1, 2, 2, 3, 3],
        1: [4, 4, 5, 5, 6, 6, 7, 7],
        2: [8, 8, 9, 9, 10, 10, 11, 11],
        3: [12, 12, 13, 13, 14, 14, 15, 15]
    };

    static calculateValue(level) {
        const possibilities = Tile.tileValues[level];
        return possibilities[Math.floor(Math.random() * possibilities.length)];
    }
}

class Board {
    constructor() {
        this.tiles = [];
        const levels = [0, 1, 2, 3];
        levels.forEach(lvl => {
            for (let i = 0; i < 8; i++) {
                const t = new Tile(lvl);
                t.stack = [];
                this.tiles.push(t);
            }
        });
    }
}

class Player {
    constructor(id) {
        this.id = id;
        this.position = 0;
        this.isDead = false;
        this.isReturning = false;
        this.roundTreasures = [];
        this.bankedTreasures = [];
        this.totalScore = 0;
    }

    resetForRound() {
        this.position = 0;
        this.isDead = false;
        this.isReturning = false;
        this.roundTreasures = [];
    }

    resetPositionOnly() {
        this.position = 0;
    }

    getWeight() {
        return this.roundTreasures.length;
    }
}

export class GameState {
    constructor(numPlayers = 2) {
        this.players = Array.from({ length: numPlayers }, (_, i) => new Player(i));
        this.board = new Board();
        this.oxygen = 25;
        this.currentPlayerIndex = 0;
        this.currentRound = 1;
        this.isGameOver = false;
        this.turnPhase = 'ROLL';
        this.log = ["Welcome to Deep Sea Adventure!"];
        this.lastReturningPlayerIndex = 0;
        this.isLastTurn = false;
        this.turnId = 0;
    }

    getCurrentPlayer() {
        return this.players[this.currentPlayerIndex];
    }

    throwDice() {
        return Math.floor(Math.random() * 3) + 1 + Math.floor(Math.random() * 3) + 1;
    }

    checkRoundEnd() {
        return this.players.every(p => p.isDead || (p.position === 0 && p.isReturning));
    }

    doMove(moveType) {
        if (this.isGameOver) return this;
        this.turnId++;
        const player = this.getCurrentPlayer();

        if (this.turnPhase === 'ROLL') {
            if (moveType === 'TURN_BACK') {
                if (!player.isReturning) {
                    if (player.roundTreasures.length === 0) {
                        this.log.push(`P${player.id + 1} cannot turn back empty-handed!`);
                        return this;
                    }
                    player.isReturning = true;
                    this.log.push(`P${player.id + 1} turns back.`);
                }
                return this;
            }

            if (moveType === 'ROLL') {
                const weight = player.getWeight();
                if (player.position > 0 || player.isReturning) {
                    this.oxygen -= weight;
                    if (this.oxygen <= 0) {
                        this.oxygen = 0;

                    }
                    if (weight > 0) this.log.push(`P${player.id + 1} consumes ${weight} Air.`);
                }

                const roll = this.throwDice();
                const moves = Math.max(0, roll - weight);
                const startPos = player.position;
                this.handleMovement(player, moves);

                const actualDist = Math.abs(player.position - startPos);
                this.log.push(`P${player.id + 1} rolls ${roll} (Moves: ${actualDist})`);

                if (player.position === 0 && player.isReturning) {
                    this.log.push(`P${player.id + 1} returns to Submarine!`);
                    this.lastReturningPlayerIndex = player.id;
                    this.nextTurn();
                } else {
                    if (startPos === player.position && moves === 0) {
                        this.log.push(`P${player.id + 1} is too heavy to move.`);
                    }
                    this.turnPhase = 'ACTION';
                }
            }
            return this;
        }

        if (this.turnPhase === 'ACTION') {
            if (moveType === MoveType.COLLECT_TREASURE) this.handleCollect(player);
            else if (moveType === MoveType.DROP_TREASURE) this.handleDrop(player);
            else this.log.push(`P${player.id + 1} waits.`);

            this.nextTurn();
        }
        return this;
    }

    handleMovement(player, points) {
        if (points <= 0) return;

        let pos = player.position;
        const dir = player.isReturning ? -1 : 1;

        if (pos > 0) this.board.tiles[pos - 1].occupied = false;

        while (points > 0) {
            pos += dir;

            if (pos <= 0) {
                pos = 0;
                break;
            }

            if (pos > this.board.tiles.length) {
                pos = this.board.tiles.length;

                while (pos > 0 && (pos > this.board.tiles.length || this.board.tiles[pos - 1].occupied)) {
                    pos--;
                }
                break;
            }

            if (pos > 0 && pos <= this.board.tiles.length) {
                const tile = this.board.tiles[pos - 1];
                if (!tile.occupied) {
                    points--;
                }
            }
        }

        if (pos < 0) pos = 0;

        player.position = pos;
        if (pos > 0) this.board.tiles[pos - 1].occupied = true;
    }

    handleCollect(player) {
        if (player.position === 0) return;
        const tile = this.board.tiles[player.position - 1];

        const hasStack = tile.stack.length > 0;
        const hasOriginal = !tile.flipped;

        if (hasStack || hasOriginal) {

            const collectedChildren = [];

            if (hasOriginal) {
                collectedChildren.push({
                    level: tile.level,
                    value: Tile.calculateValue(tile.level)
                });
            }

            collectedChildren.push(...tile.stack);

            let itemToAdd;
            if (collectedChildren.length === 1) {
                const c = collectedChildren[0];
                itemToAdd = { type: 'chip', level: c.level, value: c.value };
            } else {
                itemToAdd = {
                    type: 'stack',
                    children: collectedChildren,
                    value: collectedChildren.reduce((sum, t) => sum + (t.value || 0), 0)
                };
            }

            player.roundTreasures.push(itemToAdd);

            tile.stack = [];
            tile.flipped = true;
        }
    }

    handleDrop(player) {
        if (player.position === 0) return;
        const tile = this.board.tiles[player.position - 1];
        if (!tile.flipped || tile.stack.length > 0) {
            this.log.push(`P${player.id + 1} cannot drop here (Occupied).`);
            return;
        }

        if (player.roundTreasures.length > 0) {
            const getMinLvl = (it) => it.type === 'stack' ? Math.min(...it.children.map(c => c.level)) : it.level;

            let dropIdx = 0;
            let minV = 999;
            player.roundTreasures.forEach((it, i) => {
                const v = getMinLvl(it);
                if (v < minV) { minV = v; dropIdx = i; }
            });

            const dropped = player.roundTreasures.splice(dropIdx, 1)[0];

            if (dropped.type === 'stack') {
                tile.stack.push(...dropped.children);
            } else {
                tile.stack.push(dropped);
            }
        }
    }

    nextTurn() {
        if (this.oxygen <= 0 || this.checkRoundEnd()) {
            this.endRound();
            return;
        }

        this.turnPhase = 'ROLL';
        let loops = 0;
        let idx = this.currentPlayerIndex;
        do {
            idx = (idx + 1) % this.players.length;
            loops++;
            const p = this.players[idx];
            if (!p.isDead && (p.position > 0 || (p.position === 0 && !p.isReturning))) {
                this.currentPlayerIndex = idx;
                return;
            }
        } while (loops < this.players.length);

        this.endRound();
    }

    endRound() {
        this.players.forEach(p => {
            if (p.position === 0 && !p.isDead) {
                const flat = [];
                p.roundTreasures.forEach(t => {
                    if (t.type === 'stack') flat.push(...t.children);
                    else flat.push(t);
                });
                p.bankedTreasures.push(...flat);
                p.roundTreasures = [];
            } else {
                p.isDead = true;
            }
            p.totalScore = p.bankedTreasures.reduce((s, t) => s + (t.value || 0), 0);
        });

        if (this.currentRound >= 3) {
            this.isGameOver = true;
            this.turnPhase = 'END';
            return;
        }

        this.turnPhase = 'ROUND_TRANSITION';
    }

    resetAllPositions() {
        this.players.forEach(p => p.resetPositionOnly());
    }

    prepareRoundEnd() {
        if (this.isGameOver) return;

        this.board.tiles.forEach(t => t.occupied = false);

        this.board.tiles = this.board.tiles.filter(t => {
            return !(t.flipped && t.stack.length === 0);
        });

        this.sealedBoardIndex = this.board.tiles.length - 1;

        return this;
    }

    dropPlayerLoot(playerId) {
        const p = this.players[playerId];
        if (!p || !p.isDead || p.roundTreasures.length === 0) return;

        const flat = [];
        p.roundTreasures.forEach(t => {
            if (t.type === 'stack') flat.push(...t.children);
            else flat.push(t);
        });
        p.roundTreasures = [];

        // Randomize drop order for EVERYONE (Human + Bots)
        for (let i = flat.length - 1; i > 0; i--) {
            const j = Math.floor(Math.random() * (i + 1));
            [flat[i], flat[j]] = [flat[j], flat[i]];
        }

        flat.forEach(item => this.addDropToBoard(item));
    }

    addDropToBoard(item) {
        const lastTile = this.board.tiles[this.board.tiles.length - 1];

        const isNewStack = (this.board.tiles.length - 1) > (this.sealedBoardIndex !== undefined ? this.sealedBoardIndex : -999);

        if (isNewStack && lastTile && lastTile.flipped && lastTile.stack.length < 3) {
            lastTile.stack.push(item);
        } else {
            const newT = new Tile(item.level);
            newT.stack = [item];
            newT.flipped = true;
            this.board.tiles.push(newT);
        }
    }

    flushPendingDrops() {
        if (!this.pendingDropStack || this.pendingDropStack.length === 0) return;

        const stackItems = [...this.pendingDropStack];
        this.pendingDropStack = [];

        const newT = new Tile(stackItems[0].level);
        newT.stack = stackItems;
        newT.flipped = true;
        this.board.tiles.push(newT);
    }

    finalizeRound() {
        this.flushPendingDrops();

        this.currentRound++;
        this.oxygen = 25;
        this.isLastTurn = false;
        this.currentPlayerIndex = this.lastReturningPlayerIndex;
        this.players.forEach(p => {
            p.resetForRound();
        });

        if (this.currentRound > 3) {
            this.isGameOver = true;
            this.turnPhase = 'END';
        } else {
            this.turnPhase = 'ROLL';
            this.log.push(`Round ${this.currentRound} Begins! Air Refilled.`);
        }
    }
}
