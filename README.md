# 🎮 C++ SFML Tetris

A classic Tetris clone built using **C++** and the **SFML** (Simple and Fast Multimedia Library) framework. This project demonstrates core game loop concepts, matrix-based grid management, collision detection, and multimedia rendering.

## ✨ Features
*   **Classic Gameplay:** Features the 7 standard Tetromino shapes with accurate rotations and grid boundary constraints.
*   **Scoring System:** Line clears reward points and dynamically update the score display in real-time.
*   **Audio & Sound Effects:** Includes window opening audio, line-clear sound effects, and a game-over sound cue.
*   **Dynamic Visuals:** Distinct colors for each Tetromino shape, rendered with tile borders against a soft blue gradient background.
*   **Optimized Game Loop:** Fixed framerate limit (60 FPS) with delta-time logic to control piece falling speed.

## 🛠️ Tech Stack
*   **Language:** C++
*   **Graphics & Audio Library:** SFML 2.6.2
*   **IDE:** Visual Studio (Solution file `ConsoleApplication4.sln` included)

## 🎮 Controls
*   **Left Arrow:** Move shape left
*   **Right Arrow:** Move shape right
*   **Up Arrow:** Rotate shape
*   **Down Arrow:** Soft drop (speeds up falling)

## 📂 Project Structure
```text
Tetris/
├── ConsoleApplication4.cpp    # Core game logic, rendering loop, and input handling
├── ConsoleApplication4.sln    # Visual Studio Solution file
├── SFML-2.6.2/                # Local SFML library directories
├── arial.ttf                  # Font file for rendering the score and UI text
├── bb.mp3                     # Game start sound effect
├── li.mp3                     # Line-clear sound effect
└── go.mp3                     # Game-over sound effect
```

## ⚙️ How to Build and Run
This project is pre-configured as a Visual Studio Solution.

### Prerequisites
1.  **Visual Studio** (with C++ Desktop Development workload installed).
2.  **SFML 2.6.2**: Ensure that your Visual Studio Include and Library directories are pointing to the `SFML-2.6.2` folder included in the repository.

### Instructions
1.  Open the `ConsoleApplication4.sln` file in Visual Studio.
2.  Ensure your build configuration is set to **x64**.
3.  Build the solution (`Ctrl + Shift + B`).
4.  Run the application (`F5`).

> [!NOTE] 
> For the game to run properly and load assets, ensure that the `.mp3` files and the `arial.ttf` font file are located in the executable's working directory. Additionally, verify that the necessary SFML `.dll` files are accessible by the executable.

---
**Developer:** ASMA SHOUKAT
