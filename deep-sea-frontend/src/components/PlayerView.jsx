// Renders player dashboard (Left Panel)
// Displays Identity, Held Treasures, Banked Score
function PlayerView({ players, currentId, isGameOver = false }) {

    // Helper: Renders a single "Held" item.
    // Logic matches BoardView: Stacks are visualized as (Base + Mini Chips).
    const renderHeldItem = (item, key) => {
        let chipsToRender = [];

        if (item.type === 'stack') {
            chipsToRender = item.children.map((c, i) => ({ ...c, isStacked: i > 0 }));
        } else {
            chipsToRender = [item];
        }

        const baseItem = chipsToRender[0];
        const sideStack = chipsToRender.slice(1);

        return (
            <div key={key} className="treasure-grid-layout"
                style={{ cursor: 'default' }}
            >
                {baseItem ? (
                    <>
                        {/* Area: base (2/3 width) - Grid Area 'base' */}
                        <div className="t-base-area">
                            <div className={`chip level-${baseItem.level} shape-${baseItem.level}`}></div>
                        </div>

                        {/* Area: stack (1/3 width) - Grid Area 'stack' */}
                        <div className="t-stack-area">
                            {/* Render items in stack column */}
                            {sideStack.map((c, idx) => (
                                <div key={idx} className={`mini-chip shape-${c.level}`}></div>
                            ))}
                        </div>
                    </>
                ) : (
                    <>
                        <div className="t-base-area">
                            <div className="empty-slot">
                                <div className="cross-circle">x</div>
                            </div>
                        </div>
                        <div className="t-stack-area"></div>
                    </>
                )}
            </div>
        )
    }

    const renderBankedItem = (item, key) => {
        return (
            <div key={key} className="treasure-grid-layout">
                {/* Area: base (2/3 width) - Grid Area 'base' */}
                <div className="t-base-area">
                    <div className={`chip level-${item.level} shape-${item.level}`}
                        style={{ display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
                        <span style={{ color: 'white', fontWeight: 'bold', fontSize: '1.2rem', textShadow: '1px 1px 2px black' }}>
                            {item.value}
                        </span>
                    </div>
                </div>
                {/* Area: stack (1/3 width) - Grid Area 'stack' (empty for banked items) */}
                <div className="t-stack-area"></div>
            </div>
        )
    }

    return (
        <div className="player-panel-list">
            {players.map(player => (
                <div key={player.id} className={`player-dashboard-small ${currentId === player.id ? 'active-turn' : ''} ${player.isDead ? 'dead' : ''}`}>
                    {/* Layer 1: Name and Color */}
                    <div className={`p-info p-card-${player.id}`}>

                        <span className="name">
                            {player.id === 0 ? `P${player.id + 1} (YOU)` : `P${player.id + 1} (BOT)`}
                            {player.isDead && " 💀"}
                            {isGameOver && ` (Score: ${player.totalScore})`}
                        </span>
                    </div>

                    <hr className="separator-line-sm" />

                    {/* Layer 2: Treasures */}
                    <div className="treasures-row small">
                        <div className="treasure-section">
                            <div className="label-sm">Held ({player.roundTreasures.length})</div>
                            <div className="t-list-sm">
                                {player.roundTreasures.map((t, i) => renderHeldItem(t, i))}
                            </div>
                        </div>

                        {/* Vertical Separator */}
                        <div className="v-separator"></div>

                        <div className="treasure-section">
                            <div className="label-sm">Banked</div>
                            <div className="t-list-sm">
                                {player.bankedTreasures.map((t, i) => renderBankedItem(t, i))}
                            </div>
                        </div>
                    </div>
                </div>
            ))}
        </div>
    )
}

export default PlayerView
