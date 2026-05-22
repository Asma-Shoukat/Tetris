#ifndef COMMON_H
#define COMMON_H

#include <SFML/Graphics.hpp>

// Grid configuration
const int GRID_WIDTH = 10;
const int GRID_HEIGHT = 20;
const int TILE_SIZE = 30;

// Screen layout configuration
const int SIDEBAR_WIDTH = 220;
const int SCREEN_WIDTH = GRID_WIDTH * TILE_SIZE + SIDEBAR_WIDTH;
const int SCREEN_HEIGHT = GRID_HEIGHT * TILE_SIZE;

// Simple Point struct
struct Point {
    int x, y;
};

// Tetromino types enum
enum class PieceType {
    I = 0,
    Z,
    S,
    T,
    L,
    J,
    O,
    None
};

// Shape block coordinate values (encoded as val % 2, val / 2)
const int SHAPE_ENCODINGS[7][4] = {
    {1, 3, 5, 7}, // I
    {2, 4, 5, 7}, // Z
    {3, 5, 4, 6}, // S
    {3, 5, 4, 7}, // T
    {2, 3, 5, 7}, // L
    {3, 5, 7, 6}, // J
    {2, 3, 4, 5}  // O
};

// Sleek, vibrant modern colors for each piece type
const sf::Color SHAPE_COLORS[7] = {
    sf::Color(0, 220, 220),   // Cyan (I)
    sf::Color(240, 50, 50),   // Red (Z)
    sf::Color(50, 220, 50),   // Green (S)
    sf::Color(180, 50, 220),  // Purple/Magenta (T)
    sf::Color(240, 180, 0),   // Gold/Yellow (L)
    sf::Color(40, 100, 240),  // Vibrant Blue (J)
    sf::Color(220, 220, 220)  // Sleek Light Gray/White (O)
};

#endif // COMMON_H
