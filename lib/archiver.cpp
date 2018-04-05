#include "archiver.hpp"

void Archiver::receiveEncodedAudioConfig(const void* data, size_t len) {
    _logger.info("archive audio config");
}

void Archiver::receiveEncodedAudio(const void* data, size_t len) {
    _logger.info("archive audio");
}

void Archiver::receiveEncodedVideoConfig(const void* data, size_t len) {
    _logger.info("archive video config");
}

void Archiver::receiveEncodedVideo(const void* data, size_t len) {
    _logger.info("archive video");
}
