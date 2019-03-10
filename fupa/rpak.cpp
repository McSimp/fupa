#include "pch.h"

const char kPatchArray1Values[] = { 0, 1, 2, 3, 4, 5, 6 };
const char kPatchArray2Values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };
const uint64_t kSpecialPatchAmounts[] = { 3, 7, 6 };

size_t(RPakFile::*(RPakFile::PatchFunctions)[])(char* buffer, size_t bytesToRead) = {
    &RPakFile::PatchFuncRead,
    &RPakFile::PatchFuncSkip,
    &RPakFile::PatchFuncInsert,
    &RPakFile::PatchFuncReplace,
    &RPakFile::PatchFuncReplaceOneThenRead,
    &RPakFile::PatchFuncReplaceOneThenRead,
    &RPakFile::PatchFuncReplaceTwoThenRead
};

RPakFile::RPakFile(std::string name, int pakNumber, tFileOpenerFunc fileOpener) :
    m_reader(std::move(fileOpener(name, pakNumber))),
    m_name(name),
    m_fileOpener(fileOpener)
{
    m_logger = spdlog::get("logger");
}

RPakFile::~RPakFile()
{
    for (uint32_t i = 0; i < kNumSlots; i++)
    {
        _aligned_free(m_slotData[i]);
        m_slotData[i] = nullptr;
    }
}

void RPakFile::Load()
{
    m_logger->info("Loading {}", m_name);
    ReadHeader();
    LoadSections();
    ApplyRelocations();
}

uint32_t RPakFile::GetNumAssets()
{
    return m_outerHeader.NumAssets;
}

std::unique_ptr<IAsset> RPakFile::GetAsset(uint32_t index)
{
    if (index >= m_outerHeader.NumAssets)
    {
        m_logger->error("Index {} is out range of assets ({})", index, m_outerHeader.NumAssets);
        return nullptr;
    }

    AssetDefinition* asset = &m_assetDefinitions[index];
    if (!IsReferenceValid(asset->MetadataRef))
    {
        m_logger->error("MetadataRef invalid for asset {}, returning null asset", index);
        return nullptr;
    }

    const uint8_t* metadata = reinterpret_cast<uint8_t*>(m_sectionPointers[asset->MetadataRef.Section] + asset->MetadataRef.Offset);
    const uint8_t* data = IsReferenceValid(asset->DataRef) ? reinterpret_cast<uint8_t*>(m_sectionPointers[asset->DataRef.Section] + asset->DataRef.Offset) : nullptr;
    auto result = AssetFactory::Create(const_cast<const AssetDefinition*>(asset), metadata, data);
    if (!result)
    {
        m_logger->warn("Failed to create IAsset for asset {} of type {:.4s} - no type implementation exists", index, reinterpret_cast<char*>(&asset->Type));
        return nullptr;
    }

    return result;
}

