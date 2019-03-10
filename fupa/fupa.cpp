#include "pch.h"

void AddStaticKnownAssetNames()
{
    KnownAssetCache::AddName("scripts/keys_controller_xone.rson");
    KnownAssetCache::AddName("scripts/keys_controller_ps4.rson");
    KnownAssetCache::AddName("scripts/keys_keyboard.rson");
    KnownAssetCache::AddName("scripts/audio/banks.rson");
    KnownAssetCache::AddName("scripts/skins.rson");
    KnownAssetCache::AddName("scripts/audio/metadata_tags.rson");
    KnownAssetCache::AddName("scripts/vscripts/scripts.rson");
    KnownAssetCache::AddName("scripts/audio/environments.rson");
    KnownAssetCache::AddName("scripts/audio/soundmeter_busses.rson");
    KnownAssetCache::AddName("scripts/entitlements.rson");
}

std::unique_ptr<IDecompressedFileReader> FileReaderFactory(const std::string& inputDir, const std::string& rpakName, int number)
{
    auto logger = spdlog::get("logger");
    std::string path = Util::GetRpakPath(inputDir, rpakName, number).string();
    logger->debug("Opening rpak: {}", path);

    // Pre-parse the header to see if the file is compressed or not
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if (!f.is_open())
    {
        throw std::runtime_error("Failed to open file");
    }

    OuterHeader header;
    f.read(reinterpret_cast<char*>(&header), sizeof(OuterHeader));
    f.seekg(0, std::ios::end);
    size_t size = f.tellg();

    // If 1 is not set in flags, file is not compressed
    if ((header.Flags & 0x100) == 0)
    {
        logger->debug("File is not compressed, opening with PreprocessedFileReader");

        if (header.DecompressedSize != header.CompressedSize)
        {
            throw std::runtime_error(fmt::format("Flags in {} state that file is not compressed, but decompressed size != compressed size", path));
        }

        if (size != header.DecompressedSize)
        {
            throw std::runtime_error(fmt::format("Size of {} (0x{:x}) does not match size in header (0x{:x})", path, size, header.CompressedSize));
        }

        return std::make_unique<PreprocessedFileReader>(path);
    }
    
    // Otherwise, file states that it is compressed. We might be trying to open a 
    // file that has been pre-decompressed with fupa though, so we want to use the
    // PreprocessedFileReader in that case.
    if (size == header.DecompressedSize)
    {
        logger->debug("File appears to have been manually decompressed, opening with PreprocessedFileReader");
        return std::make_unique<PreprocessedFileReader>(path);
    }

    if (size != header.CompressedSize)
    {
        throw std::runtime_error(fmt::format("Size of {} (0x{:x}) does not match compressed size in header (0x{:x})", path, size, header.CompressedSize));
    }

    logger->debug("File is compressed, opening with CompressedFileReader");
    return std::make_unique<CompressedFileReader>(path);
}


void VerbosityCallback(size_t count)
{
    if (count == 1)
    {
        spdlog::get("logger")->set_level(spdlog::level::debug);
    }
    else if (count > 1)
    {
        spdlog::get("logger")->set_level(spdlog::level::trace);
    }
}

struct DecompressParams
{
    std::string BinDir;
    std::string RPakFile;
    std::string OutputFile = "decompressed.rpak";
};

void AddDecompressCommand(CLI::App& app)
{
    CLI::App* command = app.add_subcommand("decompress", "Decompress an RPak file");

    auto params = std::make_shared<DecompressParams>();
    command->add_option("-b,--bindir", params->BinDir, "Path to x64_retail in your Titanfall 2 folder")
        ->required();
    command->add_flag("-v", VerbosityCallback, "Verbose output (-vv for very verbose)");
    command->add_option("rpak_file", params->RPakFile, "Path to RPak file to decompress")
        ->required();
    command->add_option("output_file", params->OutputFile, "Path to output file", true);

    command->callback([params]() {
        auto logger = spdlog::get("logger");

        // Check that bindir exists
        logger->debug("TTF2 binary directory: {}", params->BinDir);
        if (!std::filesystem::is_directory(params->BinDir))
        {
            throw std::runtime_error(fmt::format("Invalid --bindir: {} does not exist or is inaccessible", params->BinDir));
        }

        // Initialize rtech functions
        rtech::Initialize(params->BinDir);

        logger->info("Decompressing {} to {}", params->RPakFile, params->OutputFile);

        // Create output file
        std::ofstream outFile(params->OutputFile, std::ios::out | std::ios::binary);
        if (!outFile.is_open())
        {
            throw std::runtime_error("Failed to open output file");
        }

        // Create reader
        auto reader = std::make_unique<CompressedFileReader>(params->RPakFile);
        logger->info("Decompressed size: 0x{:x}", reader->GetFileSize());

        // Read out chunks of the file and write them to the output file
        const uint64_t kBufSize = 0x400000;
        std::unique_ptr<char[]> buf = std::make_unique<char[]>(kBufSize);
        uint64_t bytesRead = 0;
        while (bytesRead != reader->GetFileSize())
        {
            uint64_t toRead = std::min(reader->GetFileSize() - bytesRead, kBufSize);
            reader->ReadData(buf.get(), toRead, 0);
            outFile.write(buf.get(), toRead);
            bytesRead += toRead;
        }

        logger->info("Decompression complete!");
    });
}

