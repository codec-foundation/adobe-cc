#ifndef MOVIE_READER_HPP
#define MOVIE_READER_HPP

#ifdef _WIN64
// TODO: needed?
#include <Windows.h>
#endif

#include "ffmpeg_helpers.hpp"
#include "sample_cache.hpp"


// ffmpeg libavformat-based file writing
class MovieReader
{
public:
    MovieReader(
        VideoFormat videoFormat,
        MovieFile file
    );
    ~MovieReader();

    void readVideoFrame(int iFrame, std::vector<uint8_t>& frame);
    bool hasAudio() const { return audioStreamIdx_ != -1;  }
    const AudioDef& audioDef() const { if (hasAudio()) return *audioDef_; else throw std::runtime_error("no audio"); }
    const int64_t numAudioFrames() const { if (hasAudio()) return audioCache_->numFrames(); else throw std::runtime_error("no audio"); }
    void readAudio(size_t samplePos, size_t size, std::vector<uint8_t> &audio_);

    int width() const { return video_.width; }
    int height() const { return video_.height;  }
    int frameRateNumerator() const { return video_.frameRate.numerator; }
    int frameRateDenominator() const { return video_.frameRate.denominator; }
    int64_t numFrames() const { return video_.maxFrames; }
    VideoDef video() const { return video_; }

private:
    std::string filespec_; // path + filename
    int64_t fileSize_;     // !!! needed by avio seek; see if can be removed

    //CodecContext videoCodecContext_;
    FormatContext formatContext_;
    IOContext ioContext_;
    int videoStreamIdx_{-1};
    int audioStreamIdx_{-1};

    int frameRateNumerator_{0};
    int frameRateDenominator_{0};
    int64_t numFrames_{0};

    VideoDef video_;
    
    // audio, valid if audioStreamIdx_>=0
    std::unique_ptr<AudioDef> audioDef_;
    std::unique_ptr<SampleCache> audioCache_;
    SampleCache::Range loadAudio(size_t pos, uint8_t *into_begin, size_t into_size);

    // adapt writers that throw exceptions
    static int c_onRead(void *context, uint8_t *data, int size);
    static int64_t c_onSeek(void *context, int64_t offset, int whence);
    // we're forced to allocate a buffer for AVIO.
    // TODO: get an no idea what an appropriate size is
    // TODO: hook libav allocation system so it can use externally cached memory management
    const size_t cAVIOBufferSize = 2 << 20;

    MovieReadCallback onRead_;
    MovieSeekCallback onSeek_;
    MovieCloseCallback onClose_;
};

#endif   // MOVIE_WRITER_HPP
