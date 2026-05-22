#ifndef GAME_H
#define GAME_H

#include "Common.h"
#include "Tetromino.h"
#include "Board.h"
#include "SoundManager.h"
#include <SFML/Graphics.hpp>

enum class GameState {
    StartScreen,
    Playing,
    Paused,
    GameOver
};

class Game {
public:
    Game();

    // Starts the game loop
    void run();

private:
    // Game loop phases
    void handleInput();
    void update(float deltaTime);
    void render();

    // Gameplay helper methods
    void spawnNewPiece();
    void handleHoldPiece();
    void handleHardDrop();
    void calculateGhostPiece();
    void lockActivePiece();
    void checkLevelUp();

    // UI drawing helper methods
    void drawSidebar();
    void drawSidebarPanel(const std::string& title, int x, int y, int width, int height);
    void drawMiniTetromino(PieceType type, int centerX, int centerY);
    void drawGhostPiece();
    void drawActivePiece();
    void drawStartScreen();
    void drawGameOverScreen();

    // Start Screen Background Animation Structure
    struct StartScreenPiece {
        PieceType type;
        float x;
        float y;
        float speed;
        int rotationState; // 0-3
        float rotateTimer;
        float rotateInterval;
    };

    void initStartScreenPieces();
    void updateStartScreen(float deltaTime);

private:
    sf::RenderWindow m_window;
    GameState m_gameState;

    Board m_board;
    Tetromino m_activePiece;
    Tetromino m_ghostPiece;
    SoundManager m_soundManager;

    PieceType m_nextType;
    PieceType m_holdType;
    bool m_hasHeldThisTurn;

    float m_fallTimer;
    float m_fallDelay;

    sf::Font m_font;
    sf::Text m_uiText;

    // Control keyboard repeat safety
    bool m_upPressedLastFrame;
    bool m_spacePressedLastFrame;
    bool m_cPressedLastFrame;

    // Animation state
    std::vector<StartScreenPiece> m_bgPieces;
    float m_pulseTimer;
};

#endif // GAME_H
