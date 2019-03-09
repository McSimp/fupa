#pragma once

// Interface for reading decompressed data from a file.
// Works with either decompressing on the fly or just reading from pre-decompressed files on disk.
class IDecompressedFileReader
{
public:
    virtual ~IDecompressedFileReader() {}
    virtual std::string GetFileName() = 0;
    virtual void ReadData(char* buffer, size_t bytesToRead, size_t skipBytes = 0) = 0; // If read fails, throws exception
    virtual size_t GetFileSize() = 0;
};
