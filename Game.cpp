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
            if (event.type == sf::Event::KeyPressed && 
                (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Return)) {
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
    sf::Color pieceColor = SHAPE_COLORS[colorIdx];

    // --- NEW: Draw Column Laser Guidelines (matching reference screenshot) ---
    bool columnDrawn[GRID_WIDTH] = { false };
    float pulse = (1.f + std::sin(m_pulseTimer * 5.f)) / 2.f;
    for (int i = 0; i < 4; i++) {
        int col = blocks[i].x;
        if (col >= 0 && col < GRID_WIDTH && !columnDrawn[col]) {
            columnDrawn[col] = true;
            
            // Faint full-column light highlight
            sf::RectangleShape stripe(sf::Vector2f(TILE_SIZE, GRID_HEIGHT * TILE_SIZE));
            stripe.setPosition(col * TILE_SIZE, 0.f);
            stripe.setFillColor(sf::Color(pieceColor.r, pieceColor.g, pieceColor.b, static_cast<sf::Uint8>(6 + pulse * 6)));
            m_window.draw(stripe);

            // Dotted glowing laser beam down the center of the column
            for (int y = 0; y < GRID_HEIGHT * TILE_SIZE; y += 12) {
                sf::RectangleShape dot(sf::Vector2f(2.f, 6.f));
                dot.setPosition(col * TILE_SIZE + TILE_SIZE / 2.f - 1.f, static_cast<float>(y));
                dot.setFillColor(sf::Color(pieceColor.r, pieceColor.g, pieceColor.b, static_cast<sf::Uint8>(50 + pulse * 50)));
                m_window.draw(dot);
            }
        }
    }

    // Draw active piece tiles with bright outline matching pieceColor
    sf::RectangleShape tile(sf::Vector2f(TILE_SIZE - 2.f, TILE_SIZE - 2.f));
    sf::RectangleShape border(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    border.setOutlineThickness(1.5f);
    border.setOutlineColor(sf::Color(pieceColor.r, pieceColor.g, pieceColor.b, 255)); // Vivid neon border
    border.setFillColor(sf::Color::Transparent);

    for (int i = 0; i < 4; i++) {
        if (blocks[i].y >= 0) {
            tile.setPosition(blocks[i].x * TILE_SIZE + 1.f, blocks[i].y * TILE_SIZE + 1.f);
            tile.setFillColor(pieceColor);

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

    // Local score formatting lambda
    auto formatWithCommas = [](int value) -> std::string {
        std::string numStr = std::to_string(value);
        int insertPosition = numStr.length() - 3;
        while (insertPosition > 0) {
            numStr.insert(insertPosition, ",");
            insertPosition -= 3;
        }
        return numStr;
    };

    // Draw Title (Neon Blue-Cyan with breathing glow)
    sf::Text titleText;
    titleText.setFont(m_font);
    titleText.setCharacterSize(22);
    titleText.setFillColor(sf::Color(0, 222, 222));
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

    // Title: SCORE
    sf::Text tScore;
    tScore.setFont(m_font);
    tScore.setCharacterSize(11);
    tScore.setFillColor(sf::Color(180, 180, 200));
    tScore.setString("SCORE");
    tScore.setStyle(sf::Text::Bold);
    tScore.setPosition(startX + 20, 285 + 25);
    m_window.draw(tScore);

    // Value: SCORE
    sf::Text vScore;
    vScore.setFont(m_font);
    vScore.setCharacterSize(14);
    vScore.setFillColor(sf::Color(255, 215, 0)); // Neon Gold
    vScore.setString(formatWithCommas(m_board.getScore()));
    vScore.setStyle(sf::Text::Bold);
    vScore.setPosition(startX + 25, 285 + 42);
    m_window.draw(vScore);

    // Title: LEVEL
    sf::Text tLevel;
    tLevel.setFont(m_font);
    tLevel.setCharacterSize(11);
    tLevel.setFillColor(sf::Color(180, 180, 200));
    tLevel.setString("LEVEL");
    tLevel.setStyle(sf::Text::Bold);
    tLevel.setPosition(startX + 20, 285 + 68);
    m_window.draw(tLevel);

    // Value: LEVEL (* Level *)
    sf::Text vLevel;
    vLevel.setFont(m_font);
    vLevel.setCharacterSize(13);
    vLevel.setFillColor(sf::Color(0, 222, 222)); // Neon Cyan
    vLevel.setString("*  " + std::to_string(m_board.getLevel()) + "  *");
    vLevel.setStyle(sf::Text::Bold);
    vLevel.setPosition(startX + 25, 285 + 85);
    m_window.draw(vLevel);

    // Title: LINES
    sf::Text tLines;
    tLines.setFont(m_font);
    tLines.setCharacterSize(11);
    tLines.setFillColor(sf::Color(180, 180, 200));
    tLines.setString("LINES");
    tLines.setStyle(sf::Text::Bold);
    tLines.setPosition(startX + 20, 285 + 110);
    m_window.draw(tLines);

    // Value: LINES (= Lines =)
    sf::Text vLines;
    vLines.setFont(m_font);
    vLines.setCharacterSize(13);
    vLines.setFillColor(sf::Color(50, 255, 50)); // Neon Green
    vLines.setString("=  " + std::to_string(m_board.getLinesCleared()) + "  =");
    vLines.setStyle(sf::Text::Bold);
    vLines.setPosition(startX + 25, 285 + 125);
    m_window.draw(vLines);

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
    // Select border color based on panel type
    sf::Color borderColor(80, 80, 120, 120); // Default
    sf::Color titleColor(180, 180, 250);
    
    if (title == "NEXT PIECE") {
        borderColor = sf::Color(0, 222, 222);       // Neon Cyan
        titleColor = sf::Color(0, 222, 222);
    } else if (title == "HOLD SLOT") {
        borderColor = sf::Color(255, 50, 50);       // Neon Red
        titleColor = sf::Color(255, 50, 50);
    } else if (title == "STATISTICS") {
        borderColor = sf::Color(255, 215, 0);       // Neon Gold
        titleColor = sf::Color(255, 215, 0);
    } else if (title == "CONTROLS") {
        borderColor = sf::Color(120, 80, 255);      // Neon Purple
        titleColor = sf::Color(120, 80, 255);
    }

    // Outer glow outline (thick, transparent)
    sf::RectangleShape outerGlow(sf::Vector2f(width + 4.f, height + 4.f));
    outerGlow.setPosition(x - 2.f, y - 2.f);
    outerGlow.setFillColor(sf::Color::Transparent);
    outerGlow.setOutlineThickness(2.f);
    outerGlow.setOutlineColor(sf::Color(borderColor.r, borderColor.g, borderColor.b, 60)); // Transparent glow
    m_window.draw(outerGlow);

    // Inner panel background
    sf::RectangleShape panelBg(sf::Vector2f(width, height));
    panelBg.setPosition(x, y);
    panelBg.setFillColor(sf::Color(12, 10, 20, 230)); // Deeper premium dark slate
    panelBg.setOutlineThickness(1.5f);
    panelBg.setOutlineColor(sf::Color(borderColor.r, borderColor.g, borderColor.b, 200)); // Vivid sharp border
    m_window.draw(panelBg);

    // Inner thin bezel border
    sf::RectangleShape innerBezel(sf::Vector2f(width - 8.f, height - 8.f));
    innerBezel.setPosition(x + 4.f, y + 4.f);
    innerBezel.setFillColor(sf::Color::Transparent);
    innerBezel.setOutlineThickness(0.5f);
    innerBezel.setOutlineColor(sf::Color(borderColor.r, borderColor.g, borderColor.b, 50));
    m_window.draw(innerBezel);

    // Panel title with a subtle shadow
    sf::Text shadowTitle;
    shadowTitle.setFont(m_font);
    shadowTitle.setCharacterSize(12);
    shadowTitle.setFillColor(sf::Color(0, 0, 0, 200));
    shadowTitle.setStyle(sf::Text::Bold);
    shadowTitle.setString(title);
    shadowTitle.setPosition(x + 13.f, y + 7.f);
    m_window.draw(shadowTitle);

    sf::Text panelTitle;
    panelTitle.setFont(m_font);
    panelTitle.setCharacterSize(12);
    panelTitle.setFillColor(titleColor);
    panelTitle.setStyle(sf::Text::Bold);
    panelTitle.setString(title);
    panelTitle.setPosition(x + 12.f, y + 6.f);
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

    // 3. Draw Pulsing Container Box around Play Prompt (matching the first reference screenshot)
    sf::Text prompt;
    prompt.setFont(m_font);
    prompt.setCharacterSize(13);
    float alpha = 127.f + 127.f * std::sin(m_pulseTimer * 4.f);
    prompt.setFillColor(sf::Color(0, 255, 255, static_cast<sf::Uint8>(alpha))); // Neon cyan text
    prompt.setString("PRESS ENTER TO START");
    prompt.setStyle(sf::Text::Bold);
    
    sf::FloatRect promptBounds = prompt.getLocalBounds();
    float boxW = promptBounds.width + 40.f;
    float boxH = promptBounds.height + 22.f;
    
    // Outer glowing border
    sf::RectangleShape promptBoxGlow(sf::Vector2f(boxW + 4.f, boxH + 4.f));
    promptBoxGlow.setFillColor(sf::Color::Transparent);
    promptBoxGlow.setOutlineThickness(2.f);
    promptBoxGlow.setOutlineColor(sf::Color(0, 222, 222, static_cast<sf::Uint8>(alpha / 3)));
    promptBoxGlow.setOrigin((boxW + 4.f) / 2.f, (boxH + 4.f) / 2.f);
    promptBoxGlow.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT * 2.f / 3.f);
    m_window.draw(promptBoxGlow);

    // Inner filled box
    sf::RectangleShape promptBox(sf::Vector2f(boxW, boxH));
    promptBox.setFillColor(sf::Color(10, 8, 20, 220));
    promptBox.setOutlineThickness(1.5f);
    promptBox.setOutlineColor(sf::Color(0, 222, 222, static_cast<sf::Uint8>(alpha)));
    promptBox.setOrigin(boxW / 2.f, boxH / 2.f);
    promptBox.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT * 2.f / 3.f);
    m_window.draw(promptBox);

    prompt.setOrigin(promptBounds.left + promptBounds.width / 2.0f, promptBounds.top + promptBounds.height / 2.0f);
    prompt.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT * 2.f / 3.f);
    m_window.draw(prompt);

    // 4. Draw Retro Arcade Bezel Frame Overlay (matching reference screenshot)
    // Outer bezel boundary (Neon Purple outline)
    sf::RectangleShape outerBezel(sf::Vector2f(SCREEN_WIDTH - 16.f, SCREEN_HEIGHT - 16.f));
    outerBezel.setPosition(8.f, 8.f);
    outerBezel.setFillColor(sf::Color::Transparent);
    outerBezel.setOutlineThickness(2.f);
    outerBezel.setOutlineColor(sf::Color(120, 30, 200, 160)); // Vibrant Neon Purple
    m_window.draw(outerBezel);

    // Inner bezel boundary (Neon Cyan outline)
    sf::RectangleShape innerBezel(sf::Vector2f(SCREEN_WIDTH - 28.f, SCREEN_HEIGHT - 28.f));
    innerBezel.setPosition(14.f, 14.f);
    innerBezel.setFillColor(sf::Color::Transparent);
    innerBezel.setOutlineThickness(1.f);
    innerBezel.setOutlineColor(sf::Color(0, 222, 222, 100)); // Glowing cyan
    m_window.draw(innerBezel);

    // Bezel Text Styling
    sf::Text bezelText;
    bezelText.setFont(m_font);
    bezelText.setCharacterSize(9);
    bezelText.setFillColor(sf::Color(140, 100, 180, 220)); // Soft purple bezel text

    // Top Center: NEW WAVE GAMING
    bezelText.setString("NEW WAVE GAMING");
    bezelText.setStyle(sf::Text::Bold);
    sf::FloatRect bBounds = bezelText.getLocalBounds();
    bezelText.setOrigin(bBounds.left + bBounds.width / 2.f, bBounds.top + bBounds.height / 2.f);
    bezelText.setPosition(SCREEN_WIDTH / 2.f, 21.f);
    m_window.draw(bezelText);

    // Top Left: ARCADE CLASSIC
    bezelText.setString("ARCADE CLASSIC");
    bezelText.setStyle(sf::Text::Regular);
    bBounds = bezelText.getLocalBounds();
    bezelText.setOrigin(0.f, bBounds.top + bBounds.height / 2.f);
    bezelText.setPosition(25.f, 21.f);
    m_window.draw(bezelText);

    // Top Right: CYBER-GRID EDITION
    bezelText.setString("CYBER-GRID EDITION");
    bBounds = bezelText.getLocalBounds();
    bezelText.setOrigin(bBounds.width, bBounds.top + bBounds.height / 2.f);
    bezelText.setPosition(SCREEN_WIDTH - 25.f, 21.f);
    m_window.draw(bezelText);

    // Bottom Center: CYBER-GRID EDITION
    bezelText.setString("CYBER-GRID EDITION");
    bezelText.setStyle(sf::Text::Bold);
    bBounds = bezelText.getLocalBounds();
    bezelText.setOrigin(bBounds.left + bBounds.width / 2.f, bBounds.top + bBounds.height / 2.f);
    bezelText.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT - 21.f);
    m_window.draw(bezelText);

    // Left Margin (Vertical): ARCADE CLASSIC
    bezelText.setString("ARCADE CLASSIC");
    bezelText.setRotation(-90.f);
    bBounds = bezelText.getLocalBounds();
    bezelText.setOrigin(bBounds.left + bBounds.width / 2.f, bBounds.top + bBounds.height / 2.f);
    bezelText.setPosition(21.f, SCREEN_HEIGHT / 2.f);
    m_window.draw(bezelText);

    // Right Margin (Vertical): NEW WAVE GAMING
    bezelText.setString("NEW WAVE GAMING");
    bezelText.setRotation(90.f);
    bBounds = bezelText.getLocalBounds();
    bezelText.setOrigin(bBounds.left + bBounds.width / 2.f, bBounds.top + bBounds.height / 2.f);
    bezelText.setPosition(SCREEN_WIDTH - 21.f, SCREEN_HEIGHT / 2.f);
    m_window.draw(bezelText);
}

