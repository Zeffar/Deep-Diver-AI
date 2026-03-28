import { describe, it, expect, beforeEach, vi } from 'vitest';
import { GameState, MoveType, Tile } from './GameLogic';

describe('Deep Sea Adventure GameLogic', () => {
    let game;

    beforeEach(() => {
        game = new GameState(2);
        game.throwDice = vi.fn(() => 6);
    });

    it('initializes correctly', () => {
        expect(game.oxygen).toBe(25);
        expect(game.players.length).toBe(2);
        expect(game.currentRound).toBe(1);
        expect(game.turnPhase).toBe('ROLL');
        expect(game.board.tiles.length).toBe(32);
    });

    describe('Movement & Oxygen', () => {
        it('consumes oxygen based on treasure weight', () => {
            const p1 = game.players[0];
            p1.roundTreasures = [{ level: 0 }, { level: 0 }];

            game.doMove('ROLL');
            expect(game.oxygen).toBe(25);

            game.nextTurn();
            game.doMove('ROLL');

            game.currentPlayerIndex = 0;
            game.turnPhase = 'ROLL';

            game.doMove('ROLL');
            expect(game.oxygen).toBe(23);
        });

        it('reduces movement points by weight', () => {
            const p1 = game.players[0];
            p1.roundTreasures = [{ level: 0 }, { level: 0 }];
            game.throwDice = vi.fn(() => 5);

            game.doMove('ROLL');
            expect(p1.position).toBe(3);
        });

        it('prevents movement if weight >= roll', () => {
            const p1 = game.players[0];
            p1.roundTreasures = [{ level: 0 }, { level: 0 }, { level: 0 }];
            game.throwDice = vi.fn(() => 2);

            game.doMove('ROLL');
            expect(p1.position).toBe(0);
        });

        it('implements leapfrog (skips occupied tiles)', () => {
            const p1 = game.players[0];
            const p2 = game.players[1];

            p2.position = 3;
            game.board.tiles[2].occupied = true;

            game.throwDice = vi.fn(() => 3);
            game.doMove('ROLL');

            expect(p1.position).toBe(4);
        });
    });

    describe('Treasure Actions', () => {
        it('collects treasure and flips tile', () => {
            const p1 = game.players[0];
            p1.position = 1;
            game.turnPhase = 'ACTION';

            game.doMove(MoveType.COLLECT_TREASURE);

            expect(p1.roundTreasures.length).toBe(1);
            expect(game.board.tiles[0].flipped).toBe(true);
        });

        it('drops treasure correctly (lowest value first)', () => {
            const p1 = game.players[0];
            p1.position = 1;
            game.board.tiles[0].flipped = true;

            p1.roundTreasures = [
                { type: 'chip', level: 2 },
                { type: 'chip', level: 0 },
                { type: 'chip', level: 1 }
            ];

            game.turnPhase = 'ACTION';
            game.doMove(MoveType.DROP_TREASURE);

            expect(game.board.tiles[0].stack.length).toBe(1);
            expect(game.board.tiles[0].stack[0].level).toBe(0);
            expect(p1.roundTreasures.length).toBe(2);
            const levels = p1.roundTreasures.map(t => t.level);
            expect(levels).toContain(2);
            expect(levels).toContain(1);
        });

        it('collects a stack as a single item', () => {
            const p1 = game.players[0];
            p1.position = 1;
            const tile = game.board.tiles[0];

            tile.flipped = true;
            tile.stack = [
                { type: 'chip', level: 0 },
                { type: 'chip', level: 0 }
            ];

            game.turnPhase = 'ACTION';
            game.doMove(MoveType.COLLECT_TREASURE);

            expect(p1.roundTreasures.length).toBe(1);
            expect(p1.roundTreasures[0].type).toBe('stack');
            expect(p1.roundTreasures[0].children.length).toBe(2);
        });
    });

    describe('Round End & Logic', () => {
        it('ends round when oxygen hits 0', () => {
            game.oxygen = 1;
            const p1 = game.players[0];
            p1.position = 5;
            p1.roundTreasures = [{ level: 0 }];

            game.turnPhase = 'ROLL';
            game.currentPlayerIndex = 0;

            game.doMove('ROLL');
            expect(game.isLastTurn).toBe(true);
            expect(game.oxygen).toBe(0);

            game.doMove(MoveType.LEAVE_TREASURE);

            expect(game.turnPhase).toBe('ROUND_TRANSITION');
        });

        it('distributes loot preserving blanks', () => {
            const p1 = game.players[0];
            p1.position = 10;
            p1.roundTreasures = [{ type: 'chip', level: 3 }];

            game.players[1].isDead = true;

            const lastIdx = game.board.tiles.length - 1;
            game.board.tiles[lastIdx].flipped = true;
            game.board.tiles[lastIdx].stack = [];

            game.endRound();
            const initialBoardSize = game.board.tiles.length;
            game.startNextRound();

            expect(game.board.tiles.length).toBe(initialBoardSize - 1);

            expect(game.board.tiles[game.board.tiles.length - 1].stack.length).toBeGreaterThan(0);
        });
    });
});
