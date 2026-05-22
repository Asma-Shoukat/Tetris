#include "Game.h"
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iostream>
#include <algorithm>

Game::Game() : 
    m_window(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Tetris — Classic Arcade Reimagined", sf::Style::Titlebar | sf::Style::Close),
    m_gameState(GameState::StartScreen),
    m_nextType(PieceType::None),
    m_holdType(PieceType::None),
    m_hasHeldThisTurn(false),
    m_fallTimer(0.f),
    m_fallDelay(0.5f),
    m_upPressedLastFrame(false),
    m_spacePressedLastFrame(false),
    m_cPressedLastFrame(false),
    m_pulseTimer(0.f) 
{
    m_window.setFramerateLimit(60);

    // Initialize Random Seed
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Load Font
    if (!m_font.loadFromFile("arial.ttf")) {
        std::cerr << "Error: Could not load arial.ttf font." << std::endl;
    }

    // Initialize Sound Manager
    if (!m_soundManager.initialize()) {
        std::cerr << "Error: Could not initialize sound files." << std::endl;
    }

    // Play window opening sound
    m_soundManager.playMusic(); // Starts background music looping
}

void Game::run() {
    sf::Clock clock;

    while (m_window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        handleInput();

        if (m_gameState == GameState::Playing) {
            update(deltaTime);
        } else if (m_gameState == GameState::StartScreen) {
            updateStartScreen(deltaTime);
        }

        render();
    }
}

void Game::handleInput() {
    sf::Event event;
    bool isSoftDropping = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);

    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_soundManager.stopMusic();
            m_window.close();
        }

        // 1. Key inputs for START SCREEN
        if (m_gameState == GameState::StartScreen) {
            if (event.type == sf::Event::KeyPressed) {
                m_board.reset();
                m_holdType = PieceType::None;
                m_hasHeldThisTurn = false;
                m_gameState = GameState::Playing;
                
                // Spawn first piece and roll next
                m_activePiece.spawn(static_cast<PieceType>(std::rand() % 7));
                m_nextType = static_cast<PieceType>(std::rand() % 7);
                calculateGhostPiece();
            }
        }
        // 2. Key inputs for GAME OVER SCREEN
        else if (m_gameState == GameState::GameOver) {
            if (event.type == sf::Event::KeyPressed) {
                m_gameState = GameState::StartScreen;
            }
        }
        // 3. Key inputs for PLAYING STATE
        else if (m_gameState == GameState::Playing) {
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Left) {
                    m_activePiece.move(-1, 0);
                    if (!m_board.isValidPosition(m_activePiece)) {
                        m_activePiece.move(1, 0); // Undo
                    }
                    calculateGhostPiece();
                }
                else if (event.key.code == sf::Keyboard::Right) {
                    m_activePiece.move(1, 0);
                    if (!m_board.isValidPosition(m_activePiece)) {
                        m_activePiece.move(-1, 0); // Undo
                    }
                    calculateGhostPiece();
                }
                else if (event.key.code == sf::Keyboard::Up) {
                    m_activePiece.rotate();
                    if (!m_board.isValidPosition(m_activePiece)) {
                        // Undo rotation (rotate 3 more times to undo a 90 deg clockwise)
                        m_activePiece.rotate();
                        m_activePiece.rotate();
                        m_activePiece.rotate();
                    }
                    calculateGhostPiece();
                }
                else if (event.key.code == sf::Keyboard::Space) {
                    handleHardDrop();
                }
                else if (event.key.code == sf::Keyboard::C || event.key.code == sf::Keyboard::LShift) {
                    handleHoldPiece();
                }
                else if (event.key.code == sf::Keyboard::P || event.key.code == sf::Keyboard::Escape) {
                    m_gameState = GameState::Paused;
                    m_soundManager.pauseMusic();
                }
            }
        }
        // 4. Key inputs for PAUSED STATE
        else if (m_gameState == GameState::Paused) {
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::P || event.key.code == sf::Keyboard::Escape) {
                    m_gameState = GameState::Playing;
                    m_soundManager.playMusic();
                }
            }
        }
    }
}

