# Deep Sea Adventure AI 

A digital implementation of the board game *Deep Sea Adventure*, built as a software engineering project. The game supports 2–6 players and lets you pit human players against a range of AI bots, from simple heuristics to full Monte Carlo Tree Search, all running inside your browser.

---

## What is Deep Sea Adventure?

Players are divers who take turns moving along a shared track of 32 face-down tiles, collecting treasure and trying to make it back to the submarine before the group's oxygen runs out. The catch: every piece of treasure you're carrying reduces your movement, and the oxygen bar drains faster the more treasure is in play. Greed kills — stay too deep and you drown, losing everything you grabbed that round. The game lasts up to three rounds, and the player with the most points at the end wins.

---

## Architecture

The project is split into two layers:

**C++ Engine** — all game logic, state management, and AI algorithms live here. The engine can run as a standalone CLI application or be compiled to WebAssembly for use in the browser.

**React Frontend** — a Vite-powered web UI that talks to the C++ engine via WebAssembly. A JavaScript adapter (`CppAdapter.js`) keeps the UI state in sync with the C++ game state after every move.

```
/
├── cpp/
│   ├── include/        # Headers
│   ├── src/            # Core engine + bot implementations
│   ├── apps/           # CLI game, tests, benchmarks
│   └── wasm/           # Emscripten bindings
├── deep-sea-frontend/
│   ├── src/
│   │   ├── components/ # React UI components
│   │   └── backend/    # JS/Wasm adapter layer
│   └── public/         # Static assets + compiled Wasm
├── docs/               # Presentation and media
├── figures/            # Analysis charts and UML
├── artifacts/wasm/     # Compiled Wasm outputs
└── scripts/            # Windows build helpers
```

---

## AI Bots

Four bot types are available, selectable per player slot:

| Bot | Description |
|---|---|
| **HeuristicBot** | Rule-based agent — fast and cheap, uses hand-crafted logic to decide when to dive deeper, collect, or turn back |
| **PureMCTS** | Monte Carlo Tree Search using random rollouts to estimate move value |
| **MCTS** | Full UCB1-based MCTS with a proper search tree; better quality decisions at the cost of compute time |
| **ParallelMCTS** | Multi-threaded MCTS — runs worker threads in parallel and aggregates results; uses a pre-allocated node pool to avoid GC pressure |

All bots implement the same `findBestMove(state, playerIndex, movedThisTurn)` interface.

---

## Getting Started

### Play in the browser

You need **Node.js** (LTS) and either **Podman** or **Docker** to compile the C++ engine to WebAssembly.

**1. Compile the C++ engine to Wasm**

```bash
podman run --rm -v $(pwd):/src:Z -w /src docker.io/emscripten/emsdk \
  /bin/bash -c "source /emsdk/emsdk_env.sh && \
  emcc -lembind -o deep-sea-frontend/public/deep_sea_backend.js \
  cpp/wasm/wasm_bindings.cpp -s WASM=1 -s MODULARIZE=1 -s EXPORT_ES6=1 \
  -s ENVIRONMENT=web -s EXPORT_NAME=\"createDeepSeaBackend\" \
  -s ALLOW_MEMORY_GROWTH=1 -O3"
```

*(Replace `podman` with `docker` if needed. Windows users can run `scripts/build_wasm.bat` if Emscripten is installed locally.)*

**2. Copy artifacts to the frontend**

```bash
cp deep-sea-frontend/public/deep_sea_backend.js deep-sea-frontend/src/backend/deep_sea_backend.js
cp deep-sea-frontend/public/deep_sea_backend.wasm deep-sea-frontend/src/backend/deep_sea_backend.wasm
```

**3. Install dependencies and start the dev server**

```bash
cd deep-sea-frontend
npm install
npm run dev
```

Open `http://localhost:5173` in your browser, configure your players, and hit **Start Dive**.

---

### Run the CLI

If you just want to play or test the engine locally without the browser:

```bash
# Requires g++ with C++17 and pthreads
make
./bin/deep_sea_cli
```

Other Makefile targets:

```bash
make run    # Run the test suite (requires libgtest)
make bench  # Run the bot benchmark
make timing # Run the MCTS timing benchmark
```

---

## Running Tests

Unit tests use **Google Test**:

```bash
make run
```

Frontend tests use **Vitest**:

```bash
cd deep-sea-frontend
npm test
```

---

## License

See [LICENSE](LICENSE).
