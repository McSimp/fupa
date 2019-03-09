#pragma once

#include "pch.h"

class ChainedReader
{
public:
    ChainedReader(std::unique_ptr<IDecompressedFileReader> baseReader);
    void PushFile(std::unique_ptr<IDecompressedFileReader> reader, bool skipHeader);
    void GotoNextFile();
    void ReadData(char* buffer, size_t bytesToRead, size_t skipBytes = 0);

private:
    struct FileDescriptor
    {
        std::unique_ptr<IDecompressedFileReader> File;
        size_t BytesRemaining;

        FileDescriptor(std::unique_ptr<IDecompressedFileReader> file, size_t bytesRemaining) :
            File(std::move(file)),
            BytesRemaining(bytesRemaining)
        {

        }
    };

    std::vector<FileDescriptor> m_files;
    size_t m_currentFile;
    std::shared_ptr<spdlog::logger> m_logger;
};
