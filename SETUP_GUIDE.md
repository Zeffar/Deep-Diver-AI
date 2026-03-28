# Deep Sea Adventure - Complete Setup Guide (AI generated, ignore please)

This project consists of two parts:
1.  **Backend Logic (C++)**: The core game mechanics implemented in C++.
2.  **Frontend (React + Vite)**: A modern web interface for avoiding console-based gameplay.

---

## 1. Prerequisites

### A. For the Frontend (Web UI)
You must have **Node.js** installed to run the web application.
*   **Download**: [https://nodejs.org/](https://nodejs.org/) (Download the **LTS** version).
*   **Verify**: Open a terminal (PowerShell or Command Prompt) and type:
    ```bash
    node -v
    npm -v
    ```
    If these output version numbers (e.g., `v18.16.0`), you are ready.

### B. For the Backend (C++ Compilation)
If you wish to compile the C++ logic (`cpp/src/environment.cpp`), you need a C++ compiler.

#### Option 1: Visual Studio (Recommended for Windows)
*   Install **Visual Studio Community** (free): [https://visualstudio.microsoft.com/downloads/](https://visualstudio.microsoft.com/downloads/)
*   Select **"Desktop development with C++"** during installation.
*   Open the project folder in Visual Studio to compile.

#### Option 2: MinGW (Command Line for Windows)
*   Install [MinGW-w64](https://www.mingw-w64.org/).
*   Ensure `g++` is added to your System PATH.
*   Verify by running `g++ --version` in terminal.

#### Option 3: WSL (Linux on Windows)
*   Install WSL via PowerShell: `wsl --install`.
*   Install compile tools: `sudo apt update && sudo apt install build-essential`.

---

## 2. Installation & Setup

### Frontend Setup
1.  Open your terminal.
2.  Navigate to the **frontend directory**:
    ```bash
    cd deep-sea-frontend
    ```
3.  Install dependencies (libraries needed for the UI):
    ```bash
    npm install
    ```
    *This might take a minute as it downloads React, Vite, and other tools.*

### Backend Setup
The C++ files are organized under `cpp/`:
*   `cpp/include`: header files
*   `cpp/src`: core engine logic
*   `cpp/apps`: CLI/tests/bench entry points
*   `cpp/wasm`: WebAssembly binding entry point
*   No standard installation is needed for C++, just compilation (see below).

---

## 3. How to Run

### Running the Web Game (Frontend)
1.  Navigate to `deep-sea-frontend`:
    ```bash
    cd deep-sea-frontend
    ```
2.  Start the development server:
    ```bash
    npm run dev
    ```
3.  The terminal will show a local URL, usually:
    > `Local: http://localhost:5173/`

4.  **Ctrl + Click** that link (or copy-paste it into your browser) to play the game!

### Compiling & Running C++ Logic (Backend)
To verify the C++ core logic separately:

**Using Make (Linux/WSL/MinGW with Make):**
```bash
# In the root project directory
make
./bin/deep_sea_cli
```

**Using g++ directly (Windows Command Prompt/Powershell):**
```bash
g++ -std=c++17 -Icpp/include cpp/apps/tests.cpp cpp/src/environment.cpp -lgtest -lgtest_main -pthread -o run_tests
.\run_tests.exe
```

---

## 4. Project Structure
*   `/` (Root): Contains build/config files and high-level docs, including the `Makefile`.
*   `/cpp`: Contains all C++ backend sources.
    *   `/include`: Header files.
    *   `/src`: Core game logic and bot implementations.
    *   `/apps`: CLI, test, and benchmark entry points.
    *   `/wasm`: Emscripten binding translation unit.
*   `/docs`: Project supporting material.
    *   `/media`: Demo videos.
    *   `/presentation`: Presentation documents.
*   `/artifacts`: Generated or exportable artifacts.
    *   `/wasm`: Root-level WebAssembly build outputs.
*   `/deep-sea-frontend`: Contains the React Web Application.
    *   `/src`: Source code for the UI.
    *   `/src/backend`: **Mock Backend** (JavaScript implementation of C++ logic for the web).
    *   `/public`: Static assets.