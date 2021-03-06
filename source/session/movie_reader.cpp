
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string>
#include <numeric>

#define __STDC_CONSTANT_MACROS

extern "C" {
#include <libavutil/channel_layout.h>
}

#include "logging.hpp"
#include "movie_reader.hpp"


#undef av_err2str
static std::string av_err2str(int errnum)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, errnum);
    return buffer;
}

extern "C" {
    extern AVInputFormat ff_mov_demuxer;
}
// =======================================================
MovieReader::MovieReader(
    VideoFormat videoFormat,
    MovieFile file)
    : fileSize_(file.fileSize),
      onRead_(file.onRead), onSeek_(file.onSeek), onClose_(file.onClose)
{
    int ret;

    try {
        uint8_t* buffer = (uint8_t*)av_malloc(cAVIOBufferSize);
        if (!buffer)
            throw std::runtime_error("couldn't allocate write buffer");
        AVIOContext *ioContext = avio_alloc_context(
            buffer,               // unsigned char *buffer,
            (int)cAVIOBufferSize, // int buffer_size,
            0,                    // int write_flag,
            this,                 // void *opaque,
            c_onRead,             // int(*read_packet)(void *opaque, uint8_t *buf, int buf_size),
            nullptr,              // int(*write_packet)(void *opaque, uint8_t *buf, int buf_size),
            c_onSeek);            // int64_t(*seek)(void *opaque, int64_t offset, int whence));
        if (!ioContext)
        {
            av_free(buffer);  // not owned by anyone yet :(
            throw std::runtime_error("couldn't allocate io context");
        }
        ioContext_.reset(ioContext);

        /* allocate the io media context */
        AVFormatContext *formatContext = avformat_alloc_context();
        formatContext->iformat = &ff_mov_demuxer;
        formatContext->pb = ioContext_.get();

        // !!! bugfix - editlist processing on read discards last frame entry
        // !!! TODO: find out why ffmpeg mov.c mov_fix_index is doing this
        AVDictionary* movOptionsDictptr(nullptr);
        av_dict_set(&movOptionsDictptr, "ignore_editlist", "1", 0);
        // !!!

        ret = avformat_open_input(&formatContext, NULL, NULL, &movOptionsDictptr);

        // !!!
        av_dict_free(&movOptionsDictptr);
        // !!!

        if (ret < 0) {
            throw std::runtime_error("format could not open input");
        }

        formatContext_.reset(formatContext); // and own it

        if ((ret = avformat_find_stream_info(formatContext_.get(), 0)) < 0) {
            throw std::runtime_error("Failed to retrieve input stream information");
        }


        //!!!
        //!!!av_dump_format(formatContext_.get(), 0, "input file", 0);

        //frameRateNumerator_ = formatContext_->
        //frameRateDenominator_ = formatContext_->iformat->

        auto desiredVideoTag = MKTAG(videoFormat[0], videoFormat[1], videoFormat[2], videoFormat[3]);

        for (size_t i = 0; i < formatContext_->nb_streams; i++) {
            AVStream *stream = formatContext_->streams[i];
            AVCodecParameters *codecpar = stream->codecpar;

            switch (codecpar->codec_type) {
            case AVMEDIA_TYPE_AUDIO:
                audioStreamIdx_ = (int)i;
                break;
            case AVMEDIA_TYPE_VIDEO:
                if (codecpar->codec_tag == desiredVideoTag) {
                    videoStreamIdx_ = (int)i;

                    int64_t frameRateGCD = std::gcd(stream->r_frame_rate.num, stream->r_frame_rate.den);
                    auto encoderName = av_dict_get(stream->metadata, "encoder", nullptr, 0);

                    video_ = VideoDef{
                        codecpar->width,
                        codecpar->height,
                        videoFormat,
                        encoderName ? encoderName->value : "<unknown>",
                        codecpar->bits_per_coded_sample,
                        Rational{
                            (int)(stream->r_frame_rate.num / frameRateGCD),
                            (int)(stream->r_frame_rate.den / frameRateGCD)
                        },
                        stream->nb_frames
                    };
                }
                break;
            default:
                break;
            }
        }

        if (videoStreamIdx_ == -1)
            throw std::runtime_error("could not find video stream");

        if (audioStreamIdx_ != -1)
        {
            AVStream *stream = formatContext_->streams[audioStreamIdx_];
            audioDef_ = std::make_unique<AudioDef>(getAVCodecParams(*stream->codecpar));
            int bytesPerFrame = audioDef_->bytesPerSample * audioDef_->numChannels;
            audioCache_ = std::make_unique<SampleCache>(
                stream->duration, bytesPerFrame,
                [this](size_t frame, uint8_t *into_begin, size_t into_size)->SampleCache::Range {
                    return loadAudio(frame, into_begin, into_size);
                }
            );
        }
    }
    catch (...)
    {
        try {
            onClose_();  // may throw, but the original error is the one we're interested in
        }
        catch (...)
        {
        }
        throw;
    }
}