void RPakFile::ReadHeader()
{
    // Read the outer header
    m_logger->debug("Reading outer header");
    m_reader.ReadData(reinterpret_cast<char*>(&m_outerHeader), sizeof(m_outerHeader));

    if (m_outerHeader.Signature != kRpakSignature)
    {
        throw std::runtime_error("Signature of file does not match");
    }

    if (m_outerHeader.Version != kExpectedVersion)
    {
        throw std::runtime_error(fmt::format("Version {} does not match expected version {}", m_outerHeader.Version, kExpectedVersion));
    }

    // Print out the header values
    m_logger->debug("====== Outer Header ======");
    m_logger->debug("Flags: 0x{:x}", m_outerHeader.Flags);
    m_logger->debug("DecompressedSize: 0x{:x}", m_outerHeader.DecompressedSize);
    m_logger->debug("StarpakPathBlockSize: 0x{:x}", m_outerHeader.StarpakPathBlockSize);
#ifdef APEX
    m_logger->debug("FullStarpakPathBlockSize: 0x{:x}", m_outerHeader.FullStarpakPathBlockSize);
#endif
    m_logger->debug("NumSlotDescriptors: {}", m_outerHeader.NumSlotDescriptors);
    m_logger->debug("NumSections: {}", m_outerHeader.NumSections);
    m_logger->debug("NumRPakLinks: {}", m_outerHeader.NumRPakLinks);
    m_logger->debug("NumRelocations: {}", m_outerHeader.NumRelocations);
    m_logger->debug("NumAssets: {}", m_outerHeader.NumAssets);
    m_logger->debug("NumExtraHeader8Bytes: {}", m_outerHeader.NumExtraHeader8Bytes);
    m_logger->debug("NumExtraHeader4Bytes1: {}", m_outerHeader.NumExtraHeader4Bytes1);
    m_logger->debug("NumExtraHeader4Bytes2: {}", m_outerHeader.NumExtraHeader4Bytes2);
    m_logger->debug("NumExtraHeader1Bytes: {}", m_outerHeader.NumExtraHeader1Bytes);

    m_bytesUntilNextPatch = m_outerHeader.DecompressedSize - sizeof(OuterHeader) + (m_outerHeader.NumRPakLinks != 0 ? 0 : 1);
    m_patchInstruction = &RPakFile::PatchFuncRead;

    // Read data on links to other RPaks
    if (m_outerHeader.NumRPakLinks != 0)
    {
        ReadPatchedData(reinterpret_cast<char*>(&m_patchDataBlockSize), sizeof(m_patchDataBlockSize));
        ReadPatchedData(reinterpret_cast<char*>(&m_startingSectionOffset), sizeof(m_startingSectionOffset));

        m_linkedRPakSizes = std::make_unique<LinkedRPakSize[]>(m_outerHeader.NumRPakLinks);
        ReadPatchedData(reinterpret_cast<char*>(m_linkedRPakSizes.get()), sizeof(LinkedRPakSize) * m_outerHeader.NumRPakLinks);

        m_linkedRPakNumbers = std::make_unique<uint16_t[]>(m_outerHeader.NumRPakLinks);
        ReadPatchedData(reinterpret_cast<char*>(m_linkedRPakNumbers.get()), sizeof(uint16_t) * m_outerHeader.NumRPakLinks);

        m_logger->debug("====== Patch Information ======");
        m_logger->debug("Patch Block Size: 0x{:x}", m_patchDataBlockSize);
        m_logger->debug("Second: 0x{:x}", m_startingSectionOffset);

        m_logger->debug("====== RPak Links ======");
        for (uint16_t i = 0; i < m_outerHeader.NumRPakLinks; i++)
        {
            m_reader.PushFile(m_fileOpener(m_name, m_linkedRPakNumbers[i]), true);
            m_logger->debug("{}: Size: 0x{:x}, Decompressed Size: 0x{:x}, Number: {}", i, m_linkedRPakSizes[i].SizeOnDisk, m_linkedRPakSizes[i].DecompressedSize, m_linkedRPakNumbers[i]);
        }
    }

    // Read starpak paths
    if (m_outerHeader.StarpakPathBlockSize != 0)
    {
        m_logger->debug("====== Starpak Paths ======");
        m_logger->debug("Size: 0x{:x}", m_outerHeader.StarpakPathBlockSize);

        std::unique_ptr<char[]> starpakBlock = std::make_unique<char[]>(m_outerHeader.StarpakPathBlockSize);
        ReadPatchedData(starpakBlock.get(), m_outerHeader.StarpakPathBlockSize);

        m_starpakPaths = ParseStarpakBlock(starpakBlock.get(), m_outerHeader.StarpakPathBlockSize);
    }

    if (m_outerHeader.FullStarpakPathBlockSize != 0)
    {
        m_logger->debug("====== Full Starpak Paths ======");
        m_logger->debug("Size: 0x{:x}", m_outerHeader.FullStarpakPathBlockSize);

        std::unique_ptr<char[]> fullStarpakBlock = std::make_unique<char[]>(m_outerHeader.FullStarpakPathBlockSize);
        ReadPatchedData(fullStarpakBlock.get(), m_outerHeader.FullStarpakPathBlockSize);

        m_fullStarpakPaths = ParseStarpakBlock(fullStarpakBlock.get(), m_outerHeader.FullStarpakPathBlockSize);
    }

    if (m_outerHeader.NumSlotDescriptors == 0)
    {
        throw std::runtime_error("NumSlotDescriptors was 0");
    }

    // Read slot descriptors and allocate memory
    m_logger->debug("====== Slot Descriptors ======");
    m_logger->debug("Size: 0x{:x}", sizeof(SlotDescriptor) * m_outerHeader.NumSlotDescriptors);

    m_slotDescriptors = std::make_unique<SlotDescriptor[]>(m_outerHeader.NumSlotDescriptors);
    ReadPatchedData(reinterpret_cast<char*>(m_slotDescriptors.get()), sizeof(SlotDescriptor) * m_outerHeader.NumSlotDescriptors);

    // Calculate sizes, offsets, and alignments
    uint64_t slotSizes[kNumSlots] = {};
    uint64_t slotDescOffsets[32] = {}; // TODO: Probably should make this dynamically allocated
    uint32_t slotAlignments[kNumSlots] = {};
    for (uint16_t i = 0; i < m_outerHeader.NumSlotDescriptors; i++)
    {
        SlotDescriptor& slotDesc = m_slotDescriptors[i];
        uint32_t slotNum = slotDesc.Slot & 3; // TODO: This will need to be updated if total slots changes
        slotAlignments[slotNum] = std::max(slotAlignments[slotNum], slotDesc.Alignment);

        uint64_t offset = (slotSizes[slotNum] + slotDesc.Alignment - 1) & ~static_cast<uint64_t>(slotDesc.Alignment - 1);
        slotDescOffsets[i] = offset;

        slotSizes[slotNum] = offset + slotDesc.Size;

        m_logger->debug("{}: SlotNum: {}, Alignment: 0x{:x}, Size: 0x{:x}, Offset: 0x{:x}", i, slotNum, slotDesc.Alignment, slotDesc.Size, offset);
    }

    m_logger->debug("====== Slot Totals ======");
    for (uint32_t i = 0; i < kNumSlots; i++)
    {
        m_logger->debug("{}: Size: 0x{:x}, Alignment: 0x{:x}", i, slotSizes[i], slotAlignments[i]);
    }

    // Allocate memory for slot data
    for (uint32_t i = 0; i < kNumSlots; i++)
    {
        if (slotSizes[i] > 0)
        {
            m_slotData[i] = static_cast<char*>(_aligned_malloc(slotSizes[i], slotAlignments[i]));
        }
        else
        {
            m_slotData[i] = nullptr;
        }
    }

    // Read section information and make array of pointers to each of them
    m_logger->debug("====== Section Descriptors ======");
    m_logger->debug("Size: 0x{:x}", sizeof(SectionDescriptor) * m_outerHeader.NumSections);

    m_sectionDescriptors = std::make_unique<SectionDescriptor[]>(m_outerHeader.NumSections);
    ReadPatchedData(reinterpret_cast<char*>(m_sectionDescriptors.get()), sizeof(SectionDescriptor) * m_outerHeader.NumSections);

    m_sectionPointers = std::make_unique<char*[]>(m_outerHeader.NumSections);

    for (uint16_t i = 0; i < m_outerHeader.NumSections; i++)
    {
        SectionDescriptor& sectDesc = m_sectionDescriptors[i];
        uint64_t offset = (slotDescOffsets[sectDesc.SlotDescIndex] + sectDesc.Alignment - 1) & ~static_cast<uint64_t>(sectDesc.Alignment - 1);
        m_sectionPointers[i] = m_slotData[m_slotDescriptors[sectDesc.SlotDescIndex].Slot & 3] + offset; // TODO: This will need to be updated if total slots changes
        slotDescOffsets[sectDesc.SlotDescIndex] = offset + sectDesc.Size;
        m_logger->debug("{}: SlotDescIdx: {}, Alignment: 0x{:x}, Size: 0x{:x}, Offset: 0x{:x}, Data: {}", i, sectDesc.SlotDescIndex, sectDesc.Alignment, sectDesc.Size, offset, static_cast<void*>(m_sectionPointers[i]));
    }

    // Read relocation information
    m_logger->debug("====== Relocation Descriptors ======");
    m_logger->debug("Size: 0x{:x}", sizeof(SectionReference) * m_outerHeader.NumRelocations);

    m_relocationDescriptors = std::make_unique<SectionReference[]>(m_outerHeader.NumRelocations);
    ReadPatchedData(reinterpret_cast<char*>(m_relocationDescriptors.get()), sizeof(SectionReference) * m_outerHeader.NumRelocations);

    for (uint32_t i = 0; i < m_outerHeader.NumRelocations; i++)
    {
        SectionReference& reloc = m_relocationDescriptors[i];
        m_logger->trace("{}: Section: {}, Offset: 0x{:x}", i, reloc.Section, reloc.Offset);
    }

    // Read asset definitions
    m_logger->debug("====== Asset Definitions ======");
    m_logger->debug("Size: 0x{:x}", sizeof(AssetDefinition) * m_outerHeader.NumAssets);

    m_assetDefinitions = std::make_unique<AssetDefinition[]>(m_outerHeader.NumAssets);
    ReadPatchedData(reinterpret_cast<char*>(m_assetDefinitions.get()), sizeof(AssetDefinition) * m_outerHeader.NumAssets);
    std::unordered_map<uint32_t, int> assetCounts;
    for (uint32_t i = 0; i < m_outerHeader.NumAssets; i++)
    {
        char* assetType = reinterpret_cast<char*>(&m_assetDefinitions[i].Type);
        m_logger->debug("{}: {:.4s}", i, assetType);
        ++assetCounts[m_assetDefinitions[i].Type];
    }

    // Print out statistics on total number of each asset type
    m_logger->info("====== Asset Totals ======");
    for (auto const&[key, val] : assetCounts)
    {
        m_logger->info("{:.4s}: {}", reinterpret_cast<const char*>(&key), val);
    }

    // Read extra header
    m_logger->debug("====== Extra Header ======");
    size_t extraHeaderSize = (m_outerHeader.NumExtraHeader8Bytes * 8) + (m_outerHeader.NumExtraHeader4Bytes1 * 4) + (m_outerHeader.NumExtraHeader4Bytes2 * 4) + m_outerHeader.NumExtraHeader1Bytes;
    m_logger->debug("Size: 0x{:x}", extraHeaderSize);

    m_extraHeader = std::make_unique<char[]>(extraHeaderSize);
    ReadPatchedData(reinterpret_cast<char*>(m_extraHeader.get()), extraHeaderSize);

    // Read patch data block
    if (m_outerHeader.NumRPakLinks != 0)
    {
        m_logger->debug("====== Patch Data Block ======");
        m_logger->debug("Size: 0x{:x}", m_patchDataBlockSize);

        m_patchDataBlock = std::make_unique<uint8_t[]>(m_patchDataBlockSize);
        ReadPatchedData(reinterpret_cast<char*>(m_patchDataBlock.get()), m_patchDataBlockSize);

        // Construct patch data arrays
        uint8_t* nextBlock = m_patchDataBlock.get() + rtech::ConstructPatchArray(m_patchDataBlock.get(), 6, kPatchArray1Values, m_patchData1, m_patchData2);
        nextBlock += rtech::ConstructPatchArray(nextBlock, 8, kPatchArray2Values, m_patchData3, m_patchData4);
        uint64_t val = *reinterpret_cast<uint64_t*>(nextBlock);

        // Set the initial patch variables
        m_patch704 = 24;
        m_patch696 = val >> 24;
        m_patch680 = reinterpret_cast<uint64_t*>(nextBlock) + 1;
        m_currentPatchData = nextBlock + (val & 0xFFFFFF);
    }
}

