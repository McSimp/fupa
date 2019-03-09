#include "pch.h"

const size_t kCompressedBufferSize = 0x1000000;
const size_t kDecompressedBufferSize = 0x400000;
const size_t kChunkSize = 512 * 1024;

CompressedFileReader::CompressedFileReader(std::string filename) :
    m_filename(filename),
    m_file(filename, std::ios::in | std::ios::binary),
    m_scratchData(nullptr),
    m_tempCompressedData(nullptr),
    m_tempDecompressedData(nullptr)
{
    if (!m_file.is_open())
    {
        throw std::runtime_error("Failed to open file");
    }

    m_scratchData = reinterpret_cast<char*>(_aligned_malloc(kCompressedBufferSize + kDecompressedBufferSize, 0x1000));
    m_tempCompressedData = m_scratchData;
    m_tempDecompressedData = m_scratchData + kCompressedBufferSize;

    OuterHeader header;
    m_file.read(reinterpret_cast<char*>(&header), sizeof(OuterHeader));
    m_file.seekg(0, std::ios::beg);

    m_compressedSize = header.CompressedSize;
    m_decompressedSize = header.DecompressedSize;
    m_totalDecompressed = sizeof(OuterHeader);
    m_bytesRead = 0;
    m_bytesInDecompressedBuffer = 0;
    m_bytesProcessed = 0;

    size_t dataToRead = std::min(kCompressedBufferSize, m_compressedSize);
    m_file.read(m_tempCompressedData, dataToRead);
    m_bytesRead += dataToRead;

    uint64_t expectedDecompressedSize = rtech::SetupDecompressState(m_decompressionState, m_tempCompressedData, 0xFFFFFF, m_compressedSize, 0, sizeof(OuterHeader));
    if (expectedDecompressedSize != m_decompressedSize)
    {
        throw std::runtime_error(fmt::format("Compressed size in header (0x{:x}) does not match compressed size from data (0x{:x})", m_decompressedSize, expectedDecompressedSize));
    }

    m_decompressionState[1] = reinterpret_cast<uint64_t>(m_tempDecompressedData);
    m_decompressionState[3] = kDecompressedBufferSize - 1;
    memcpy(m_tempDecompressedData, reinterpret_cast<char*>(&header), sizeof(OuterHeader));
    DecompressNext();
}

CompressedFileReader::~CompressedFileReader()
{
    if (m_scratchData != nullptr)
    {
        _aligned_free(m_scratchData);
        m_scratchData = nullptr;
        m_tempCompressedData = nullptr;
        m_tempDecompressedData = nullptr;
    }
}

std::string CompressedFileReader::GetFileName()
{
    return m_filename;
}

void CompressedFileReader::ReadData(char* buffer, size_t bytesToRead, size_t skipBytes)
{
    uint64_t bytesCopied = 0;
    uint64_t bytesSkipped = 0;
    while (bytesSkipped != skipBytes || bytesCopied != bytesToRead)
    {
        if (m_bytesProcessed == m_totalDecompressed)
        {
            DecompressNext();
        }

        uint64_t currentDecompressedBufferOffset = m_bytesProcessed % kDecompressedBufferSize;
        if (currentDecompressedBufferOffset > m_bytesInDecompressedBuffer)
        {
            throw std::runtime_error("currentDecompressedBufferOffset > m_bytesInDecompressedBuffer");
        }

        uint64_t remainingDecompressedBytes = m_bytesInDecompressedBuffer - currentDecompressedBufferOffset;
        if (bytesSkipped != skipBytes)
        {
            uint64_t bytesToSkip = std::min(remainingDecompressedBytes, skipBytes - bytesSkipped);
            bytesSkipped += bytesToSkip;
            m_bytesProcessed += bytesToSkip;
        }
        else
        {
            uint64_t bytesToCopy = std::min(remainingDecompressedBytes, bytesToRead - bytesCopied);
            memcpy(buffer + bytesCopied, m_tempDecompressedData + currentDecompressedBufferOffset, bytesToCopy);
            bytesCopied += bytesToCopy;
            m_bytesProcessed += bytesToCopy;
        }
    }
}

size_t CompressedFileReader::GetFileSize()
{
    return m_decompressedSize;
}

void CompressedFileReader::DecompressNext()
{
    if (m_totalDecompressed >= m_decompressedSize)
    {
        throw std::runtime_error("Cannot decompress next block, already decompressed whole file");
    }

    uint64_t totalDecompressedBefore = m_decompressionState[10] == sizeof(OuterHeader) ? 0 : m_decompressionState[10];
    uint64_t inputChunksProcessedBefore = m_decompressionState[9] / kChunkSize;
    rtech::DoDecompress(m_decompressionState, m_bytesRead, m_totalDecompressed + 0x400000);
    uint64_t totalDecompressedAfter = m_decompressionState[10];
    uint64_t inputChunksProcessedAfter = m_decompressionState[9] / kChunkSize;

    uint64_t chunksProcessed = inputChunksProcessedAfter - inputChunksProcessedBefore;
    for (uint64_t i = 0; i < chunksProcessed; i++)
    {
        size_t dataToRead = std::min(kChunkSize, m_compressedSize - m_bytesRead);
        if (dataToRead == 0)
        {
            break;
        }
        m_file.read(m_tempCompressedData + (m_bytesRead % kCompressedBufferSize), dataToRead);
        m_bytesRead += dataToRead;
    }

    m_bytesInDecompressedBuffer = totalDecompressedAfter - totalDecompressedBefore;
    m_totalDecompressed = m_decompressionState[10];
}