void MovieReader::readVideoFrame(int iFrame, std::vector<uint8_t>& frame)
{
    AVStream *stream = formatContext_->streams[videoStreamIdx_];
    int64_t timestamp = (int64_t)iFrame * stream->r_frame_rate.den * stream->time_base.den / (int64_t(stream->r_frame_rate.num) * stream->time_base.num);

    int ret = av_seek_frame(formatContext_.get(), videoStreamIdx_, timestamp, AVSEEK_FLAG_ANY);
    if (ret < 0)
        throw std::runtime_error(std::string("could not seek to read frame " + std::to_string(iFrame) + " - " + av_err2str(ret)));

    AVPacket pkt;
    while (true) {
        ret = av_read_frame(formatContext_.get(), &pkt);
        if (ret < 0)
            throw std::runtime_error(std::string("could not read frame " + std::to_string(iFrame) + " - " + av_err2str(ret)));
        else if (pkt.stream_index == videoStreamIdx_) {
            frame.resize(pkt.size);
            std::copy(pkt.data, pkt.data + pkt.size, &frame[0]);
            av_packet_unref(&pkt);
            return;
        }
        else {
            av_packet_unref(&pkt);
        }
    }
}

void MovieReader::readAudio(size_t pos, size_t size, std::vector<uint8_t>& buffer)
{
    if (!hasAudio())
        throw std::runtime_error("no audio");

    if (pos + size > audioCache_->numFrames())
        throw std::runtime_error("attempt to read past end of audio");

    const uint8_t *samples = audioCache_->get(SampleCache::Range(pos, pos + size));
    int bytesPerFrame = audioDef_->bytesPerSample * audioDef_->numChannels;

    buffer.clear();
    buffer.assign(samples, samples + size*bytesPerFrame);
}

SampleCache::Range MovieReader::loadAudio(size_t frame, uint8_t *into_begin, size_t into_size)
{
    AVStream *stream = formatContext_->streams[audioStreamIdx_];
    int bytesPerFrame = audioDef_->bytesPerSample * audioDef_->numChannels;

    AVPacket pkt;
    while (frame < (size_t)stream->duration) {
        int ret = av_seek_frame(formatContext_.get(), audioStreamIdx_, frame, AVSEEK_FLAG_ANY);
        if (ret < 0)
            throw std::runtime_error(std::string("could not seek to start of audio - " + av_err2str(ret)));

        while (true) {
            ret = av_read_frame(formatContext_.get(), &pkt);
            if (ret < 0)
                throw std::runtime_error(std::string("could not read audio frame - " + av_err2str(ret)));
            else if (pkt.stream_index == audioStreamIdx_) {
                if (into_size < (size_t)pkt.pts * bytesPerFrame + pkt.size)
                    throw std::runtime_error("audio cache not large enough for loadAudio");
                std::copy(pkt.data, pkt.data + pkt.size, into_begin + pkt.pts * bytesPerFrame);
                if (pkt.duration < 0)
                    throw std::runtime_error("audio pkt duration < 0");
                SampleCache::Range loaded{pkt.pts, pkt.pts + pkt.duration};
                if (loaded.first > frame || loaded.second <= frame)
                    throw std::runtime_error("loaded audio pkt that didn't include selected frame");
                av_packet_unref(&pkt);
                return loaded;
            }
            else {
                av_packet_unref(&pkt);
            }
        }
    }
    throw std::runtime_error("attempt to load audio outside of stream duration");
}


MovieReader::~MovieReader()
{
    onClose_();
}


int MovieReader::c_onRead(void *context, uint8_t *data, int size)
{
    MovieReader *obj = reinterpret_cast<MovieReader*>(context);
    try
    {
        return (int)obj->onRead_(data, size);
    }
    catch (const std::exception &ex)
    {
        FDN_ERROR(ex.what());
        return -1;
    }
    catch (...)
    {
        FDN_ERROR("unhandled exception while writing");
        return -1;
    }
    return size;
}

int64_t MovieReader::c_onSeek(void *context, int64_t seekPos, int whence)
{
    MovieReader *obj = reinterpret_cast<MovieReader*>(context);
    try
    {
        if (whence & AVSEEK_SIZE) {
            return obj->fileSize_;
        } else {
            whence &= (~AVSEEK_FORCE);  // we don't want this potential option to interfere

            return obj->onSeek_(seekPos, whence);
        }
    }
    catch (const std::exception &ex)
    {
        FDN_ERROR(ex.what());
        return -1;
    }
    catch (...)
    {
        FDN_ERROR("unhandled exception while seeking");
        return -1;
    }
}