void RPakFile::LoadSections()
{
    // Sectors are stored sequentially after the header information, so just read them in order
    for (uint32_t i = 0; i < m_outerHeader.NumSections; i++)
    {
        int32_t section = NormalizeSection(i);
        if (m_sectionDescriptors[section].Size > 0)
        {
            m_logger->debug("Reading section {} (0x{:x} bytes)", section, m_sectionDescriptors[section].Size);
            ReadPatchedData(m_sectionPointers[section], m_sectionDescriptors[section].Size);
        }
    }
}

void RPakFile::ApplyRelocations()
{
    // Each relocation entry is a reference to some offset in a section. Located at this offset is another reference
    // to a section and an offset. This latter reference is converted into a real pointer (8 bytes since we only deal
    // with 64-bit) and written to the location specified in the relocation.
    for (uint32_t i = 0; i < m_outerHeader.NumRelocations; i++)
    {
        SectionReference& relocLoc = m_relocationDescriptors[i];
        if (!IsReferenceValid(relocLoc))
        {
            throw std::runtime_error(fmt::format("Relocation descriptor {} is invalid: offset 0x{:x} in section {}", i, relocLoc.Offset, relocLoc.Section));
        }

        // Get the reference at the location referenced by the relocation entry
        SectionReference* ref = reinterpret_cast<SectionReference*>(m_sectionPointers[relocLoc.Section] + relocLoc.Offset);
        if (!IsReferenceValid(*ref))
        {
            throw std::runtime_error(fmt::format("Relocation {}'s inner reference is invalid: offset 0x{:x} in section {}", i, ref->Offset, ref->Section));
        }

        // Update the reference data to be a real pointer
        *reinterpret_cast<char**>(ref) = m_sectionPointers[ref->Section] + ref->Offset;
    }
}