void Game::update(float deltaTime) {
    // Dynamically adjust fall speed based on Level
    // Starts at 0.5s, decreases by 0.05s per level down to a minimum of 0.05s
    m_fallDelay = std::max(0.05f, 0.5f - (m_board.getLevel() - 1) * 0.05f);

    float currentDelay = m_fallDelay;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
        currentDelay = 0.05f; // Fast drop / Soft drop speed
    }

    m_fallTimer += deltaTime;

    if (m_fallTimer > currentDelay) {
        // Move the piece down
        m_activePiece.move(0, 1);

        if (!m_board.isValidPosition(m_activePiece)) {
            // Collision below: undo and lock piece
            m_activePiece.move(0, -1);
            lockActivePiece();
        }

        m_fallTimer = 0.f;
    }
}

void Game::handleHoldPiece() {
    if (m_hasHeldThisTurn) return;

    PieceType currentType = m_activePiece.getType();

    if (m_holdType == PieceType::None) {
        // First hold: store current, spawn next
        m_holdType = currentType;
        spawnNewPiece();
    } else {
        // Swap hold and current
        PieceType temp = m_holdType;
        m_holdType = currentType;
        m_activePiece.spawn(temp);
        
        // Check if the swapped piece is immediately stuck (Game Over)
        if (!m_board.isValidPosition(m_activePiece)) {
            m_gameState = GameState::GameOver;
            m_soundManager.playGameOver();
            m_board.saveHighScore();
        }
    }

    m_hasHeldThisTurn = true;
    calculateGhostPiece();
}

void Game::handleHardDrop() {
    // Drop piece downward until collision
    int cellDropCount = 0;
    while (m_board.isValidPosition(m_activePiece)) {
        m_activePiece.move(0, 1);
        cellDropCount++;
    }
    m_activePiece.move(0, -1); // Undo last step that caused collision
    cellDropCount--;

    // Hard drop score bonus: 2 points per cell dropped
    m_board.addScore(cellDropCount * 2);

    lockActivePiece();
}

void Game::calculateGhostPiece() {
    m_ghostPiece = m_activePiece;
    while (m_board.isValidPosition(m_ghostPiece)) {
        m_ghostPiece.move(0, 1);
    }
    m_ghostPiece.move(0, -1); // Undo last colliding step
}

void Game::lockActivePiece() {
    // Add active piece blocks to the grid
    m_board.lockPiece(m_activePiece);

    // Scan for and clear full rows
    int cleared = m_board.clearRows();
    if (cleared > 0) {
        // Authentic Tetris Scoring
        int pts = 0;
        switch (cleared) {
            case 1: pts = 100; break;
            case 2: pts = 300; break;
            case 3: pts = 500; break;
            case 4: pts = 800; break; // Tetris!
            default: pts = 800; break;
        }
        
        m_board.addScore(pts * m_board.getLevel());
        m_board.addLinesCleared(cleared);
        
        m_soundManager.playLineClear();
        checkLevelUp();
    }

    // Reset hold restriction
    m_hasHeldThisTurn = false;

    // Spawn new piece
    spawnNewPiece();
}

void Game::spawnNewPiece() {
    m_activePiece.spawn(m_nextType);
    m_nextType = static_cast<PieceType>(std::rand() % 7);

    // Check if new piece immediately collides (Game Over)
    if (!m_board.isValidPosition(m_activePiece)) {
        m_gameState = GameState::GameOver;
        m_soundManager.playGameOver();
        m_board.saveHighScore();
    }

    calculateGhostPiece();
}

void Game::checkLevelUp() {
    // Advance 1 level for every 10 lines cleared
    int calculatedLevel = (m_board.getLinesCleared() / 10) + 1;
    if (calculatedLevel > m_board.getLevel()) {
        m_board.setLevel(calculatedLevel);
    }
}

