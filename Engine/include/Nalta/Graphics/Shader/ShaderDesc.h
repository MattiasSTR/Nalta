#pragma once
#include "ShaderStage.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Nalta::Graphics
{
    struct ShaderStageDesc
    {
        ShaderStage stage;
        std::string entryPoint;
    };

    struct ShaderDesc
    {
        std::filesystem::path filePath;
        std::vector<ShaderStageDesc> stages;
        std::unordered_map<std::string, std::string> defines;
    };
}
