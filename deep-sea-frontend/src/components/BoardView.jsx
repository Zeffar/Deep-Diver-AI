// Pure Presentational Component
// Renders the board visuals based on fixed coordinates
import { TILE_COORDINATES } from '../constants/LayoutConfig';

function BoardView({ board, players, oxygen }) {
    // Helper: Find players on a specific board index
    const getPlayersAt = (pos) => players.filter(p => p.position === pos)

    return (
        <div className="board-container">
            {/* Submarine (Oxygen Track) */}
            <div className="submarine__container" id="first_sub">
                <div className="submarine__periscope"></div>
                <div className="submarine__periscope-glass"></div>
                <div className="submarine__sail">
                    <div className="submarine__sail-shadow dark1"></div>
                    <div className="submarine__sail-shadow light1"></div>
                    <div className="submarine__sail-shadow dark2"></div>
                </div>
                <div className="submarine__body">
                    <div className="submarine-track">
                        <div className="oxygen-grid-rows" style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: '5px' }}>
                            {/* Row 1: 25 -> 16 (10 slots) */}
                            <div className="oxy-row" style={{ display: 'flex', gap: '5px', justifyContent: 'center' }}>
                                {Array.from({ length: 10 }, (_, i) => 25 - i).map(oxyVal => (
                                    <div key={oxyVal} className={`oxy-slot ${oxygen === oxyVal ? 'active' : ''}`}>
                                        {oxyVal}
                                    </div>
                                ))}
                            </div>

                            {/* Row 2: 15 -> 6 (10 slots) */}
                            <div className="oxy-row" style={{ display: 'flex', gap: '5px', justifyContent: 'center' }}>
                                {Array.from({ length: 10 }, (_, i) => 15 - i).map(oxyVal => (
                                    <div key={oxyVal} className={`oxy-slot ${oxygen === oxyVal ? 'active' : ''}`}>
                                        {oxyVal}
                                    </div>
                                ))}
                            </div>

                            {/* Row 3: 5 -> 0 (6 slots) */}
                            <div className="oxy-row" style={{ display: 'flex', gap: '5px', justifyContent: 'center' }}>
                                {Array.from({ length: 6 }, (_, i) => 5 - i).map(oxyVal => (
                                    <div key={oxyVal} className={`oxy-slot ${oxygen === oxyVal ? 'active' : ''} ${oxyVal === 0 ? 'danger' : ''}`}>
                                        {oxyVal}
                                    </div>
                                ))}
                            </div>
                        </div>
                        <div className="sub-holding-area">
                            {getPlayersAt(0).map(p => (
                                <div key={p.id} className={`player-token p${p.id}`}></div>
                            ))}
                        </div>
                    </div>
                </div>
                <div className="submarine__propeller">
                    <div className="propeller__perspective rotating">
                        <div className="submarine__propeller-parts darkOne"></div>
                        <div className="submarine__propeller-parts lightOne"></div>
                    </div>
                </div>
            </div>

            {/* GAME PATH - Renders 32 Tiles using absolute positioning */}
            <div className="path-grid">

                {board.tiles.map((tile, i) => {
                    const pos = i + 1
                    const playersHere = getPlayersAt(pos)
                    const coords = TILE_COORDINATES[i] || { top: '0%', left: '0%' }

                    const showOriginal = !tile.flipped;
                    const stackItems = tile.stack || [];

                    // VISUAL STACKING LOGIC: Flatten tile contents: Original (Base) + Stacked Items
                    let chipsToRender = [];
                    if (showOriginal) {
                        chipsToRender.push({ isOriginal: true, level: tile.level });
                        stackItems.forEach(t => chipsToRender.push({ ...t, isStacked: true }));
                    } else {
                        if (stackItems.length > 0) {
                            chipsToRender = stackItems.map((t, idx) => ({ ...t, isStacked: idx > 0 }));
                        }
                    }

                    const baseItem = chipsToRender[0];
                    const sideStack = chipsToRender.slice(1);

                    return (
                        <div
                            key={i}
                            className={`tile`}
                            style={{
                                position: 'absolute',
                                left: coords.left,
                                top: coords.top
                            }}
                        >
                            <div className="tile-content">
                                {baseItem ? (
                                    <div className="treasure-grid-layout">
                                        <div className="t-base-area">
                                            <div className={`chip level-${baseItem.level} shape-${baseItem.level}`}></div>
                                        </div>
                                        <div className="t-stack-area">
                                            {[...sideStack].reverse().map((c, idx) => (
                                                <div key={idx} className={`mini-chip shape-${c.level}`}></div>
                                            ))}
                                        </div>
                                    </div>
                                ) : (
                                    <div className="treasure-grid-layout">
                                        <div className="t-base-area">
                                            <div className="empty-slot">
                                                <div className="cross-circle">x</div>
                                            </div>
                                        </div>
                                        <div className="t-stack-area"></div>
                                    </div>
                                )}
                            </div>

                            <div className="players-on-tile">
                                {playersHere.map(p => (
                                    <div key={p.id} className={`player-token p${p.id}`}>
                                        <span className="direction-arrow">{p.isReturning ? '↑' : '↓'}</span>
                                    </div>
                                ))}
                            </div>
                        </div>
                    )
                })}
            </div>
        </div>
    )
}

export default BoardView;