void Game::render() {
    m_window.clear(sf::Color(10, 10, 22)); // Dark theme outer workspace

    if (m_gameState == GameState::StartScreen) {
        drawStartScreen();
    } 
    else if (m_gameState == GameState::GameOver) {
        drawGameOverScreen();
    } 
    else {
        // Draw Main Board Playfield
        m_board.draw(m_window);

        // Draw Ghost landing outline
        drawGhostPiece();

        // Draw active dropping piece
        drawActivePiece();

        // Draw HUD Sidebar on the right
        drawSidebar();

        if (m_gameState == GameState::Paused) {
            // Draw Pause Overlay
            sf::RectangleShape pauseOverlay(sf::Vector2f(GRID_WIDTH * TILE_SIZE, GRID_HEIGHT * TILE_SIZE));
            pauseOverlay.setFillColor(sf::Color(0, 0, 0, 150));
            m_window.draw(pauseOverlay);

            sf::Text pauseText;
            pauseText.setFont(m_font);
            pauseText.setCharacterSize(30);
            pauseText.setFillColor(sf::Color::White);
            pauseText.setString("PAUSED");
            
            // Center the text
            sf::FloatRect bounds = pauseText.getLocalBounds();
            pauseText.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
            pauseText.setPosition((GRID_WIDTH * TILE_SIZE) / 2.f, (GRID_HEIGHT * TILE_SIZE) / 2.f);
            
            m_window.draw(pauseText);
        }
    }

    m_window.display();
}

