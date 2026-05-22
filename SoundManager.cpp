#include "SoundManager.h"
#include <iostream>

SoundManager::SoundManager() : m_initialized(false) {}

bool SoundManager::initialize() {
    // Load SFX files
    if (!m_lineClearBuffer.loadFromFile("li.mp3")) {
        std::cerr << "Failed to load line clear sound (li.mp3)" << std::endl;
        return false;
    }
    m_lineClearSound.setBuffer(m_lineClearBuffer);
    m_lineClearSound.setVolume(80.f);

    if (!m_gameOverBuffer.loadFromFile("go.mp3")) {
        std::cerr << "Failed to load game over sound (go.mp3)" << std::endl;
        return false;
    }
    m_gameOverSound.setBuffer(m_gameOverBuffer);
    m_gameOverSound.setVolume(90.f);

    // Load background music (streamed directly to save RAM)
    if (!m_bgMusic.openFromFile("bb.mp3")) {
        std::cerr << "Failed to load background music (bb.mp3)" << std::endl;
        return false;
    }

    m_bgMusic.setLoop(true);
    m_bgMusic.setVolume(40.f); // Moderate default volume

    m_initialized = true;
    return true;
}

void SoundManager::playMusic() {
    if (m_initialized && m_bgMusic.getStatus() != sf::SoundSource::Playing) {
        m_bgMusic.play();
    }
}

void SoundManager::pauseMusic() {
    if (m_initialized && m_bgMusic.getStatus() == sf::SoundSource::Playing) {
        m_bgMusic.pause();
    }
}

void SoundManager::stopMusic() {
    if (m_initialized) {
        m_bgMusic.stop();
    }
}

void SoundManager::setMusicVolume(float volume) {
    if (m_initialized) {
        m_bgMusic.setVolume(volume);
    }
}

void SoundManager::playLineClear() {
    if (m_initialized) {
        m_lineClearSound.play();
    }
}

void SoundManager::playGameOver() {
    if (m_initialized) {
        m_gameOverSound.play();
    }
}
