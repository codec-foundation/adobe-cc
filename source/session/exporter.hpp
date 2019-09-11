#pragma once

#include <atomic>
#include <list>
#include <mutex>
#include <queue>
#include <thread>

#include "movie_writer.hpp"
#include "codec_registration.hpp"
#include "freelist.hpp"

enum class ExportJobType { Video, Audio };

struct ExportJobBase
{
    ExportJobBase() {}
    virtual ~ExportJobBase() {}

    virtual ExportJobType type() const { throw std::runtime_error("no type"); }
    virtual void encode() { }; // readies output 

    int64_t iFrameOrPts{ -1 };  // for video jobs
    EncodeOutput output;

    // noncopyable
    ExportJobBase(const ExportJobBase&) = delete;
    ExportJobBase& operator=(const ExportJobBase&) = delete;
};

struct VideoExportJob : ExportJobBase
{
    VideoExportJob(std::unique_ptr<EncoderJob> codecJob_) : codecJob(std::move(codecJob_)) {}

    virtual ExportJobType type() const override { return ExportJobType::Video; }
    virtual void encode() override { codecJob->encode(output); }

    std::unique_ptr<EncoderJob> codecJob;
};

struct AudioExportJob : ExportJobBase
{
    AudioExportJob() {}

    virtual ExportJobType type() const override { return ExportJobType::Audio; }
};

class ExporterWorker;

typedef FreeList<ExportJobBase> ExporterJobFreeList;

typedef ExporterJobFreeList::PooledT ExportJob;  // either encode or write, depending on the queue its in
typedef std::queue<ExportJob> ExportJobQueue;
typedef std::vector<ExportJob> ExportJobs;
typedef std::list<std::unique_ptr<ExporterWorker> > ExportWorkers;


// thread-safe encoder of ExportJob
class ExporterJobEncoder
{
public:
    ExporterJobEncoder(Encoder& encoder);

    void push(ExportJob job);
    ExportJob encode();

    uint64_t nEncodeJobs() const { return nEncodeJobs_;  }

private:
    Encoder& encoder_;  // must have thread-safe processing functions

    std::mutex mutex_;
    ExportJobQueue queue_;

    std::atomic<uint64_t> nEncodeJobs_;
};

// thread-safe writer of ExportJob
class ExporterJobWriter
{
public:
    ExporterJobWriter(std::unique_ptr<MovieWriter> writer);

    // frames may arrive out of order due to encoding taking varied lengths of time
    // we don't know the index of the first frame until the first frame is dispatched
    // there can be jumps in sequence where there are no source frames (Premiere Pro can skip frames)
    // so when the external host dispatches frames to us, we store the order they arrived, and write
    // them out in that order
    void enqueueVideoFrameWrite(int64_t iFrame);
    void enqueueAudioFrameWrite(int64_t pts);

    void close();  // call ahead of destruction in order to recognise errors

    void push(ExportJob job);
    ExportJob write();  // returns the job that was written

    double utilisation() { return utilisation_; }

private:

    std::mutex mutex_;
    bool error_{false};
    ExportJobs jobs_;
    std::unique_ptr<MovieWriter> writer_;
    std::chrono::high_resolution_clock::time_point idleStart_;
    std::chrono::high_resolution_clock::time_point writeStart_;

    std::atomic<double> utilisation_;

    std::mutex writeOrderMutex_;
    std::queue<std::pair<ExportJobType, int64_t>> writeOrderQueue_; // !!! replace with promise / future
};

class ExporterWorker
{
public:
    ExporterWorker(
        std::atomic<bool>& error,
        ExporterJobEncoder& encoder, ExporterJobWriter& writer);
    ~ExporterWorker();

    static void worker_start(ExporterWorker& worker);

private:
    std::thread worker_;
    void run();

    std::atomic<bool> quit_;
    std::atomic<bool>& error_;
    ExporterJobEncoder& jobEncoder_;
    ExporterJobWriter& jobWriter_;
};


class Exporter
{
public:
    Exporter(
        UniqueEncoder encoder,
        std::unique_ptr<MovieWriter> writer);
    ~Exporter();

    // users should call close if they wish to handle errors on shutdown - destructors cannot
    // throw, 'close' can and will
    void close();
    
    // thread safe to be called 'on frame rendered'
    void dispatchVideo(int64_t iFrame, const uint8_t* data, size_t stride, FrameFormat format) const;
    
    // thread safe, to be called in dispatching thread interleaved with video frames
    // output via MovieWriter will be in same exact sequence
    // this is primarily for AfterEffects where the audio cannot be obtained at beginning
    void dispatchAudio(int64_t pts, const uint8_t* data, size_t size) const;

private:
    bool closed_{false};
    mutable std::unique_ptr<int64_t> currentFrame_;

    bool expandWorkerPoolToCapacity() const;
    int concurrentThreadsSupported_;

    mutable std::atomic<bool> error_{false};
    UniqueEncoder encoder_;

    mutable ExporterJobFreeList videoJobFreeList_;
    mutable ExporterJobFreeList audioJobFreeList_;
    mutable ExporterJobEncoder jobEncoder_;
    mutable ExporterJobWriter jobWriter_;

    // must be last to ensure they're joined before their dependents are destructed
    mutable ExportWorkers workers_;
};