void Game::drawActivePiece() {
    const Point* blocks = m_activePiece.getBlocks();
    PieceType type = m_activePiece.getType();
    if (type == PieceType::None) return;

    int colorIdx = static_cast<int>(type);

    sf::RectangleShape tile(sf::Vector2f(TILE_SIZE - 1.f, TILE_SIZE - 1.f));
    sf::RectangleShape border(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    border.setOutlineThickness(1.f);
    border.setOutlineColor(sf::Color(0, 0, 0, 200));
    border.setFillColor(sf::Color::Transparent);

    for (int i = 0; i < 4; i++) {
        if (blocks[i].y >= 0) {
            tile.setPosition(blocks[i].x * TILE_SIZE, blocks[i].y * TILE_SIZE);
            tile.setFillColor(SHAPE_COLORS[colorIdx]);

            border.setPosition(blocks[i].x * TILE_SIZE, blocks[i].y * TILE_SIZE);

            m_window.draw(tile);
            m_window.draw(border);
        }
    }
}

void Game::drawGhostPiece() {
    const Point* blocks = m_ghostPiece.getBlocks();
    PieceType type = m_activePiece.getType();
    if (type == PieceType::None) return;

    int colorIdx = static_cast<int>(type);

    sf::RectangleShape tile(sf::Vector2f(TILE_SIZE - 1.f, TILE_SIZE - 1.f));
    sf::RectangleShape border(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    
    // Ghost is a translucent filled piece with a brighter outline
    tile.setFillColor(sf::Color(SHAPE_COLORS[colorIdx].r, SHAPE_COLORS[colorIdx].g, SHAPE_COLORS[colorIdx].b, 40));
    border.setOutlineThickness(1.f);
    border.setOutlineColor(sf::Color(SHAPE_COLORS[colorIdx].r, SHAPE_COLORS[colorIdx].g, SHAPE_COLORS[colorIdx].b, 160));
    border.setFillColor(sf::Color::Transparent);

    for (int i = 0; i < 4; i++) {
        if (blocks[i].y >= 0) {
            tile.setPosition(blocks[i].x * TILE_SIZE, blocks[i].y * TILE_SIZE);
            border.setPosition(blocks[i].x * TILE_SIZE, blocks[i].y * TILE_SIZE);
            m_window.draw(tile);
            m_window.draw(border);
        }
    }
}

void Game::drawSidebar() {
    int startX = GRID_WIDTH * TILE_SIZE + 15;

    // Draw Title
    sf::Text titleText;
    titleText.setFont(m_font);
    titleText.setCharacterSize(22);
    titleText.setFillColor(sf::Color(200, 200, 255));
    titleText.setStyle(sf::Text::Bold);
    titleText.setString("TETRIS");
    titleText.setPosition(startX + 45, 10);
    m_window.draw(titleText);

    // 1. NEXT PIECE PANEL
    drawSidebarPanel("NEXT PIECE", startX, 45, 190, 110);
    if (m_nextType != PieceType::None) {
        drawMiniTetromino(m_nextType, startX + 95, 45 + 55);
    }

    // 2. HOLD PIECE PANEL
    drawSidebarPanel("HOLD SLOT", startX, 165, 190, 110);
    if (m_holdType != PieceType::None) {
        drawMiniTetromino(m_holdType, startX + 95, 165 + 55);
    }

    // 3. STATS PANEL
    drawSidebarPanel("STATISTICS", startX, 285, 190, 150);

    sf::Text statsText;
    statsText.setFont(m_font);
    statsText.setCharacterSize(14);
    statsText.setFillColor(sf::Color::White);
    
    std::ostringstream statsOss;
    statsOss << "SCORE\n" 
             << "  " << m_board.getScore() << "\n\n"
             << "HIGH SCORE\n"
             << "  " << m_board.getHighScore() << "\n\n"
             << "LEVEL        " << m_board.getLevel() << "\n"
             << "LINES        " << m_board.getLinesCleared();
    
    statsText.setString(statsOss.str());
    statsText.setPosition(startX + 15, 285 + 25);
    m_window.draw(statsText);

    // 4. CONTROLS PANEL
    drawSidebarPanel("CONTROLS", startX, 445, 190, 140);
    
    sf::Text controlsText;
    controlsText.setFont(m_font);
    controlsText.setCharacterSize(11);
    controlsText.setFillColor(sf::Color(180, 180, 200));

    std::ostringstream controlsOss;
    controlsOss << "Left / Right  : Move Piece\n"
                << "Up Arrow      : Rotate Piece\n"
                << "Down Arrow    : Soft Drop\n"
                << "Spacebar      : Hard Drop\n"
                << "C / LShift    : Hold Piece\n"
                << "P / Esc       : Pause Game";

    controlsText.setString(controlsOss.str());
    controlsText.setPosition(startX + 12, 445 + 25);
    m_window.draw(controlsText);
}

void Game::drawSidebarPanel(const std::string& title, int x, int y, int width, int height) {
    // Glassmorphic panel background
    sf::RectangleShape panelBg(sf::Vector2f(width, height));
    panelBg.setPosition(x, y);
    panelBg.setFillColor(sf::Color(25, 25, 45, 180)); // Sleek semi-transparent dark blue-purple
    panelBg.setOutlineThickness(1.f);
    panelBg.setOutlineColor(sf::Color(80, 80, 120, 120)); // Thin neon blue-grey border
    m_window.draw(panelBg);

    // Panel title
    sf::Text panelTitle;
    panelTitle.setFont(m_font);
    panelTitle.setCharacterSize(12);
    panelTitle.setFillColor(sf::Color(140, 140, 220));
    panelTitle.setStyle(sf::Text::Bold);
    panelTitle.setString(title);
    panelTitle.setPosition(x + 12, y + 6);
    m_window.draw(panelTitle);
}

void Game::drawMiniTetromino(PieceType type, int centerX, int centerY) {
    int typeIdx = static_cast<int>(type);
    int minX = 9, maxX = 0, minY = 9, maxY = 0;
    Point tempBlocks[4];
    
    // Decode coordinates
    for (int i = 0; i < 4; i++) {
        tempBlocks[i].x = SHAPE_ENCODINGS[typeIdx][i] % 2;
        tempBlocks[i].y = SHAPE_ENCODINGS[typeIdx][i] / 2;
        if (tempBlocks[i].x < minX) minX = tempBlocks[i].x;
        if (tempBlocks[i].x > maxX) maxX = tempBlocks[i].x;
        if (tempBlocks[i].y < minY) minY = tempBlocks[i].y;
        if (tempBlocks[i].y > maxY) maxY = tempBlocks[i].y;
    }

    // Mini tile size
    float miniTileSize = 18.f;
    float pieceWidth = (maxX - minX + 1) * miniTileSize;
    float pieceHeight = (maxY - minY + 1) * miniTileSize;
    
    // Compute starts to center the bounding box exactly at centerX, centerY
    float startX = centerX - pieceWidth / 2.f - minX * miniTileSize;
    float startY = centerY - pieceHeight / 2.f - minY * miniTileSize;

    sf::RectangleShape tile(sf::Vector2f(miniTileSize - 1.f, miniTileSize - 1.f));
    sf::RectangleShape border(sf::Vector2f(miniTileSize, miniTileSize));
    border.setOutlineThickness(1.f);
    border.setOutlineColor(sf::Color(0, 0, 0, 180));
    border.setFillColor(sf::Color::Transparent);

    for (int i = 0; i < 4; i++) {
        tile.setPosition(startX + tempBlocks[i].x * miniTileSize, startY + tempBlocks[i].y * miniTileSize);
        tile.setFillColor(SHAPE_COLORS[typeIdx]);
        
        border.setPosition(startX + tempBlocks[i].x * miniTileSize, startY + tempBlocks[i].y * miniTileSize);
        
        m_window.draw(tile);
        m_window.draw(border);
    }
}

void Game::initStartScreenPieces() {
    m_bgPieces.clear();
    for (int i = 0; i < 8; i++) {
        StartScreenPiece p;
        p.type = static_cast<PieceType>(std::rand() % 7);
        p.x = static_cast<float>(std::rand() % (SCREEN_WIDTH - 40) + 20);
        // Distribute starting Y positions above and across the screen
        p.y = static_cast<float>(std::rand() % SCREEN_HEIGHT - SCREEN_HEIGHT);
        p.speed = static_cast<float>(std::rand() % 50 + 40); // 40 to 90 pixels per second
        p.rotationState = std::rand() % 4;
        p.rotateTimer = 0.f;
        p.rotateInterval = static_cast<float>(std::rand() % 10 + 8) / 10.f; // 0.8s to 1.8s
        m_bgPieces.push_back(p);
    }
}

void Game::updateStartScreen(float deltaTime) {
    m_pulseTimer += deltaTime;

    if (m_bgPieces.empty()) {
        initStartScreenPieces();
    }

    for (auto& p : m_bgPieces) {
        p.y += p.speed * deltaTime;
        p.rotateTimer += deltaTime;

        if (p.rotateTimer > p.rotateInterval) {
            p.rotationState = (p.rotationState + 1) % 4;
            p.rotateTimer = 0.f;
        }

        // Reset if it goes off bottom
        if (p.y > SCREEN_HEIGHT + 60.f) {
            p.type = static_cast<PieceType>(std::rand() % 7);
            p.x = static_cast<float>(std::rand() % (SCREEN_WIDTH - 40) + 20);
            p.y = -80.f;
            p.speed = static_cast<float>(std::rand() % 50 + 40);
            p.rotationState = std::rand() % 4;
            p.rotateTimer = 0.f;
            p.rotateInterval = static_cast<float>(std::rand() % 10 + 8) / 10.f;
        }
    }
}

void Game::drawStartScreen() {
    // Dark arcade background
    m_window.clear(sf::Color(8, 8, 18));

    // 0. Draw a beautiful dark neon background grid mesh
    sf::RectangleShape gridLine;
    gridLine.setFillColor(sf::Color(45, 20, 65, 55)); // Subtle neon dark purple grid
    for (int x = 0; x < SCREEN_WIDTH; x += TILE_SIZE) {
        gridLine.setSize(sf::Vector2f(1.f, SCREEN_HEIGHT));
        gridLine.setPosition(static_cast<float>(x), 0.f);
        m_window.draw(gridLine);
    }
    for (int y = 0; y < SCREEN_HEIGHT; y += TILE_SIZE) {
        gridLine.setSize(sf::Vector2f(SCREEN_WIDTH, 1.f));
        gridLine.setPosition(0.f, static_cast<float>(y));
        m_window.draw(gridLine);
    }

    // 1. Draw animated drifting background pieces
    sf::RectangleShape tile(sf::Vector2f(TILE_SIZE - 1.f, TILE_SIZE - 1.f));
    sf::RectangleShape border(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    border.setOutlineThickness(1.f);
    border.setFillColor(sf::Color::Transparent);

    for (const auto& p : m_bgPieces) {
        int typeIdx = static_cast<int>(p.type);
        Point blocks[4];

        // Decode blocks
        for (int i = 0; i < 4; i++) {
            blocks[i].x = SHAPE_ENCODINGS[typeIdx][i] % 2;
            blocks[i].y = SHAPE_ENCODINGS[typeIdx][i] / 2;
        }

        // Apply rotation
        if (p.type != PieceType::O) {
            for (int r = 0; r < p.rotationState; r++) {
                Point pivot = blocks[1];
                for (int i = 0; i < 4; i++) {
                    int bx = blocks[i].y - pivot.y;
                    int by = blocks[i].x - pivot.x;
                    blocks[i].x = pivot.x - bx;
                    blocks[i].y = pivot.y + by;
                }
            }
        }

        // Bounding box dynamic centering
        int minX = 9, maxX = 0, minY = 9, maxY = 0;
        for (int i = 0; i < 4; i++) {
            if (blocks[i].x < minX) minX = blocks[i].x;
            if (blocks[i].x > maxX) maxX = blocks[i].x;
            if (blocks[i].y < minY) minY = blocks[i].y;
            if (blocks[i].y > maxY) maxY = blocks[i].y;
        }
        float pieceWidth = (maxX - minX + 1) * TILE_SIZE;
        float pieceHeight = (maxY - minY + 1) * TILE_SIZE;
        float startX = p.x - pieceWidth / 2.f - minX * TILE_SIZE;
        float startY = p.y - pieceHeight / 2.f - minY * TILE_SIZE;

        // Draw soft translucent glowing piece blocks
        sf::Color shapeCol = SHAPE_COLORS[typeIdx];
        tile.setFillColor(sf::Color(shapeCol.r, shapeCol.g, shapeCol.b, 25)); // extremely soft overlay
        border.setOutlineColor(sf::Color(shapeCol.r, shapeCol.g, shapeCol.b, 40));

        for (int i = 0; i < 4; i++) {
            tile.setPosition(startX + blocks[i].x * TILE_SIZE, startY + blocks[i].y * TILE_SIZE);
            border.setPosition(startX + blocks[i].x * TILE_SIZE, startY + blocks[i].y * TILE_SIZE);
            m_window.draw(tile);
            m_window.draw(border);
        }
    }

    // 2. Draw Text Graphics
    
    // Dynamic neon color cycling for the main title
    float colorTime = m_pulseTimer * 1.5f;
    sf::Color neonColors[] = {
        sf::Color(0, 255, 255),   // Cyan
        sf::Color(255, 0, 255),   // Magenta
        sf::Color(255, 255, 0),   // Yellow
        sf::Color(50, 120, 255),  // Blue
        sf::Color(50, 255, 50)    // Green
    };
    int numColors = 5;
    int idx1 = static_cast<int>(colorTime) % numColors;
    int idx2 = (idx1 + 1) % numColors;
    float frac = colorTime - static_cast<int>(colorTime);
    
    sf::Color titleColor(
        static_cast<sf::Uint8>(neonColors[idx1].r + frac * (neonColors[idx2].r - neonColors[idx1].r)),
        static_cast<sf::Uint8>(neonColors[idx1].g + frac * (neonColors[idx2].g - neonColors[idx1].g)),
        static_cast<sf::Uint8>(neonColors[idx1].b + frac * (neonColors[idx2].b - neonColors[idx1].b))
    );

    // Main title shadow
    sf::Text shadowTitle;
    shadowTitle.setFont(m_font);
    shadowTitle.setCharacterSize(44);
    shadowTitle.setFillColor(sf::Color(0, 0, 0, 180)); // Dark shadow
    shadowTitle.setStyle(sf::Text::Bold);
    shadowTitle.setString("TETRIS");
    
    sf::FloatRect shadowBounds = shadowTitle.getLocalBounds();
    shadowTitle.setOrigin(shadowBounds.left + shadowBounds.width / 2.0f, shadowBounds.top + shadowBounds.height / 2.0f);
    shadowTitle.setPosition(SCREEN_WIDTH / 2.f + 3.f, SCREEN_HEIGHT / 3.f + 3.f);
    m_window.draw(shadowTitle);

    // Main title (foreground neon cycling)
    sf::Text mainTitle;
    mainTitle.setFont(m_font);
    mainTitle.setCharacterSize(42);
    mainTitle.setFillColor(titleColor);
    mainTitle.setStyle(sf::Text::Bold);
    mainTitle.setString("TETRIS");
    
    sf::FloatRect titleBounds = mainTitle.getLocalBounds();
    mainTitle.setOrigin(titleBounds.left + titleBounds.width / 2.0f, titleBounds.top + titleBounds.height / 2.0f);
    mainTitle.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 3.f);
    m_window.draw(mainTitle);
    
    // Subtitle
    sf::Text subtitle;
    subtitle.setFont(m_font);
    subtitle.setCharacterSize(16);
    subtitle.setFillColor(sf::Color(160, 160, 210));
    subtitle.setString("Classic Arcade Reimagined in C++");
    
    sf::FloatRect subBounds = subtitle.getLocalBounds();
    subtitle.setOrigin(subBounds.left + subBounds.width / 2.0f, subBounds.top + subBounds.height / 2.0f);
    subtitle.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 3.f + 50.f);
    m_window.draw(subtitle);

    // Dynamic Pulsing play prompt
    sf::Text prompt;
    prompt.setFont(m_font);
    prompt.setCharacterSize(14);
    
    float alpha = 127.f + 127.f * std::sin(m_pulseTimer * 4.f);
    prompt.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));
    prompt.setString("PRESS ANY KEY TO START");
    
    sf::FloatRect promptBounds = prompt.getLocalBounds();
    prompt.setOrigin(promptBounds.left + promptBounds.width / 2.0f, promptBounds.top + promptBounds.height / 2.0f);
    prompt.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT * 2.f / 3.f);
    m_window.draw(prompt);
}

