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

void InitializeFupa(const std::string& binDir)
{
    // Initialize D3D stuff
    Microsoft::WRL::Wrappers::RoInitializeWrapper comInit(RO_INIT_MULTITHREADED);
    D3D::Initialize();

    // Initialize rtech functions
    rtech::Initialize(binDir);

    // Initialize asset types
    RegisterCommonAssetTypes();
    RegisterAssetTypes();
}

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

int GetLatestRPakNumber(tFileOpenerFunc opener, const std::string& rpakName)
{
    // Get map of asset names to latest number
    auto assetMap = GetPatchRPakMap(opener);

    // Get the number for the rpak
    int number = 0;
    std::string fullRPakName = rpakName + ".rpak";
    if (assetMap.find(fullRPakName) != assetMap.end())
    {
        number = assetMap[fullRPakName];
    }

    spdlog::get("logger")->info("Latest RPak version for {} is {}", rpakName, number);

    return number;
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
    std::string RPakName;
};

void AddExtractCommand(CLI::App& app)
{
    CLI::App* command = app.add_subcommand("extract", "Extract an RPak file");

    auto params = std::make_shared<ExtractParams>();
    command->add_option("-b,--bindir", params->BinDir, "Path to x64_retail in your Titanfall 2 folder")
        ->required();
    command->add_option("-i,--inputdir", params->InputDir, "Path to folder containing rpak files")
        ->required();
    command->add_option("-o,--outputdir", params->OutputDir, "Path to folder to write extracted files", true);
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

        InitializeFupa(params->BinDir);

        // Create file opener
        using namespace std::placeholders;
        auto opener = std::bind(FileReaderFactory, params->InputDir, _1, _2);

        // Load the rpak
        RPakFile pak(params->RPakName, GetLatestRPakNumber(opener, params->RPakName), opener);
        pak.Load();

        // Create JSON array for database
        using json = nlohmann::json;
        json assetDB = json::object();
        json assetList = json::array();
        std::set<std::string> assetStrings;

        // Iterate over assets and dump ones that can be dumped
        for (uint32_t i = 0; i < pak.GetNumAssets(); i++)
        {
            json assetInfo;
            auto assetDef = pak.GetAssetDefinition(i);
            assetInfo["hash"] = Util::HashToString(assetDef->Hash);
            const char* typeStr = reinterpret_cast<const char*>(&assetDef->Type);
            assetInfo["type"] = std::string(typeStr, strnlen(typeStr, 4));
            auto asset = pak.GetAsset(i);
            if (asset)
            {
                if (asset->HasEmbeddedName())
                {
                    assetInfo["name"] = asset->GetEmbeddedName();
                }

                if (asset->CanDump())
                {
                    std::filesystem::path outputFile = params->OutputDir / asset->GetOutputFilePath();
                    std::filesystem::path outputFileDir = outputFile;
                    outputFileDir.remove_filename();
                    std::filesystem::create_directories(outputFileDir);
                    auto thisAssetStrings = asset->Dump(outputFile);
                    assetStrings.merge(thisAssetStrings);
                    assetInfo["dump_path"] = asset->GetOutputFilePath().string();
                }
            }
            assetList.push_back(assetInfo);
        }

        assetDB["strings"] = assetStrings;
        assetDB["assets"] = assetList;

        // Write out the asset database
        std::filesystem::path dbFile = std::filesystem::path(params->OutputDir) / (params->RPakName + ".json");
        std::ofstream output(dbFile);
        output << std::setw(2) << assetDB << std::endl;

        logger->info("Extraction complete!");
    });
}

struct PostProcessParams
{
    std::string BinDir;
    std::string InputDir;
    std::string OutputDir = "extracted";
    std::string RPakName;
};

std::optional<std::ifstream> DumpedFileReaderFactory(const std::string& outputDir, std::map<uint64_t, std::string>& assetMap, uint64_t hash)
{
    if (assetMap.find(hash) != assetMap.end())
    {
        std::ifstream f(std::filesystem::path(outputDir) / assetMap[hash]);
        if (f.is_open())
        {
            return f;
        }
    }

    return {};
}

void AddPostProcessCommand(CLI::App& app)
{
    CLI::App* command = app.add_subcommand("postprocess", "Post-process assets for an RPak file");

    auto params = std::make_shared<ExtractParams>();
    command->add_option("-b,--bindir", params->BinDir, "Path to x64_retail in your Titanfall 2 folder")
        ->required();
    command->add_option("-i,--inputdir", params->InputDir, "Path to folder containing rpak files")
        ->required();
    command->add_option("-o,--outputdir", params->OutputDir, "Path to folder to read and write post-processed files", true);
    command->add_flag("-v", VerbosityCallback, "Verbose output (-vv for very verbose)");
    command->add_option("rpak_name", params->RPakName, "Name of RPak file to extract (e.g. sp_training)")
        ->required();

    command->callback([params]() {
        using json = nlohmann::json;
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

        InitializeFupa(params->BinDir);

        // Read all the asset database json files to create map of asset hash => dump file
        std::map<uint64_t, std::string> dumpedFilesMap;
        for (auto& p : std::filesystem::directory_iterator(params->OutputDir))
        {
            if (p.path().extension() == ".json")
            {
                logger->info("Loading asset database {}", p.path().string());

                json db;
                std::ifstream f(p.path());
                f >> db;

                for (const auto& obj : db["assets"])
                {
                    if (obj.find("dump_path") != obj.end())
                    {
                        std::string hash = obj["hash"];
                        dumpedFilesMap[strtoull(hash.c_str(), nullptr, 16)] = obj["dump_path"];
                    }
                }
            }
        }

        // Read the current RPak's database
        json thisRPakDB;
        std::ifstream f(std::filesystem::path(params->OutputDir) / (params->RPakName + ".json"));
        f >> thisRPakDB;

        // Create file opener
        using namespace std::placeholders;
        auto opener = std::bind(FileReaderFactory, params->InputDir, _1, _2);

        // Load the rpak
        RPakFile pak(params->RPakName, GetLatestRPakNumber(opener, params->RPakName), opener);
        pak.Load();

        // Iterate over asset defs and dump those which support post-processing dumps
        auto dumpedOpener = std::bind(DumpedFileReaderFactory, params->OutputDir, dumpedFilesMap, _1);
        for (uint32_t i = 0; i < pak.GetNumAssets(); i++)
        {
            auto asset = pak.GetAsset(i);
            if (asset && asset->CanDumpPost())
            {
                std::filesystem::path outputFile = params->OutputDir / asset->GetOutputFilePath();
                std::filesystem::path outputFileDir = outputFile;
                outputFileDir.remove_filename();
                std::filesystem::create_directories(outputFileDir);
                auto thisAssetStrings = asset->DumpPost(dumpedOpener, outputFile);
                // TODO: Add asset strings. Update dump path.
                //assetStrings.merge(thisAssetStrings);
                //assetInfo["dump_path"] = asset->GetOutputFilePath().string();
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
    AddPostProcessCommand(app);

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