std::vector<std::string> RPakFile::ParseStarpakBlock(const char* data, size_t blockSize)
{
    std::vector<std::string> starpakPaths;
    size_t offset = 0;
    while (offset < blockSize)
    {
        const char* path = data + offset;
        size_t pathLen = strlen(path);
        if (pathLen > 0)
        {
            starpakPaths.emplace_back(path, pathLen);
            m_logger->debug("{}", starpakPaths.back());
            if (starpakPaths.back().find("_hotswap.starpak") != std::string::npos)
            {
                throw std::runtime_error("Unexpected hotswap starpak present in file");
            }
            offset += pathLen + 1;
        }
        else
        {
            break;
        }
    }

    return starpakPaths;
}

void RPakFile::ReadPatchedData(char* buffer, size_t bytesToRead)
{
    size_t bytesRead = 0;
    while (bytesRead != bytesToRead)
    {
        if (m_bytesUntilNextPatch > 0)
        {
            size_t read = (*this.*m_patchInstruction)(buffer + bytesRead, bytesToRead - bytesRead);
            bytesRead += read;
        }
        else
        {
            UpdatePatchInstruction();
        }
    }
}

int32_t RPakFile::NormalizeSection(uint32_t section)
{
    int32_t val = section + m_startingSectionOffset;
    if (val >= m_outerHeader.NumSections)
    {
        val -= m_outerHeader.NumSections;
    }
    return val;
}