void Game::drawGameOverScreen() {
    // Low opacity red over the board
    sf::RectangleShape deathOverlay(sf::Vector2f(SCREEN_WIDTH, SCREEN_HEIGHT));
    deathOverlay.setFillColor(sf::Color(30, 10, 10, 220));
    m_window.draw(deathOverlay);

    sf::Text goText;
    goText.setFont(m_font);
    goText.setCharacterSize(40);
    goText.setFillColor(sf::Color(255, 60, 60));
    goText.setStyle(sf::Text::Bold);
    goText.setString("GAME OVER");
    
    sf::FloatRect goBounds = goText.getLocalBounds();
    goText.setOrigin(goBounds.left + goBounds.width / 2.0f, goBounds.top + goBounds.height / 2.0f);
    goText.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 3.f);

    sf::Text statsText;
    statsText.setFont(m_font);
    statsText.setCharacterSize(18);
    statsText.setFillColor(sf::Color::White);
    
    std::ostringstream ss;
    ss << "Final Score: " << m_board.getScore() << "\n"
       << "Level Reached: " << m_board.getLevel() << "\n"
       << "Lines Cleared: " << m_board.getLinesCleared();
    
    statsText.setString(ss.str());
    sf::FloatRect statsBounds = statsText.getLocalBounds();
    statsText.setOrigin(statsBounds.left + statsBounds.width / 2.0f, statsBounds.top + statsBounds.height / 2.0f);
    statsText.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f);

    sf::Text prompt;
    prompt.setFont(m_font);
    prompt.setCharacterSize(14);
    prompt.setFillColor(sf::Color(150, 150, 200));
    prompt.setString("PRESS ANY KEY TO RETRY");
    
    sf::FloatRect promptBounds = prompt.getLocalBounds();
    prompt.setOrigin(promptBounds.left + promptBounds.width / 2.0f, promptBounds.top + promptBounds.height / 2.0f);
    prompt.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT * 2.f / 3.f);

    m_window.draw(goText);
    m_window.draw(statsText);
    m_window.draw(prompt);
}
