#include "demuxer.hpp"

extern "C" {
    #include <libavformat/avformat.h>
}

#include "ffmpeg.hpp"
#include "mpeg4.hpp"
#include "utility.hpp"

#include <h264/h264.hpp>
#include <h264/nal_unit.hpp>

Demuxer::Demuxer(Logger logger, std::string in, EncodedAVHandler* handler) {
    _thread = std::thread([=] {
        _run(logger, in, handler);
        _isDone = true;
    });
}

Demuxer::~Demuxer() {
    _thread.join();
}

void Demuxer::_run(Logger logger, std::string in, EncodedAVHandler* handler) {
    InitFFmpeg();

    AVFormatContext* formatContext = nullptr;

    auto err = avformat_open_input(&formatContext, in.c_str(), nullptr, nullptr);
    if (err < 0) {
        logger.error("unable to open input file: {}", FFmpegErrorString(err));
        return;
    }
    RegisterFFmpegLogContext(formatContext, logger);
    CLEANUP([&] {
        UnregisterFFmpegLogContext(formatContext);
        avformat_close_input(&formatContext);
    });

    err = avformat_find_stream_info(formatContext, nullptr);
    if (err < 0) {
        logger.error("unable to find stream info: {}", FFmpegErrorString(err));
        avformat_close_input(&formatContext);
        return;
    }

    AVPacket packet{0};
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;

    bool didOutputAudioConfig = false;
    bool didOutputVideoConfig = false;

    std::vector<uint8_t> outputBuffer;

    auto isAVCCIn = false;

    AVCDecoderConfigurationRecord videoConfig;

    while (av_read_frame(formatContext, &packet) >= 0) {
        auto stream = formatContext->streams[packet.stream_index];
        auto mediaType = stream->codecpar->codec_type;

        if (mediaType == AVMEDIA_TYPE_VIDEO) {
            if (!didOutputVideoConfig) {
                _totalFrameCount = stream->nb_frames;

                if (stream->codecpar->extradata) {
                    if (!videoConfig.decode(stream->codecpar->extradata, stream->codecpar->extradata_size)) {
                        logger.error("unable to decode video videoConfig");
                        return;
                    }
                    isAVCCIn = true;
                } else {
                    videoConfig.lengthSizeMinusOne = 3;

                    if (!h264::IterateAnnexB(packet.data, packet.size, [&](const void* data, size_t len) {
                        auto naluType = *reinterpret_cast<const uint8_t*>(data) & 0x0f;

                        if (naluType != h264::NALUnitType::SequenceParameterSet && naluType != h264::NALUnitType::PictureParameterSet) {
                            return;
                        }

                        h264::nal_unit nalu;
                        h264::bitstream bs{data, len};
                        auto err = nalu.decode(&bs, len);
                        if (err) {
                            logger.error("error decoding videoConfig nalu: {}", err.message);
                            return;
                        }

                        auto bytePtr = reinterpret_cast<const uint8_t*>(data);

                        if (naluType == h264::NALUnitType::SequenceParameterSet) {
                            videoConfig.avcProfileIndication = nalu.rbsp_byte[0];
                            videoConfig.profileCompatibility = nalu.rbsp_byte[1];
                            videoConfig.avcLevelIndication = nalu.rbsp_byte[2];
                            videoConfig.sequenceParameterSets.emplace_back(bytePtr, bytePtr + len);
                        } else if (naluType == h264::NALUnitType::PictureParameterSet) {
                            videoConfig.pictureParameterSets.emplace_back(bytePtr, bytePtr + len);
                        }
                    })) {
                        logger.error("unable to iterate encoded nalus");
                        return;
                    }

                    if (videoConfig.sequenceParameterSets.empty() || videoConfig.pictureParameterSets.empty()) {
                        logger.error("expected first access unit to contain sps and pps");
                        return;
                    }
                }

                auto encoded = videoConfig.encode();
                handler->handleEncodedVideoConfig(encoded.data(), encoded.size());
                didOutputVideoConfig = true;
            }

            auto pts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(packet.pts * av_q2d(stream->time_base)));
            auto dts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(packet.dts * av_q2d(stream->time_base)));

            outputBuffer.clear();

            static const auto filter = [](unsigned int naluType) {
                switch (naluType) {
                    case h264::NALUnitType::SequenceParameterSet:
                    case h264::NALUnitType::PictureParameterSet:
                    case h264::NALUnitType::AccessUnitDelimiter:
                    case h264::NALUnitType::PrefixNALUnit:
                        return false;
                }
                return true;
            };

            if (isAVCCIn) {
                if (!h264::FilterAVCC(&outputBuffer, packet.data, packet.size, videoConfig.lengthSizeMinusOne + 1, filter)) {
                    logger.error("unable to filter avcc");
                    return;
                }
            } else {
                if (!h264::AnnexBToAVCC(&outputBuffer, packet.data, packet.size, filter)) {
                    logger.error("unable to convert annex-b to avcc");
                    return;
                }
            }

            handler->handleEncodedVideo(pts, dts, outputBuffer.data(), outputBuffer.size());
            ++_framesDemuxed;
        } else if (mediaType == AVMEDIA_TYPE_AUDIO) {
            if (!didOutputAudioConfig) {
                auto codec = stream->codecpar;

                MPEG4AudioSpecificConfig config;

                switch (codec->profile) {
                case FF_PROFILE_AAC_MAIN:
                    config.objectType = MPEG4AudioObjectType::AACMain;
                    break;
                case FF_PROFILE_AAC_LOW:
                    config.objectType = MPEG4AudioObjectType::AACLC;
                    break;
                case FF_PROFILE_AAC_SSR:
                    config.objectType = MPEG4AudioObjectType::AACSSR;
                    break;
                case FF_PROFILE_AAC_LTP:
                    config.objectType = MPEG4AudioObjectType::AACLTP;
                    break;
                default:
                    logger.error("unsupported aac profile: {}", codec->profile);
                    return;
                }

                config.frequency = codec->sample_rate;

                if (codec->channels < 1 || (codec->channels > 6 && codec->channels != 8)) {
                    logger.error("unsupported channel count: {}", codec->channels);
                    return;
                }
                config.channelConfiguration = codec->channels == 8 ? MPEG4ChannelConfiguration::EightChannels : MPEG4ChannelConfiguration(codec->channels);

                auto encoded = config.encode();
                handler->handleEncodedAudioConfig(encoded.data(), encoded.size());
                didOutputAudioConfig = true;
            }
            auto pts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(packet.pts * av_q2d(stream->time_base)));
            handler->handleEncodedAudio(pts, packet.data, packet.size);
        }
    }
}