void RPakFile::UpdatePatchInstruction()
{
    m_patch696 |= *m_patch680 << (64 - static_cast<uint8_t>(m_patch704));
    m_patch680 = reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_patch680) + (m_patch704 >> 3));
    m_patch704 = m_patch704 & 7;

    int64_t index = m_patch696 & 0x3F;
    uint8_t opcode = m_patchData1[index];
    uint8_t patchData2Val = m_patchData2[index];

    m_patch696 >>= patchData2Val;
    m_patch704 += patchData2Val;

    if (opcode >= kNumPatchFunctions)
    {
        throw std::runtime_error("Patch opcode invalid");
    }

    m_patchInstruction = RPakFile::PatchFunctions[opcode];

    if (opcode > 3)
    {
        m_bytesUntilNextPatch = kSpecialPatchAmounts[opcode - 4];
    }
    else
    {
        uint8_t patchData3Val = m_patchData3[static_cast<uint8_t>(m_patch696)];
        uint8_t patchData4Val = m_patchData4[static_cast<uint8_t>(m_patch696)];
        uint64_t new696Val = m_patch696 >> patchData4Val;
        m_patch696 = new696Val >> patchData3Val;
        m_bytesUntilNextPatch = (1ULL << patchData3Val) + (new696Val & ((1ULL << patchData3Val) - 1));
        m_patch704 += patchData3Val + patchData4Val;
    }
}

bool RPakFile::IsReferenceValid(SectionReference& ref)
{
    if (ref.Section >= m_outerHeader.NumSections)
    {
        return false;
    }

    if (ref.Offset >= m_sectionDescriptors[ref.Section].Size)
    {
        return false;
    }

    return true;
}

