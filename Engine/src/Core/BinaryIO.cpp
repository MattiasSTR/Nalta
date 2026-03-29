#include "npch.h"
#include "Nalta/Core/BinaryIO.h"

#include <fstream>

namespace Nalta
{
    bool BinaryWriter::SaveToFile(const std::filesystem::path& aPath) const
    {
        std::filesystem::create_directories(aPath.parent_path());

        std::ofstream file{ aPath, std::ios::binary };
        if (!file.is_open())
        {
            NL_ERROR(GCoreLogger, "BinaryWriter: failed to open '{}' for writing", aPath.string());
            return false;
        }

        file.write(reinterpret_cast<const char*>(myBuffer.data()), static_cast<std::streamsize>(myBuffer.size()));

        return file.good();
    }

    BinaryReader BinaryReader::FromFile(const std::filesystem::path& aPath)
    {
        std::ifstream file{ aPath, std::ios::binary | std::ios::ate };
        if (!file.is_open())
        {
            NL_ERROR(GCoreLogger, "BinaryReader: failed to open '{}'", aPath.string());
            return BinaryReader({});
        }

        const std::streamsize size{ file.tellg() };
        file.seekg(0);

        std::vector<uint8_t> buffer(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(buffer.data()), size);

        return BinaryReader(std::move(buffer));
    }
}