void Game::drawGameOverScreen() {
    // 1. Sleek semi-transparent dark red death overlay
    sf::RectangleShape deathOverlay(sf::Vector2f(SCREEN_WIDTH, SCREEN_HEIGHT));
    deathOverlay.setFillColor(sf::Color(15, 2, 2, 235));
    m_window.draw(deathOverlay);

    // 2. Neon Red Cyber Grid Overlay (Faded background)
    sf::VertexArray gridLines(sf::Lines);
    sf::Color gridColor(255, 30, 30, 20); // very faint red
    // Draw vertical lines
    for (int x = 0; x <= SCREEN_WIDTH; x += 30) {
        gridLines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.f), gridColor));
        gridLines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(SCREEN_HEIGHT)), gridColor));
    }
    // Draw horizontal lines
    for (int y = 0; y <= SCREEN_HEIGHT; y += 30) {
        gridLines.append(sf::Vertex(sf::Vector2f(0.f, static_cast<float>(y)), gridColor));
        gridLines.append(sf::Vertex(sf::Vector2f(static_cast<float>(SCREEN_WIDTH), static_cast<float>(y)), gridColor));
    }
    m_window.draw(gridLines);

    // 3. GAME OVER Title with dual glow shadow (subtle scaling breathing effect)
    float scale = 1.0f + 0.03f * std::sin(m_pulseTimer * 3.f);
    
    // Outer red glow
    sf::Text glowText;
    glowText.setFont(m_font);
    glowText.setCharacterSize(42);
    glowText.setFillColor(sf::Color(255, 0, 0, 100)); // semi-trans red glow
    glowText.setStyle(sf::Text::Bold);
    glowText.setString("GAME OVER");
    glowText.setScale(scale, scale);
    sf::FloatRect glowBounds = glowText.getLocalBounds();
    glowText.setOrigin(glowBounds.left + glowBounds.width / 2.0f, glowBounds.top + glowBounds.height / 2.0f);
    glowText.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 4.f);
    m_window.draw(glowText);

    // Foreground color-pulsing title (shifts crimson-red to neon pink)
    sf::Text goText;
    goText.setFont(m_font);
    goText.setCharacterSize(40);
    float pulseVal = (1.f + std::sin(m_pulseTimer * 3.f)) / 2.f;
    sf::Color pulsingRed(
        static_cast<sf::Uint8>(255),
        static_cast<sf::Uint8>(30 + pulseVal * 50),
        static_cast<sf::Uint8>(30 + pulseVal * 100)
    );
    goText.setFillColor(pulsingRed);
    goText.setStyle(sf::Text::Bold);
    goText.setString("GAME OVER");
    goText.setScale(scale, scale);
    sf::FloatRect goBounds = goText.getLocalBounds();
    goText.setOrigin(goBounds.left + goBounds.width / 2.0f, goBounds.top + goBounds.height / 2.0f);
    goText.setPosition(SCREEN_WIDTH / 2.f - 2.f, SCREEN_HEIGHT / 4.f - 2.f);
    m_window.draw(goText);

    // 4. Stylish Stats Box (Semi-transparent dark gray panel with glowing red border)
    float boxW = 260.f;
    float boxH = 150.f;
    sf::RectangleShape statsBox(sf::Vector2f(boxW, boxH));
    statsBox.setFillColor(sf::Color(25, 8, 8, 210));
    statsBox.setOutlineColor(sf::Color(255, 50, 50, 180));
    statsBox.setOutlineThickness(2.0f);
    
    // Center the stats box
    statsBox.setOrigin(boxW / 2.f, boxH / 2.f);
    statsBox.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f);
    m_window.draw(statsBox);

    // 5. Draw Stats Inside the Box
    // Title of stats section
    sf::Text statsTitle;
    statsTitle.setFont(m_font);
    statsTitle.setCharacterSize(14);
    statsTitle.setFillColor(sf::Color(200, 100, 100));
    statsTitle.setString("--- CAMPAIGN STATS ---");
    statsTitle.setStyle(sf::Text::Bold);
    sf::FloatRect stBounds = statsTitle.getLocalBounds();
    statsTitle.setOrigin(stBounds.left + stBounds.width / 2.0f, stBounds.top + stBounds.height / 2.0f);
    statsTitle.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT / 2.f - boxH / 2.f + 20.f);
    m_window.draw(statsTitle);

    // Score Label & Value
    sf::Text labelScore;
    labelScore.setFont(m_font);
    labelScore.setCharacterSize(14);
    labelScore.setFillColor(sf::Color(180, 180, 180));
    labelScore.setString("Score:");
    labelScore.setPosition(SCREEN_WIDTH / 2.f - boxW / 2.f + 25.f, SCREEN_HEIGHT / 2.f - boxH / 2.f + 45.f);
    m_window.draw(labelScore);

    sf::Text valScore;
    valScore.setFont(m_font);
    valScore.setCharacterSize(14);
    valScore.setFillColor(sf::Color(255, 215, 0)); // Neon Gold
    valScore.setString(std::to_string(m_board.getScore()));
    valScore.setStyle(sf::Text::Bold);
    sf::FloatRect vsBounds = valScore.getLocalBounds();
    valScore.setPosition(SCREEN_WIDTH / 2.f + boxW / 2.f - vsBounds.width - 25.f, SCREEN_HEIGHT / 2.f - boxH / 2.f + 45.f);
    m_window.draw(valScore);

    // Level Label & Value
    sf::Text labelLevel;
    labelLevel.setFont(m_font);
    labelLevel.setCharacterSize(14);
    labelLevel.setFillColor(sf::Color(180, 180, 180));
    labelLevel.setString("Level:");
    labelLevel.setPosition(SCREEN_WIDTH / 2.f - boxW / 2.f + 25.f, SCREEN_HEIGHT / 2.f - boxH / 2.f + 75.f);
    m_window.draw(labelLevel);

    sf::Text valLevel;
    valLevel.setFont(m_font);
    valLevel.setCharacterSize(14);
    valLevel.setFillColor(sf::Color(0, 222, 222)); // Neon Cyan
    valLevel.setString(std::to_string(m_board.getLevel()));
    valLevel.setStyle(sf::Text::Bold);
    sf::FloatRect vlBounds = valLevel.getLocalBounds();
    valLevel.setPosition(SCREEN_WIDTH / 2.f + boxW / 2.f - vlBounds.width - 25.f, SCREEN_HEIGHT / 2.f - boxH / 2.f + 75.f);
    m_window.draw(valLevel);

    // Lines Label & Value
    sf::Text labelLines;
    labelLines.setFont(m_font);
    labelLines.setCharacterSize(14);
    labelLines.setFillColor(sf::Color(180, 180, 180));
    labelLines.setString("Lines Cleared:");
    labelLines.setPosition(SCREEN_WIDTH / 2.f - boxW / 2.f + 25.f, SCREEN_HEIGHT / 2.f - boxH / 2.f + 105.f);
    m_window.draw(labelLines);

    sf::Text valLines;
    valLines.setFont(m_font);
    valLines.setCharacterSize(14);
    valLines.setFillColor(sf::Color(50, 255, 50)); // Neon Green
    valLines.setString(std::to_string(m_board.getLinesCleared()));
    valLines.setStyle(sf::Text::Bold);
    sf::FloatRect vlnBounds = valLines.getLocalBounds();
    valLines.setPosition(SCREEN_WIDTH / 2.f + boxW / 2.f - vlnBounds.width - 25.f, SCREEN_HEIGHT / 2.f - boxH / 2.f + 105.f);
    m_window.draw(valLines);

    // 6. Dynamic Pulsing retry prompt
    sf::Text prompt;
    prompt.setFont(m_font);
    prompt.setCharacterSize(14);
    float alpha = 127.f + 127.f * std::sin(m_pulseTimer * 4.f);
    prompt.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha)));
    prompt.setString("PRESS ANY KEY TO RETRY");
    prompt.setStyle(sf::Text::Bold);
    
    sf::FloatRect promptBounds = prompt.getLocalBounds();
    prompt.setOrigin(promptBounds.left + promptBounds.width / 2.0f, promptBounds.top + promptBounds.height / 2.0f);
    prompt.setPosition(SCREEN_WIDTH / 2.f, SCREEN_HEIGHT * 2.f / 3.f + 30.f);
    m_window.draw(prompt);
}
