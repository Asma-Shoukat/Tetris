#include "Tetromino.h"
#include <cstdlib>

Tetromino::Tetromino() : m_type(PieceType::None) {
    for (int i = 0; i < 4; i++) {
        m_blocks[i] = {0, 0};
    }
}

void Tetromino::spawn(PieceType type) {
    m_type = type;
    int typeIdx = static_cast<int>(type);
    if (typeIdx < 0 || typeIdx >= 7) return;

    for (int i = 0; i < 4; i++) {
        m_blocks[i].x = SHAPE_ENCODINGS[typeIdx][i] % 2 + GRID_WIDTH / 2 - 1;
        m_blocks[i].y = SHAPE_ENCODINGS[typeIdx][i] / 2;
    }
}

void Tetromino::move(int dx, int dy) {
    for (int i = 0; i < 4; i++) {
        m_blocks[i].x += dx;
        m_blocks[i].y += dy;
    }
}

void Tetromino::rotate() {
    // The O piece (index 6) rotation can be skipped or allowed.
    // In standard Tetris, the O piece doesn't rotate.
    // Let's allow rotation for all pieces (matching original behavior),
    // but we can skip O-piece rotation to match standard Tetris guidelines!
    if (m_type == PieceType::O) return;

    Point pivot = m_blocks[1]; // Index 1 is the pivot block
    for (int i = 0; i < 4; i++) {
        int x = m_blocks[i].y - pivot.y;
        int y = m_blocks[i].x - pivot.x;
        m_blocks[i].x = pivot.x - x;
        m_blocks[i].y = pivot.y + y;
    }
}

const Point* Tetromino::getBlocks() const {
    return m_blocks;
}

void Tetromino::setBlocks(const Point newBlocks[4]) {
    for (int i = 0; i < 4; i++) {
        m_blocks[i] = newBlocks[i];
    }
}

PieceType Tetromino::getType() const {
    return m_type;
}
