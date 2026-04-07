#pragma once
#include "Nalta/RHI/RHIResources.h"
#include "Nalta/RHI/Types/RHIShader.h"

#include <filesystem>
#include <optional>

namespace Nalta::Graphics
{
    class ShaderBlobStore
    {
    public:
        // Returns nullopt if cache is missing or stale
        [[nodiscard]] static std::optional<RHI::Shader> Load(const std::filesystem::path& aCachePath, const std::filesystem::path& aSourcePath);

        static void Save(const std::filesystem::path& aCachePath, const RHI::Shader& aShader, const RHI::ShaderDesc& aDesc);

    private:
        static constexpr char MAGIC[4]{ 'N', 'S', 'H', 'D' };
        static constexpr uint32_t VERSION{ 1 };
    };
}