#ifndef TETROMINO_H
#define TETROMINO_H

#include "Common.h"

class Tetromino {
public:
    Tetromino();
    
    // Spawns a new piece of the given type centered at the top of the grid
    void spawn(PieceType type);

    // Moves the piece in x and y directions
    void move(int dx, int dy);

    // Rotates the piece 90 degrees clockwise around its pivot (block index 1)
    void rotate();

    // Gets the current blocks of the tetromino
    const Point* getBlocks() const;

    // Sets the blocks (useful for temporary/temp operations)
    void setBlocks(const Point newBlocks[4]);

    // Gets the piece type
    PieceType getType() const;

private:
    PieceType m_type;
    Point m_blocks[4];
};

#endif // TETROMINO_H
