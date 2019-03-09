#pragma once

class PreprocessedFileReader : public IDecompressedFileReader
{
public:
    PreprocessedFileReader(std::string filename);

    std::string GetFileName() override;
    void ReadData(char* buffer, size_t bytesToRead, size_t skipBytes) override;
    size_t GetFileSize() override;

private:
    std::string m_filename;
    std::ifstream m_file;
    size_t m_size;
};
