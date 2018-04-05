#pragma once

#include "encoded_av_receiver.hpp"
#include "logger.hpp"

class Archiver : public EncodedAVReceiver {
public:
    explicit Archiver(Logger logger) : _logger{logger} {}
    virtual ~Archiver() {}

    virtual void receiveEncodedAudioConfig(const void* data, size_t len) override;
    virtual void receiveEncodedAudio(const void* data, size_t len) override;
    virtual void receiveEncodedVideoConfig(const void* data, size_t len) override;
    virtual void receiveEncodedVideo(const void* data, size_t len) override;

private:
    const Logger _logger;
};
