# How to Build and Run Deep Sea Adventure (this is ai generated, ignore)

This guide outlines the steps required to set up and run the application from a clean, new environment.

## Prerequisites
- **Node.js & npm**: Required to run the React (Vite) frontend server.
- **Podman or Docker** (Linux/macOS): Required to cross-compile the C++ engine into WebAssembly (WASM) without installing a local toolchain.

## 1. Clone the repository
First, clone the repository to your local machine and navigate into it:
```bash
git clone <your-repo-url>
cd proiect-inginerie-software-team
```

## 2. Compile the C++ Engine to WebAssembly
The core game engine and bot AIs (Heuristic, MCTS, etc.) are written in C++. We use Emscripten to compile these down to WebAssembly files that the browser can execute.

Run the following command from the **root** of the repository (replace `podman` with `docker` if you are using Docker):
```bash
podman run --rm -v $(pwd):/src:Z -w /src docker.io/emscripten/emsdk /bin/bash -c "source /emsdk/emsdk_env.sh && emcc -lembind -o deep-sea-frontend/public/deep_sea_backend.js cpp/wasm/wasm_bindings.cpp -s WASM=1 -s MODULARIZE=1 -s EXPORT_ES6=1 -s ENVIRONMENT=web -s EXPORT_NAME=\"createDeepSeaBackend\" -s ALLOW_MEMORY_GROWTH=1 -O3"
```

Next, copy the newly generated WebAssembly artifacts into the frontend's Javascript bridge directory so the React app can properly import them:
```bash
cp deep-sea-frontend/public/deep_sea_backend.js deep-sea-frontend/src/backend/deep_sea_backend.js
cp deep-sea-frontend/public/deep_sea_backend.wasm deep-sea-frontend/src/backend/deep_sea_backend.wasm
```

*(Note for Windows users: You can run `scripts/build_wasm.bat` if you have the Emscripten SDK installed and configured locally).*

## 3. Install dependencies and start the Frontend
Navigate into the frontend project, install the necessary npm packages, and spin up the development server:

```bash
cd deep-sea-frontend
npm install
npm run dev
```

The application will now be running locally. Open the provided placeholder URL (e.g., `http://localhost:5173` or `http://localhost:5174`) in your browser to configure your bots and play the game!