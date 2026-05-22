#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <SFML/Audio.hpp>

class SoundManager {
public:
    SoundManager();

    // Loads sound buffers and configures the background music stream
    bool initialize();

    // Control background music
    void playMusic();
    void pauseMusic();
    void stopMusic();
    void setMusicVolume(float volume);

    // Play sound effects
    void playLineClear();
    void playGameOver();

private:
    sf::SoundBuffer m_lineClearBuffer;
    sf::SoundBuffer m_gameOverBuffer;

    sf::Sound m_lineClearSound;
    sf::Sound m_gameOverSound;

    sf::Music m_bgMusic;
    bool m_initialized;
};

#endif // SOUND_MANAGER_H
