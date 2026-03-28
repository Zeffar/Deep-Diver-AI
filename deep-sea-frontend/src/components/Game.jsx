import { useState, useEffect, useRef } from 'react'
import { CppBackendAdapter, MoveType } from '../backend/CppAdapter'
import createDeepSeaBackend from '../backend/deep_sea_backend'
import BoardView from './BoardView'
import PlayerView from './PlayerView'

// Main Controller Component
// Bridges Wasm Backend with React UI
function Game({ playerConfigs, onExit }) {
    // Force UI updates via state
    const [gameState, setGameState] = useState(null) // Null = Loading
    const [isLoading, setIsLoading] = useState(true)

    // Hold logic instance in ref
    const gameRef = useRef(null)

    // Initialize Wasm Backend
    useEffect(() => {
        const initGame = async () => {
            try {
                const adapter = new CppBackendAdapter();
                await adapter.init(() => createDeepSeaBackend({
                    locateFile: (path) => {
                        if (path.endsWith('.wasm')) return '/deep_sea_backend.wasm';
                        return path;
                    }
                }), playerConfigs);

                gameRef.current = adapter;
                // Force sync and update
                adapter.syncState();
                setGameState({ ...adapter });
                setIsLoading(false);
            } catch (err) {
                console.error("Failed to load Wasm backend:", err);
            }
        };

        initGame();
    }, [playerConfigs])

    // Helpers
    const updateState = () => {
        if (!gameRef.current) return
        gameRef.current.syncState()
        // console.log(`[React Update] Adapter Phase: ${gameRef.current.turnPhase}, Index: ${gameRef.current.currentPlayerIndex}`);
        setGameState({ ...gameRef.current })
    }

    const handleStartNextRound = () => {
        if (!gameRef.current) return;
        const logic = gameRef.current;

        logic.startNextRound();
        updateState();
    }

    // Helper to finish the round after human is done



    // Bot Logic Loop
    // Watches turnPhase and currentPlayerIndex to trigger bot actions
    useEffect(() => {
        if (!gameState || gameState.isGameOver) return

        if (gameState.turnPhase === 'ROUND_TRANSITION') {
            // Wait for user input
        }

        const currentPlayer = gameState.players?.[gameState.currentPlayerIndex]
        const isBot = gameRef.current && gameRef.current.playerConfigs[gameState.currentPlayerIndex] !== 'Human';

        if (currentPlayer && isBot && gameState.turnPhase !== 'ROUND_TRANSITION') {
            // const delay = Math.floor(Math.random() * (7000 - 3000 + 1)) + 3000
            const delay = 1000;
            const timer = setTimeout(() => {
                playBotTurn()
            }, delay)
            return () => clearTimeout(timer)
        }
    }, [gameState?.currentPlayerIndex, gameState?.turnPhase, gameState?.currentRound, gameState?.turnId])

    // Executes AI logic for current active player
    const playBotTurn = () => {
        if (!gameRef.current) return
        const logic = gameRef.current

        // Use C++ Heuristic Bot
        const botMove = logic.getBotMove();
        console.log(`[Game] Bot chooses: ${botMove}, Phase: ${logic.turnPhase}`);

        if (!botMove) {
            console.warn("[Game] Bot failed to generate move. Skipping.");
            logic.doMove('LEAVE_TREASURE');
            updateState();
            return;
        }

        if (logic.turnPhase === 'ROLL') {
            if (botMove === 'TURN_BACK') {
                logic.doMove('TURN_BACK');
            }
            logic.doMove('ROLL');

            if (logic.turnPhase === 'ACTION') {
                const actionMove = logic.getBotMove();
                console.log(`[Game] Bot action: ${actionMove}`);
                if (actionMove && actionMove !== 'ROLL' && actionMove !== 'TURN_BACK') {
                    logic.doMove(actionMove);
                } else {
                    logic.doMove('LEAVE_TREASURE');
                }
            }
        }
        else if (logic.turnPhase === 'ACTION') {
            logic.doMove(botMove);
        }

        updateState()
    }


    const handleMove = (type) => {
        gameRef.current.doMove(type)
        updateState()
    }

    if (isLoading || !gameState) {
        return <div className="game-container">Loading Deep Sea Adventure Engine...</div>
    }

    const currentType = gameRef.current ? gameRef.current.playerConfigs[gameState.currentPlayerIndex] : 'Bot';
    const isHumanTurn = currentType === 'Human' && !gameState.isGameOver;
    const phase = gameState.turnPhase;
    // Let the human currently playing be the source of controls, else default to player 0
    const controllingPlayer = gameState.players[gameState.currentPlayerIndex] || gameState.players[0];
    const allPlayers = [...gameState.players];

    return (
        <div className="game-container">
            <div className="main-content">
                {/* Left: All Players (Human first) */}
                <div className="left-panel">
                    <PlayerView
                        players={allPlayers}
                        currentId={gameState.players[gameState.currentPlayerIndex]?.id}
                        isGameOver={gameState.isGameOver}
                    />
                </div>

                {/* Middle: Board */}
                <div className="center-board">
                    <div className="round-info">Round {gameState.currentRound}/3</div>
                    {gameState.isGameOver && (
                        <div className="game-over-modal">
                            <div className="game-over-title">Dive Complete!</div>
                            <div className="score-list">
                                {(() => {
                                    // Helper: Strict comparator based on official rules
                                    // Returns: negative if A < B, positive if A > B, 0 if perfectly tied
                                    const comparePlayers = (a, b) => {
                                        // 1. Total Score
                                        if (b.totalScore !== a.totalScore) return b.totalScore - a.totalScore;

                                        // 2. Count of High-Level Chips (3 down to 0)
                                        const countLevel = (p, lvl) => p.bankedTreasures.filter(t => t.level === lvl).length;
                                        for (let lvl = 3; lvl >= 0; lvl--) {
                                            const cA = countLevel(a, lvl);
                                            const cB = countLevel(b, lvl);
                                            if (cB !== cA) return cB - cA;
                                        }
                                        return 0;
                                    };

                                    // 1. Sort players
                                    const sorted = [...gameState.players].sort(comparePlayers);

                                    // 2. Assign ranks (handling ties)
                                    let currentRank = 1;
                                    return sorted.map((p, i) => {
                                        // Check if tied with previous
                                        let isTied = false;
                                        if (i > 0) {
                                            const prev = sorted[i - 1];
                                            isTied = (comparePlayers(prev, p) === 0);
                                        }

                                        if (i > 0 && !isTied) currentRank = i + 1;

                                        return (
                                            <div key={p.id} className={`score-item rank-${currentRank}`}>
                                                <div className="left">
                                                    <span className="rank-badge">#{currentRank}</span>
                                                    <span className="player-name">
                                                        {p.id === 0 ? "YOU" : `Player ${p.id + 1}`}
                                                    </span>
                                                </div>
                                                <div className="score-val">{p.totalScore} pts</div>
                                            </div>
                                        );
                                    });
                                })()}
                            </div>
                        </div>
                    )}
                    <BoardView board={gameState.board} players={gameState.players} oxygen={gameState.oxygen} />
                </div>

                {/* Right: Controls + Logs */}
                <div className="right-panel">
                    <div className="turn-indicator">
                        <span className={`turn-text p-text-${gameState.currentPlayerIndex}`}>
                            {isHumanTurn ? "YOUR TURN" : (
                                gameState.players[gameState.currentPlayerIndex] ?
                                    `PLAYER ${gameState.players[gameState.currentPlayerIndex].id + 1}'S TURN` :
                                    "OPPONENT'S TURN"
                            )}
                        </span>
                    </div>

                    <div className="controls-box">
                        <div className="phase-controls">

                            {gameState.turnPhase === 'ROUND_TRANSITION' && (
                                <button
                                    onClick={() => {
                                        console.log("[React] Clicking Start Next Round");
                                        handleStartNextRound();
                                    }}
                                    className="primary-btn"
                                    style={{ backgroundColor: '#4CAF50', width: '100%', marginBottom: '10px' }}
                                >
                                    Start Next Round
                                </button>
                            )}

                            <button
                                onClick={() => handleMove('ROLL')}
                                disabled={!isHumanTurn || phase !== 'ROLL'}
                            >
                                Roll
                            </button>
                            <button
                                onClick={() => handleMove('TURN_BACK')}
                                disabled={
                                    !isHumanTurn ||
                                    phase !== 'ROLL' ||
                                    controllingPlayer.isReturning ||
                                    controllingPlayer.roundTreasures.length === 0
                                }
                                className="secondary-btn"
                            >
                                Turn Back
                            </button>
                        </div>

                        <div className="phase-controls">
                            {(() => {
                                const pos = controllingPlayer.position
                                // Robustness: check if board tile exists
                                const tile = (pos > 0 && gameState.board.tiles[pos - 1]) ? gameState.board.tiles[pos - 1] : null;

                                // FORCE HIDE if transitioning
                                if (gameState.turnPhase === 'ROUND_TRANSITION') return null;

                                const canPick = isHumanTurn && phase === 'ACTION' && tile && (!tile.flipped || tile.stack.length > 0)
                                const canDrop = isHumanTurn && phase === 'ACTION' && controllingPlayer.roundTreasures.length > 0 && tile && tile.flipped && tile.stack.length === 0
                                const canWait = isHumanTurn && phase === 'ACTION'

                                return (
                                    <>
                                        <button onClick={() => handleMove(MoveType.COLLECT_TREASURE)} disabled={!canPick}>
                                            Pick Up
                                        </button>
                                        <button onClick={() => handleMove(MoveType.DROP_TREASURE)} disabled={!canDrop}>
                                            Drop
                                        </button>
                                        <button onClick={() => handleMove(MoveType.LEAVE_TREASURE)} className="secondary-btn" disabled={!canWait}>
                                            Do Nothing
                                        </button>
                                    </>
                                )
                            })()}
                        </div>

                        <button className="exit-btn" onClick={onExit}>Exit Game</button>
                    </div>

                    <div className="status-panel-right">
                        <div className="logs">
                            {gameState.log?.slice().reverse().map((l, i) => <div key={i}>{l}</div>)}
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}
export default Game
