#ifndef BOARD_H
#define BOARD_H

#include "Common.h"
#include "Tetromino.h"
#include <vector>

class Board {
public:
    Board();

    // Clears the board grid, resets score, lines, and level
    void reset();

    // Checks if the piece is in a valid position (no collision, within boundaries)
    bool isValidPosition(const Tetromino& piece) const;

    // Locks the tetromino into the board grid
    void lockPiece(const Tetromino& piece);

    // Checks for full rows, clears them, shifts grid down, and returns number of rows cleared
    int clearRows();

    // Renders the board grid, its locked blocks, and layout borders
    void draw(sf::RenderWindow& window) const;

    // High score persistence
    void loadHighScore();
    void saveHighScore();

    // Getters and setters
    int getScore() const;
    void addScore(int points);
    int getLevel() const;
    void setLevel(int lvl);
    int getLinesCleared() const;
    void addLinesCleared(int lines);
    int getHighScore() const;
    
    // Checks if a block is occupied at the given coordinate
    int getCell(int x, int y) const;

private:
    int m_grid[GRID_HEIGHT][GRID_WIDTH];
    int m_score;
    int m_level;
    int m_linesCleared;
    int m_highScore;
};

#endif // BOARD_H
