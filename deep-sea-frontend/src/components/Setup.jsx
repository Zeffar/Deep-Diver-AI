import { useState } from 'react'

// Main Menu / Setup Screen
// Handles player/bot configuration
function Setup({ onStart }) {
    const [count, setCount] = useState(3)
    const [botTypes, setBotTypes] = useState(['Human', 'HeuristicBot', 'HeuristicBot', 'HeuristicBot', 'HeuristicBot', 'HeuristicBot']) // max 6

    const handlePlayerCountChange = (e) => {
        const newCount = Number(e.target.value)
        setCount(newCount)
    }

    const handleBotTypeChange = (index, value) => {
        const newTypes = [...botTypes]
        newTypes[index] = value
        setBotTypes(newTypes)
    }

    const handleSubmit = (e) => {
        e.preventDefault()
        // Pass the actual configuration up
        // e.g. ['Human', 'MCTS', 'PureMCTS']
        onStart(botTypes.slice(0, count))
    }

    const playerOptions = [
        { label: 'Human', value: 'Human' },
        { label: 'Bot: Heuristic', value: 'HeuristicBot' },
        { label: 'Bot: MCTS', value: 'MCTS' },
        { label: 'Bot: Pure MCTS', value: 'PureMCTS' },
        { label: 'Bot: Parallel MCTS', value: 'ParallelMCTS' },
    ]

    return (
        <div className="setup-container">
            <h1>Deep Sea Adventure</h1>
            <form onSubmit={handleSubmit}>
                <div className="form-group">
                    <label htmlFor="player-count">Number of Players:</label>
                    <select
                        id="player-count"
                        value={count}
                        onChange={handlePlayerCountChange}
                    >
                        {[2, 3, 4, 5, 6].map(n => (
                            <option key={n} value={n}>{n} Players</option>
                        ))}
                    </select>
                </div>

                <div className="bot-configs">
                    {Array.from({ length: count }).map((_, i) => (
                        <div key={i} className="form-group">
                            <label>Player {i + 1}:</label>
                            <select
                                value={botTypes[i]}
                                onChange={(e) => handleBotTypeChange(i, e.target.value)}
                            >
                                {playerOptions.map(opt => (
                                    <option key={opt.value} value={opt.value}>{opt.label}</option>
                                ))}
                            </select>
                        </div>
                    ))}
                </div>

                <button type="submit" className="start-btn">Start Dive</button>
            </form>
        </div>
    )
}

export default Setup
