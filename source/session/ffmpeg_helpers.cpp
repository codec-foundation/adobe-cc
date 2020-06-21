// ffmpeg_helpers.cpp

#include "ffmpeg_helpers.hpp"
#include "logging.hpp"
#include "movie_reader.hpp"

MovieFile createMovieFile(const std::string &filename)
{
    MovieFile fileWrapper;
    auto file=std::make_shared<FILE *>((FILE *)nullptr);
    fileWrapper.onOpenForWrite = [=]() {
        FDN_INFO("opening ", filename, " for writing");

#ifdef WIN32
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
#ifdef WIN32
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


// helper to create movie reader wrapped around a <FileHandle> that is compatible with
// the Adobe SDK
#ifdef WIN32
std::unique_ptr<MovieFile> createMovieFileReader(VideoFormat videoFormat, const fs::path& filePath)
{
	MovieFile file;

	std::shared_ptr<FileHandle> handle = std::make_shared<FileHandle>(0);

    FDN_INFO("opening ", filePath, " for reading");

    HANDLE fileRef = CreateFileW(filePath.wstring(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    // Check to see if file is valid
    if (fileRef == INVALID_HANDLE_VALUE)
    {
        auto error = GetLastError();

        throw std::runtime_error(std::string("could not open ")
                                 + filePath.string() + " - error " + std::to_string(error));
    }

    // fileSize is *only* needed by the seek wrapper for ffmpeg
    LARGE_INTEGER ffs;
    if (GetFileSizeEx(fileRef, &ffs)) {
        file.fileSize = ffs.QuadPart;
    }
    else {
        file.fileSize = -1; // this is expected by seek wrapper as meaning "I can't do that"
    }

	file.onRead = [&, fileRef](uint8_t* buffer, size_t size) {
                DWORD bytesReadLu;
                BOOL ok = ReadFile(fileRef,
                    (LPVOID)buffer, (DWORD)size,
                    &bytesReadLu,
                    NULL);
                if (!ok)
                    throw std::runtime_error("could not read");
                return bytesReadLu;
            };

    file.onSeek = [&, fileRef](int64_t offset, int whence) {
        DWORD         dwMoveMethod;
        LARGE_INTEGER distanceToMove;
        distanceToMove.QuadPart = offset;

        if (whence == SEEK_SET)
            dwMoveMethod = FILE_BEGIN;
        else if (whence == SEEK_END)
            dwMoveMethod = FILE_END;
        else if (whence == SEEK_CUR)
            dwMoveMethod = FILE_CURRENT;
        else
            throw std::runtime_error("unhandled file seek mode");

        BOOL ok = SetFilePointerEx(
            fileRef,
            distanceToMove,
            NULL,
            FILE_BEGIN);

        if (!ok)
            throw std::runtime_error("could not read");

        return 0;
    };

    file.onClose = [fileRef]() {
        CloseHandle(fileRef);
        return 0;
    };

    return std::make_unique<MovieReader>(videoFormat, file);
}

#else

std::unique_ptr<MovieReader> createMovieReader(VideoFormat videoFormat, const fs::path& filePath)
{
	MovieFile file;

    FDN_INFO("opening ", filePath, " for reading");

    FILE *fileRef = fopen(filePath.string().c_str(), "rb");
    
    // Check to see if file is valid
    if (fileRef == nullptr)
    {
        auto error = errno;

        throw std::runtime_error(std::string("could not open ")
                                 + filePath.string() + " - error " + std::to_string(error));
    }

    // fileSize is *only* needed by the seek wrapper for ffmpeg
    if (fseek(fileRef, 0, SEEK_END) == 0)
    {
        // returns -1 on failure, which matches our expected failure value below
        file.fileSize = ftell(fileRef);
    }
    else
    {
        file.fileSize = -1; // this is expected by seek wrapper as meaning "I can't do that"
    }

    rewind(fileRef);

	file.onRead = [&, fileRef](uint8_t* buffer, size_t size) {
        size_t read = fread(static_cast<void *>(buffer), 1, size, fileRef);
        
        if (read != size && !feof(fileRef))
            throw std::runtime_error("could not read");
        return read;
    };

    file.onSeek = [&, fileRef](int64_t offset, int whence) {
        if (fseek(fileRef, offset, whence) != 0)
            throw std::runtime_error("could not seek");

        return 0;
    };
    file.onClose = [fileRef]() {
        fclose(fileRef);
        return 0;
    };

    return std::make_unique<MovieReader>(videoFormat, file);
}

#endif

