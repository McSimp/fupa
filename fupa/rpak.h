#pragma once

const size_t kNumSlots = 4;
const size_t kNumPatchFunctions = 7;

typedef std::function<std::unique_ptr<IDecompressedFileReader>(const std::string&, int)> tRpakOpenerFunc;

class RPakFile
{
public:
    RPakFile(std::string name, int pakNumber, tRpakOpenerFunc rpakOpener);
    ~RPakFile();
    void Load();
    uint32_t GetNumAssets();
    const AssetDefinition* GetAssetDefinition(uint32_t index);
    std::unique_ptr<IAsset> GetAsset(uint32_t index);
    const std::vector<std::string>& GetStarpakPaths() const;
#ifdef APEX
    const std::vector<std::string>& GetFullStarpakPaths() const;
#endif

private:
    void ReadHeader();
    void LoadSections();
    void ApplyRelocations();

    std::vector<std::string> ParseStarpakBlock(const char* data, size_t blockSize);
    void ReadPatchedData(char* buffer, size_t bytesToRead);
    int32_t NormalizeSection(uint32_t section);
    void UpdatePatchInstruction();
    bool IsReferenceValid(SectionReference& ref);

    // Patch functions
    size_t PatchFuncRead(char* buffer, size_t bytesToRead);
    size_t PatchFuncSkip(char* buffer, size_t bytesToRead);
    size_t PatchFuncInsert(char* buffer, size_t bytesToRead);
    size_t PatchFuncReplace(char* buffer, size_t bytesToRead);
    size_t PatchFuncReplaceOneThenRead(char* buffer, size_t bytesToRead);
    size_t PatchFuncReplaceTwoThenRead(char* buffer, size_t bytesToRead);

    static size_t(RPakFile::*PatchFunctions[kNumPatchFunctions])(char* buffer, size_t bytesToRead);

    std::shared_ptr<spdlog::logger> m_logger;
    ChainedReader m_reader;
    std::string m_name;
    tRpakOpenerFunc m_rpakOpener;

    OuterHeader m_outerHeader;

    // Patch information
    uint32_t m_patchDataBlockSize;
    uint32_t m_startingSectionOffset;
    std::unique_ptr<uint8_t[]> m_patchDataBlock;
    uint8_t m_patchData1[64];
    uint8_t m_patchData2[64];
    uint8_t m_patchData3[256];
    uint8_t m_patchData4[256];
    uint64_t* m_patch680;
    uint64_t m_patch696;
    uint32_t m_patch704;
    uint64_t m_bytesUntilNextPatch;
    uint8_t* m_currentPatchData;
    size_t(RPakFile::*m_patchInstruction)(char* buffer, size_t bytesToRead);

    // RPak links
    std::unique_ptr<LinkedRPakSize[]> m_linkedRPakSizes;
    std::unique_ptr<uint16_t[]> m_linkedRPakNumbers;

    // Starpak links
    std::vector<std::string> m_starpakPaths;
#ifdef APEX
    std::vector<std::string> m_fullStarpakPaths;
#endif

    // Slots
    std::unique_ptr<SlotDescriptor[]> m_slotDescriptors;
    char* m_slotData[kNumSlots] = {}; // TODO: Make this a unique_ptr. No idea why it's so hard to do it.

    // Sections
    std::unique_ptr<SectionDescriptor[]> m_sectionDescriptors;
    std::unique_ptr<char*[]> m_sectionPointers; // TODO: This is not really safe because it contains pointers into the allocated memory in m_slotData

    // Relocations
    std::unique_ptr<SectionReference[]> m_relocationDescriptors;

    // Assets
    std::unique_ptr<AssetDefinition[]> m_assetDefinitions;

    // Extra header
    std::unique_ptr<char[]> m_extraHeader;
};