// All these functions assume that bytesToRead > 0

size_t RPakFile::PatchFuncRead(char* buffer, size_t bytesToRead)
{
    // Just read data from the input
    size_t read = std::min(bytesToRead, m_bytesUntilNextPatch);
    m_logger->trace("Reading 0x{:x} bytes (wanted to read 0x{:x}) - m_bytesUntilNextPatch = 0x{:x}", read, bytesToRead, m_bytesUntilNextPatch);
    m_reader.ReadData(buffer, read);
    m_bytesUntilNextPatch -= read;
    // TODO: Write to debug output file and log current output position
    return read;
}

size_t RPakFile::PatchFuncSkip(char* buffer, size_t bytesToRead)
{
    // Advance the internal pointer by as much as possible, but don't read anything
    size_t skip = m_bytesUntilNextPatch;
    m_logger->trace("Skipping 0x{:x} bytes - m_bytesUntilNextPatch = 0x{:x}", skip, m_bytesUntilNextPatch);
    m_reader.ReadData(buffer, 0, skip);
    m_bytesUntilNextPatch -= skip;
    return 0;
}

size_t RPakFile::PatchFuncInsert(char* buffer, size_t bytesToRead)
{
    // For an insert, the input data stream doesn't progress - only the output
    size_t insert = std::min(bytesToRead, m_bytesUntilNextPatch);
    m_logger->trace("Inserting 0x{:x} bytes - m_bytesUntilNextPatch = 0x{:x}", insert, m_bytesUntilNextPatch);
    memcpy(buffer, m_currentPatchData, insert);
    m_currentPatchData += insert;
    m_bytesUntilNextPatch -= insert;
    // TODO: Write to debug output file and log current output position
    return insert;
}

size_t RPakFile::PatchFuncReplace(char* buffer, size_t bytesToRead)
{
    // For a replace, the input data also progresses
    size_t replace = std::min(bytesToRead, m_bytesUntilNextPatch);
    m_logger->trace("Replacing 0x{:x} bytes - m_bytesUntilNextPatch = 0x{:x}", replace, m_bytesUntilNextPatch);
    memcpy(buffer, m_currentPatchData, replace);
    m_reader.ReadData(buffer, 0, replace);
    m_currentPatchData += replace;
    m_bytesUntilNextPatch -= replace;
    // TODO: Write to debug output file and log current output position
    return replace;
}

size_t RPakFile::PatchFuncReplaceOneThenRead(char* buffer, size_t bytesToRead)
{
    m_logger->trace("Replacing 1 byte then reading - m_bytesUntilNextPatch = 0x{:x}", m_bytesUntilNextPatch);
    buffer[0] = m_currentPatchData[0];
    m_reader.ReadData(buffer, 0, 1);
    m_currentPatchData++;
    m_patchInstruction = &RPakFile::PatchFuncRead;
    // TODO: Write to debug output file and log current output position
    return 1 + (bytesToRead > 1 ? PatchFuncRead(buffer + 1, bytesToRead - 1) : 0);
}

size_t RPakFile::PatchFuncReplaceTwoThenRead(char* buffer, size_t bytesToRead)
{
    // If we can write both bytes, do that then execute the read
    // Otherwise, if we can only write 1, do that then switch to ReplaceOneThenRead
    m_logger->trace("Replacing 2 bytes then reading - m_bytesUntilNextPatch = 0x{:x}", m_bytesUntilNextPatch);

    size_t replace = std::min(bytesToRead, 2ULL);
    memcpy(buffer, m_currentPatchData, replace);
    m_reader.ReadData(buffer, 0, replace);
    m_currentPatchData += replace;

    // TODO: Write to debug output file and log current output position

    if (bytesToRead >= 2)
    {
        m_patchInstruction = &RPakFile::PatchFuncRead;
        return 2 + (bytesToRead > 2 ? PatchFuncRead(buffer + 2, bytesToRead - 2) : 0);
    }
    else if (bytesToRead == 1)
    {
        m_patchInstruction = &RPakFile::PatchFuncReplaceOneThenRead;
        return 1;
    }
    else
    {
        throw std::runtime_error("Unexpected value for bytesToRead");
    }
}
