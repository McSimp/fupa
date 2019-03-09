#include "pch.h"

PreprocessedFileReader::PreprocessedFileReader(std::string filename) :
    m_filename(filename),
    m_file(filename, std::ios::in | std::ios::binary)
{
    if (!m_file.is_open())
    {
        throw std::runtime_error("Failed to open file");
    }

    m_file.seekg(0, std::ios::end);
    m_size = m_file.tellg();
    m_file.seekg(0, std::ios::beg);
}

std::string PreprocessedFileReader::GetFileName()
{
    return m_filename;
}

void PreprocessedFileReader::ReadData(char* buffer, size_t bytesToRead, size_t skipBytes)
{
    m_file.seekg(skipBytes, std::ios::cur);
    m_file.read(buffer, bytesToRead);
    if (m_file.fail())
    {
        throw std::runtime_error("File read failed");
    }
}
size_t PreprocessedFileReader::GetFileSize()
{
    return m_size;
}