struct ExtractParams
{
    std::string BinDir;
    std::string InputDir;
    std::string OutputDir = "extracted";
    std::string KnownAssets;
    std::string RPakName;
};

std::map<std::string, int> GetPatchRPakMap(tFileOpenerFunc opener)
{
    // Parse patch_master.rpak
    RPakFile patchFile("patch_master", 0, opener);
    patchFile.Load();

    // Grab the patch asset and get the asset map
    std::unique_ptr<IAsset> patchAssetGeneric = patchFile.GetAsset(0);
    if (!patchAssetGeneric || patchAssetGeneric->GetType() != kPatchAssetType)
    {
        throw std::runtime_error("Failed to read patch information from patch_master.rpak");
    }

    PatchAsset* patchAsset = static_cast<PatchAsset*>(patchAssetGeneric.get());
    return patchAsset->BuildRPakMap();
}

void AddExtractCommand(CLI::App& app)
{
    CLI::App* command = app.add_subcommand("extract", "Extract an RPak file");

    auto params = std::make_shared<ExtractParams>();
    command->add_option("-b,--bindir", params->BinDir, "Path to x64_retail in your Titanfall 2 folder")
        ->required();
    command->add_option("-i,--inputdir", params->InputDir, "Path to folder containing rpak files")
        ->required();
    command->add_option("-o,--outputdir", params->OutputDir, "Path to folder to write extracted files", true);
    command->add_option("-k,--knownassets", params->KnownAssets, "Path to file with list of known asset names");
    command->add_flag("-v", VerbosityCallback, "Verbose output (-vv for very verbose)");
    command->add_option("rpak_name", params->RPakName, "Name of RPak file to extract (e.g. sp_training)")
        ->required();

    command->callback([params]() {
        auto logger = spdlog::get("logger");

        // Check that bindir exists
        logger->debug("TTF2 binary directory: {}", params->BinDir);
        if (!std::filesystem::is_directory(params->BinDir))
        {
            throw std::runtime_error(fmt::format("Invalid --bindir: {} does not exist or is inaccessible", params->BinDir));
        }

        // Check that inputdir exists
        logger->debug("RPak directory: {}", params->InputDir);
        if (!std::filesystem::is_directory(params->InputDir))
        {
            throw std::runtime_error(fmt::format("Invalid --inputdir: {} does not exist or is inaccessible", params->InputDir));
        }

        // Create outputdir if it doesn't already exist
        logger->debug("Output directory: {}", params->OutputDir);
        std::filesystem::create_directories(params->OutputDir);

        // Initialize D3D stuff
        Microsoft::WRL::Wrappers::RoInitializeWrapper comInit(RO_INIT_MULTITHREADED);
        D3D::Initialize();

        // Initialize rtech functions
        rtech::Initialize(params->BinDir);

        // Add known asset names
        AddStaticKnownAssetNames();
        if (params->KnownAssets != "")
        {
            logger->debug("Known assets file: {}", params->KnownAssets);
            if (!std::filesystem::exists(params->KnownAssets))
            {
                throw std::runtime_error(fmt::format("Invalid --knownassets: {} does not exist or is inaccessible", params->KnownAssets));
            }

            std::ifstream f(params->KnownAssets);
            if (!f.is_open())
            {
                throw std::runtime_error("Failed to open known assets file");
            }

            std::string line;
            while (std::getline(f, line))
            {
                if (line.size() > 0)
                {
                    KnownAssetCache::AddName(line);
                }
            }
        }

        // Initialize asset types
        RegisterCommonAssetTypes();
        RegisterAssetTypes();

        // Create file opener
        using namespace std::placeholders;
        auto opener = std::bind(FileReaderFactory, params->InputDir, _1, _2);

        // Get map of asset names to latest number
        auto assetMap = GetPatchRPakMap(opener);

        // Get the number for the rpak
        int number = 0;
        std::string fullRPakName = params->RPakName + ".rpak";
        if (assetMap.find(fullRPakName) != assetMap.end())
        {
            number = assetMap[fullRPakName];
        }

        logger->info("Latest RPak version for {} is {}", params->RPakName, number);

        // Load the rpak
        RPakFile pak(params->RPakName, number, opener);
        pak.Load();

        // Iterate over assets and dump ones that can be dumped
        for (uint32_t i = 0; i < pak.GetNumAssets(); i++)
        {
            auto asset = pak.GetAsset(i);
            if (asset && asset->CanDump())
            {
                asset->Dump(params->OutputDir);
            }
        }
    });
}

void InitializeLogger()
{
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
    auto logger = std::make_shared<spdlog::logger>("logger", sinks.begin(), sinks.end());
    logger->set_pattern("[%T] [%^%l%$] %v");
    logger->flush_on(spdlog::level::trace);
    spdlog::register_logger(logger);
}

int main(int argc, char** argv)
{
    InitializeLogger();

    CLI::App app{ "A tool to extract data from Respawn's RPak files" };
    app.require_subcommand(1, 1);
    AddDecompressCommand(app);
    AddExtractCommand(app);

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError&)
    {
        std::cerr << app.help() << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
