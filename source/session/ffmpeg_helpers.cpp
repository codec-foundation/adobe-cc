// ffmpeg_helpers.cpp

#include "ffmpeg_helpers.hpp"
#include "logging.hpp"

MovieFile createMovieFile(const std::string &filename)
{
    MovieFile fileWrapper;
    auto file=std::make_shared<FILE *>((FILE *)nullptr);
    fileWrapper.onOpenForWrite = [=]() {
        FDN_INFO("opening ", filename, " for writing");

#ifdef _WIN64
        fopen_s(file.get(), filename.c_str(), "wb");
#else
        FILE *ptr = fopen(filename.c_str(), "wb");
        *file = ptr;
#endif
        if (!(*file))
            throw std::runtime_error("couldn't open output file");
    };
    fileWrapper.onWrite = [=](const uint8_t* buffer, size_t size) {
        auto nWritten = fwrite(buffer, size, 1, *file);
        if (!nWritten) {
            FDN_ERROR("Could not write to file");
            return -1;
        }
        return 0;
    };
    fileWrapper.onSeek = [=](int64_t offset, int whence) {
#ifdef AE_OS_WIN
        auto result = _fseeki64(*file, offset, whence);
#else
        auto result = fseek(*file, offset, whence);
#endif
        if (0 != result) {
            FDN_ERROR("Could not seek in file");
            return -1;
        }
        return 0;
    };
    fileWrapper.onClose = [=]() {
        return (fclose(*file)==0) ? 0 : -1;
    };

    return fileWrapper;
}
