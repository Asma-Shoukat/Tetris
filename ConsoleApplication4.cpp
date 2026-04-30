#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <ctime>
#include <string>
#include <sstream>
#include <vector>
using namespace std;
using namespace sf;

const int gridWidth = 10;
const int gridHeight = 20;
const int tileSize = 30;
const int screenWidth = gridWidth * tileSize;
const int screenHeight = gridHeight * tileSize;
int grid[gridHeight][gridWidth] = { 0 };

struct Point {
    int x, y;
};

Point tetromino[4], temp[4];

// Tetromino shapes
int shapes[7][4] = {
    1, 3, 5, 7,  // I
    2, 4, 5, 7,  // Z
    3, 5, 4, 6,  // S
    3, 5, 4, 7,  // T
    2, 3, 5, 7,  // L
    3, 5, 7, 6,  // J
    2, 3, 4, 5   // O
};

// Color for each shape
Color shapeColors[7] = {
    Color::Cyan,    // I
    Color::Red,     // Z
    Color::Green,   // S
    Color::Magenta, // T
    Color::Yellow,  // L
    Color::Blue,    // J
    Color::White    // O
};

int score = 0;
bool rowCleared = false;
bool gameOver = false;

bool checkCollision() {
    for (int i = 0; i < 4; i++) {
        if (tetromino[i].x < 0 || tetromino[i].x >= gridWidth || tetromino[i].y >= gridHeight)
            return false;
        if (tetromino[i].y >= 0 && grid[tetromino[i].y][tetromino[i].x])
            return false;
    }
    return true;
}

void clearRows(Sound& lineClearSound) {
    rowCleared = false;
    for (int i = gridHeight - 1; i >= 0; i--) {
        bool fullRow = true;
        for (int j = 0; j < gridWidth; j++) {
            if (!grid[i][j]) {
                fullRow = false;
                break;
            }
        }

        if (fullRow) {
            rowCleared = true;
            score += 10;

            lineClearSound.play();

            for (int k = i; k > 0; k--) {
                for (int j = 0; j < gridWidth; j++) {
                    grid[k][j] = grid[k - 1][j];
                }
            }

            for (int j = 0; j < gridWidth; j++) {
                grid[0][j] = 0;
            }

            i++; // Recheck this row since it has new data
        }
    }
}

int main() {
    RenderWindow window(VideoMode(screenWidth, screenHeight), "Tetris Game");
    window.setFramerateLimit(60);

    // Load sound for game window opening
    SoundBuffer bbBuffer;
    if (!bbBuffer.loadFromFile("bb.mp3")) {
        return -1;
    }
    Sound windowOpenSound(bbBuffer);

    // Play the window opening sound
    windowOpenSound.play();

    Font font;
    if (!font.loadFromFile("arial.ttf")) {
        return -1;
    }

    Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(24);
    scoreText.setFillColor(Color::White);
    scoreText.setPosition(10, 10);

    Text gameOverText;
    gameOverText.setFont(font);
    gameOverText.setCharacterSize(36);
    gameOverText.setFillColor(Color::White);
    gameOverText.setString("Game Over!");
    gameOverText.setPosition(50, screenHeight / 2 - 30);

    srand(static_cast<unsigned>(time(0)));

    int currentShape = rand() % 7;
    for (int i = 0; i < 4; i++) {
        tetromino[i].x = shapes[currentShape][i] % 2 + gridWidth / 2 - 1;
        tetromino[i].y = shapes[currentShape][i] / 2;
    }

    float timer = 0, delay = 0.5f;
    Clock clock;

    // Load sounds
    SoundBuffer lineClearBuffer, gameOverBuffer;
    if (!lineClearBuffer.loadFromFile("li.mp3") || !gameOverBuffer.loadFromFile("go.mp3")) {
        return -1;
    }

    Sound lineClearSound(lineClearBuffer);
    Sound gameOverSound(gameOverBuffer);

    while (window.isOpen()) {
        float time = clock.restart().asSeconds();
        timer += time;

        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();

            if (!gameOver && event.type == Event::KeyPressed) {
                if (event.key.code == Keyboard::Left || event.key.code == Keyboard::Right) {
                    int dx = (event.key.code == Keyboard::Left) ? -1 : 1;

                    for (int i = 0; i < 4; i++) {
                        temp[i] = tetromino[i];
                        tetromino[i].x += dx;
                    }

                    if (!checkCollision()) {
                        for (int i = 0; i < 4; i++)
                            tetromino[i] = temp[i];
                    }
                }
                else if (event.key.code == Keyboard::Up) {
                    Point pivot = tetromino[1];
                    for (int i = 0; i < 4; i++) {
                        int x = tetromino[i].y - pivot.y;
                        int y = tetromino[i].x - pivot.x;
                        tetromino[i].x = pivot.x - x;
                        tetromino[i].y = pivot.y + y;
                    }

                    if (!checkCollision()) {
                        for (int i = 0; i < 4; i++)
                            tetromino[i] = temp[i];
                    }
                }
                else if (event.key.code == Keyboard::Down) {
                    delay = 0.1f;
                }
            }

            if (event.type == Event::KeyReleased) {
                if (event.key.code == Keyboard::Down) {
                    delay = 0.5f;
                }
            }
        }

        if (!gameOver) {
            if (timer > delay) {
                for (int i = 0; i < 4; i++) {
                    temp[i] = tetromino[i];
                    tetromino[i].y += 1;
                }

                if (!checkCollision()) {
                    for (int i = 0; i < 4; i++)
                        grid[temp[i].y][temp[i].x] = currentShape + 1;

                    currentShape = rand() % 7;
                    for (int i = 0; i < 4; i++) {
                        tetromino[i].x = shapes[currentShape][i] % 2 + gridWidth / 2 - 1;
                        tetromino[i].y = shapes[currentShape][i] / 2;
                    }

                    if (!checkCollision()) {
                        gameOver = true;
                        gameOverSound.play();
                    }
                }

                timer = 0;
            }

            clearRows(lineClearSound);
        }

        ostringstream scoreStream;
        scoreStream << "Score: " << score;
        scoreText.setString(scoreStream.str());

        // Draw gradient background
        RectangleShape background(Vector2f(screenWidth, screenHeight));
        background.setFillColor(Color(50, 50, 150)); // Soft gradient-like color
        window.draw(background);

        RectangleShape tile(Vector2f(tileSize - 1, tileSize - 1));
        RectangleShape border(Vector2f(tileSize, tileSize)); // Tile border

        for (int i = 0; i < gridHeight; i++) {
            for (int j = 0; j < gridWidth; j++) {
                if (grid[i][j] == 0) continue;
                tile.setPosition(j * tileSize, i * tileSize);
                tile.setFillColor(shapeColors[grid[i][j] - 1]);

                border.setPosition(j * tileSize, i * tileSize);
                border.setOutlineThickness(1);
                border.setOutlineColor(Color::Black);
                border.setFillColor(Color::Transparent);

                window.draw(tile);
                window.draw(border);
            }
        }

        for (int i = 0; i < 4; i++) {
            tile.setPosition(tetromino[i].x * tileSize, tetromino[i].y * tileSize);
            tile.setFillColor(shapeColors[currentShape]);

            border.setPosition(tetromino[i].x * tileSize, tetromino[i].y * tileSize);

            window.draw(tile);
            window.draw(border);
        }

        window.draw(scoreText);

        if (gameOver) {
            window.draw(gameOverText);
        }

        window.display();
    }

    return 0;
}
//Made by ASMA SHOUKAT