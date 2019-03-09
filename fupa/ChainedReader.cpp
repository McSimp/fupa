#include "pch.h"

ChainedReader::ChainedReader(std::unique_ptr<IDecompressedFileReader> baseReader)
{
    m_logger = spdlog::get("logger");
    PushFile(std::move(baseReader), false);
    m_currentFile = 0;
}

void ChainedReader::PushFile(std::unique_ptr<IDecompressedFileReader> reader, bool skipHeader)
{
    size_t size = reader->GetFileSize();
    if (skipHeader)
    {
        reader->ReadData(nullptr, 0, sizeof(OuterHeader));
        size -= sizeof(OuterHeader);
    }

    m_logger->trace("Pushing file {} with size 0x{:x}", reader->GetFileName(), size);
    m_files.emplace_back(std::move(reader), size);
}

void ChainedReader::GotoNextFile()
{
    if ((m_currentFile + 1) >= m_files.size())
    {
        throw std::runtime_error("Attempted to seek beyond end of chained files");
    }

    m_currentFile++;
}

void ChainedReader::ReadData(char* buffer, size_t bytesToRead, size_t skipBytes)
{
    size_t bytesSkipped = 0;
    while (bytesSkipped != skipBytes)
    {
        FileDescriptor& f = m_files[m_currentFile];
        if (f.BytesRemaining == 0)
        {
            GotoNextFile();
        }
        size_t skipFromCurrent = std::min(f.BytesRemaining, skipBytes - bytesSkipped);
        f.File->ReadData(nullptr, 0, skipFromCurrent);
        f.BytesRemaining -= skipFromCurrent;
        bytesSkipped += skipFromCurrent;
        m_logger->trace("Skipped 0x{:x} bytes from {}, now at 0x{:x}", skipFromCurrent, f.File->GetFileName(), (f.File->GetFileSize() - f.BytesRemaining));
    }

    size_t bytesRead = 0;
    while (bytesRead != bytesToRead)
    {
        FileDescriptor& f = m_files[m_currentFile];
        if (f.BytesRemaining == 0)
        {
            GotoNextFile();
        }
        size_t readFromCurrent = std::min(f.BytesRemaining, bytesToRead - bytesRead);
        f.File->ReadData(buffer + bytesRead, readFromCurrent);
        f.BytesRemaining -= readFromCurrent;
        bytesRead += readFromCurrent;
        m_logger->trace("Read 0x{:x} bytes from {}, now at 0x{:x}", readFromCurrent, f.File->GetFileName(), (f.File->GetFileSize() - f.BytesRemaining));
    }
}
