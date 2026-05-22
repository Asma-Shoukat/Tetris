#include "Board.h"
#include <fstream>
#include <iostream>

Board::Board() : m_score(0), m_level(1), m_linesCleared(0), m_highScore(0) {
    reset();
    loadHighScore();
}

void Board::reset() {
    m_score = 0;
    m_level = 1;
    m_linesCleared = 0;
    for (int i = 0; i < GRID_HEIGHT; i++) {
        for (int j = 0; j < GRID_WIDTH; j++) {
            m_grid[i][j] = 0;
        }
    }
}

bool Board::isValidPosition(const Tetromino& piece) const {
    const Point* blocks = piece.getBlocks();
    for (int i = 0; i < 4; i++) {
        // Boundary checking
        if (blocks[i].x < 0 || blocks[i].x >= GRID_WIDTH || blocks[i].y >= GRID_HEIGHT) {
            return false;
        }
        // Collision with locked grid blocks (only if the block is inside the visible grid y >= 0)
        if (blocks[i].y >= 0 && m_grid[blocks[i].y][blocks[i].x] != 0) {
            return false;
        }
    }
    return true;
}

void Board::lockPiece(const Tetromino& piece) {
    const Point* blocks = piece.getBlocks();
    int typeIdx = static_cast<int>(piece.getType());
    for (int i = 0; i < 4; i++) {
        if (blocks[i].y >= 0) {
            m_grid[blocks[i].y][blocks[i].x] = typeIdx + 1; // Store 1-based index
        }
    }
}

int Board::clearRows() {
    int rowsClearedThisTick = 0;
    for (int i = GRID_HEIGHT - 1; i >= 0; i--) {
        bool fullRow = true;
        for (int j = 0; j < GRID_WIDTH; j++) {
            if (m_grid[i][j] == 0) {
                fullRow = false;
                break;
            }
        }

        if (fullRow) {
            rowsClearedThisTick++;
            
            // Shift all rows above down by 1
            for (int k = i; k > 0; k--) {
                for (int j = 0; j < GRID_WIDTH; j++) {
                    m_grid[k][j] = m_grid[k - 1][j];
                }
            }

            // Zero out the top row
            for (int j = 0; j < GRID_WIDTH; j++) {
                m_grid[0][j] = 0;
            }

            i++; // Recheck this row index since it now contains the row that was above it
        }
    }
    return rowsClearedThisTick;
}

void Board::draw(sf::RenderWindow& window) const {
    // 1. Draw Sleek Board Background Playfield
    sf::RectangleShape playfieldBg(sf::Vector2f(GRID_WIDTH * TILE_SIZE, GRID_HEIGHT * TILE_SIZE));
    playfieldBg.setFillColor(sf::Color(18, 18, 32, 230)); // Sleek dark space blue
    window.draw(playfieldBg);

    // 2. Draw Subtle Grid Mesh (helps player coordinate positions)
    sf::RectangleShape gridLine;
    gridLine.setFillColor(sf::Color(255, 255, 255, 12)); // Extremely faint white

    // Vertical lines
    for (int col = 1; col < GRID_WIDTH; col++) {
        gridLine.setSize(sf::Vector2f(1.f, GRID_HEIGHT * TILE_SIZE));
        gridLine.setPosition(col * TILE_SIZE, 0.f);
        window.draw(gridLine);
    }
    // Horizontal lines
    for (int row = 1; row < GRID_HEIGHT; row++) {
        gridLine.setSize(sf::Vector2f(GRID_WIDTH * TILE_SIZE, 1.f));
        gridLine.setPosition(0.f, row * TILE_SIZE);
        window.draw(gridLine);
    }

    // 3. Draw Locked Tiles
    sf::RectangleShape tile(sf::Vector2f(TILE_SIZE - 1.f, TILE_SIZE - 1.f));
    sf::RectangleShape border(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    border.setOutlineThickness(1.f);
    border.setOutlineColor(sf::Color(0, 0, 0, 160));
    border.setFillColor(sf::Color::Transparent);

    for (int i = 0; i < GRID_HEIGHT; i++) {
        for (int j = 0; j < GRID_WIDTH; j++) {
            if (m_grid[i][j] == 0) continue;

            int colorIdx = m_grid[i][j] - 1;
            tile.setPosition(j * TILE_SIZE, i * TILE_SIZE);
            tile.setFillColor(SHAPE_COLORS[colorIdx]);

            border.setPosition(j * TILE_SIZE, i * TILE_SIZE);

            window.draw(tile);
            window.draw(border);
        }
    }

    // 4. Draw Thick outer border around the board
    sf::RectangleShape outerBorder(sf::Vector2f(GRID_WIDTH * TILE_SIZE, GRID_HEIGHT * TILE_SIZE));
    outerBorder.setFillColor(sf::Color::Transparent);
    outerBorder.setOutlineThickness(3.f);
    outerBorder.setOutlineColor(sf::Color(100, 100, 150, 150)); // Semi-transparent blue-grey border
    window.draw(outerBorder);
}

void Board::loadHighScore() {
    std::ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> m_highScore;
        file.close();
    } else {
        m_highScore = 0;
    }
}

void Board::saveHighScore() {
    if (m_score > m_highScore) {
        m_highScore = m_score;
        std::ofstream file("highscore.txt");
        if (file.is_open()) {
            file << m_highScore;
            file.close();
        }
    }
}

int Board::getScore() const { return m_score; }
void Board::addScore(int points) { m_score += points; }
int Board::getLevel() const { return m_level; }
void Board::setLevel(int lvl) { m_level = lvl; }
int Board::getLinesCleared() const { return m_linesCleared; }
void Board::addLinesCleared(int lines) { m_linesCleared += lines; }
int Board::getHighScore() const { return m_highScore; }

int Board::getCell(int x, int y) const {
    if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return 0;
    return m_grid[y][x];
}
