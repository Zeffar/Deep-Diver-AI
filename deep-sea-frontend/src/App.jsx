import { useState } from 'react'
import Setup from './components/Setup'
import Game from './components/Game'
import './App.css'

// Application Entry Point
// Manages high-level state (MainMenu vs Game)
function App() {
  const [gameStarted, setGameStarted] = useState(false)
  const [playerConfigs, setPlayerConfigs] = useState(['Human', 'HeuristicBot', 'HeuristicBot'])

  const handleStart = (configs) => {
    setPlayerConfigs(configs)
    setGameStarted(true)
  }

  return (
    <div className="app-container">
      {!gameStarted ? (
        <Setup onStart={handleStart} />
      ) : (
        <Game playerConfigs={playerConfigs} onExit={() => setGameStarted(false)} />
      )}
    </div>
  )
}

export default App
