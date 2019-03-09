#pragma once

class CompressedFileReader : public IDecompressedFileReader
{
public:
    CompressedFileReader(std::string filename);
    ~CompressedFileReader() override;
    std::string GetFileName() override;
    void ReadData(char* buffer, size_t bytesToRead, size_t skipBytes) override;
    size_t GetFileSize() override;

private:
    void DecompressNext();

    std::string m_filename;
    std::ifstream m_file;
    size_t m_compressedSize;
    size_t m_decompressedSize;
    char* m_scratchData;
    char* m_tempCompressedData;
    char* m_tempDecompressedData;
    uint64_t m_decompressionState[17];
    uint64_t m_bytesInDecompressedBuffer;
    uint64_t m_totalDecompressed;
    uint64_t m_bytesRead;
    uint64_t m_bytesProcessed;
};
