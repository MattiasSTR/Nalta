#include "npch.h"
#include "Nalta/Graphics/Shader/ShaderCompiler.h"

#include <objbase.h>
#include <objidl.h>
#include <dxcapi.h>
#include <wrl/client.h>

#pragma comment(lib, "dxcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace Nalta::Graphics
{
    namespace
    {
        const wchar_t* GetTargetProfile(const ShaderStage aStage)
        {
            switch (aStage)
            {
                case ShaderStage::Vertex:  return L"vs_6_6";
                case ShaderStage::Pixel:   return L"ps_6_6";
                case ShaderStage::Compute: return L"cs_6_6";
                default:
                {
                    N_CORE_ASSERT(false, "ShaderCompiler: unknown shader stage");
                    return L"";
                }
            }
        }

        std::wstring ToWide(const std::string& aStr)
        {
            return std::wstring(aStr.begin(), aStr.end());
        }
    }
    
    struct ShaderCompiler::Impl
    {
        ComPtr<IDxcUtils>          utils;
        ComPtr<IDxcCompiler3>      compiler;
        ComPtr<IDxcIncludeHandler> defaultIncludeHandler;
    };
    
    ShaderCompiler::ShaderCompiler() = default;
    ShaderCompiler::~ShaderCompiler() = default;
    
    void ShaderCompiler::Initialize()
    {
        NL_SCOPE_CORE("ShaderCompiler");
        myImpl = std::make_unique<Impl>();

        if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&myImpl->utils))))
        {
            NL_FATAL(GCoreLogger, "failed to create DXC utils");
        }

        if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&myImpl->compiler))))
        {
            NL_FATAL(GCoreLogger, "failed to create DXC compiler");
        }

        if (FAILED(myImpl->utils->CreateDefaultIncludeHandler(&myImpl->defaultIncludeHandler)))
        {
            NL_FATAL(GCoreLogger, "failed to create include handler");
        }

        NL_INFO(GCoreLogger, "initialized");
    }
    
    void ShaderCompiler::Shutdown()
    {
        NL_SCOPE_CORE("ShaderCompiler");
        
        myCache.clear();

        myImpl->defaultIncludeHandler.Reset();
        myImpl->compiler.Reset();
        myImpl->utils.Reset();

        myImpl.reset();
        NL_INFO(GCoreLogger, "shutdown");
    }
    
    std::shared_ptr<Shader> ShaderCompiler::Compile(const ShaderDesc& aDesc)
    {
        NL_SCOPE_CORE("ShaderCompiler");
        
        const std::string key{ BuildCacheKey(aDesc) };

        if (const auto it{ myCache.find(key) }; it != myCache.end())
        {
            NL_TRACE(GCoreLogger, "cache hit for '{}'", aDesc.filePath.string());
            return it->second;
        }

        auto shader{ CompileInternal(aDesc) };
        if (shader && shader->HasStage(aDesc.stages.front().stage))
        {
            myCache[key] = shader;
            NL_INFO(GCoreLogger, "compiled '{}'", aDesc.filePath.string());
        }

        return shader;
    }
    
    std::shared_ptr<Shader> ShaderCompiler::Recompile(const ShaderDesc& aDesc)
    {
        InvalidateCache(aDesc.filePath);
        return Compile(aDesc);
    }

    void ShaderCompiler::InvalidateCache(const std::filesystem::path& aPath)
    {
        NL_SCOPE_CORE("ShaderCompiler");
        
        const std::string pathStr{ aPath.string() };
        std::erase_if(myCache, [&](const auto& aEntry)
        {
            return aEntry.first.starts_with(pathStr);
        });

        NL_TRACE(GCoreLogger, "invalidated cache for '{}'", pathStr);
    }
    
    std::shared_ptr<Shader> ShaderCompiler::CompileInternal(const ShaderDesc& aDesc) const
    {
        N_CORE_ASSERT(std::filesystem::exists(aDesc.filePath), "file not found '{}'", aDesc.filePath.string().c_str());

        auto shader{ std::make_shared<Shader>() };

        for (const auto& stageDesc : aDesc.stages)
        {
            if (!CompileStage(stageDesc, aDesc, *shader))
            {
                NL_ERROR(GCoreLogger, "failed to compile stage '{}' in '{}'", stageDesc.entryPoint, aDesc.filePath.string());
                return nullptr;
            }
        }

        return shader;
    }

    bool ShaderCompiler::CompileStage(const ShaderStageDesc& aStage, const ShaderDesc& aDesc, Shader& aOutShader) const
    {
        ComPtr<IDxcBlobEncoding> sourceBlob;
        if (FAILED(myImpl->utils->LoadFile(
            aDesc.filePath.wstring().c_str(), nullptr, &sourceBlob)))
        {
            NL_ERROR(GCoreLogger, "failed to load '{}'",
                aDesc.filePath.string());
            return false;
        }

        const DxcBuffer sourceBuffer
        {
            .Ptr      = sourceBlob->GetBufferPointer(),
            .Size     = sourceBlob->GetBufferSize(),
            .Encoding = DXC_CP_ACP
        };

        std::vector<LPCWSTR> args;
        
        args.push_back(L"-Zpr"); // Pack matrices in row-major order

        const std::wstring fileName{ aDesc.filePath.wstring() };
        args.push_back(fileName.c_str());

        args.push_back(L"-E");
        const std::wstring entryPoint{ ToWide(aStage.entryPoint) };
        args.push_back(entryPoint.c_str());

        args.push_back(L"-T");
        args.push_back(GetTargetProfile(aStage.stage));

        args.push_back(L"-I");
        const std::wstring includeDir{ aDesc.filePath.parent_path().wstring() };
        args.push_back(includeDir.c_str());

#ifndef N_SHIPPING
        args.push_back(L"-Zi");
        args.push_back(L"-Qembed_debug");
        args.push_back(L"-Od");
#else
        args.push_back(L"-O3");
#endif

        std::vector<std::wstring> defineStrings;
        for (const auto& [name, value] : aDesc.defines)
        {
            defineStrings.push_back(ToWide(name + "=" + value));
            args.push_back(L"-D");
            args.push_back(defineStrings.back().c_str());
        }

        ComPtr<IDxcResult> result;
        if (FAILED(myImpl->compiler->Compile(
            &sourceBuffer,
            args.data(),
            static_cast<UINT32>(args.size()),
            myImpl->defaultIncludeHandler.Get(),
            IID_PPV_ARGS(&result))))
        {
            NL_ERROR(GCoreLogger, "compile call failed for '{}'", aDesc.filePath.string());
            return false;
        }

        ComPtr<IDxcBlobUtf8> errors;
        result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        if (errors && errors->GetStringLength() > 0)
        {
            NL_ERROR(GCoreLogger, "'{}' ({}):\n{}",
                aDesc.filePath.string(),
                aStage.entryPoint,
                errors->GetStringPointer());
        }

        HRESULT status{};
        result->GetStatus(&status);
        if (FAILED(status))
        {
            return false;
        }

        ComPtr<IDxcBlob> bytecodeBlob;
        result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&bytecodeBlob), nullptr);
        if ((bytecodeBlob == nullptr) || bytecodeBlob->GetBufferSize() == 0)
        {
            NL_ERROR(GCoreLogger, "empty bytecode for '{}'", aDesc.filePath.string());
            return false;
        }
        
        ComPtr<IDxcBlob> reflectionBlob;
        result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr);

        ShaderBytecode bytecode;
        const auto* codeData{ static_cast<const uint8_t*>(bytecodeBlob->GetBufferPointer()) };
        bytecode.code.assign(codeData, codeData + bytecodeBlob->GetBufferSize());

        if ((reflectionBlob != nullptr) && reflectionBlob->GetBufferSize() > 0)
        {
            const auto* reflData{ static_cast<const uint8_t*>(reflectionBlob->GetBufferPointer()) };
            bytecode.reflection.assign(reflData, reflData + reflectionBlob->GetBufferSize());
            NL_TRACE(GCoreLogger, "reflection data captured for '{}'", aStage.entryPoint);
        }
        else
        {
            NL_WARN(GCoreLogger, "no reflection data for '{}'", aStage.entryPoint);
        }

        aOutShader.SetBytecode(aStage.stage, std::move(bytecode));
        return true;
    }

    std::string ShaderCompiler::BuildCacheKey(const ShaderDesc& aDesc)
    {
        std::string key{ aDesc.filePath.string() };
        for (const auto& stage : aDesc.stages)
        {
            key += "|" + stage.entryPoint;
        }
        for (const auto& [name, value] : aDesc.defines)
        {
            key += "|" + name + "=" + value;
        }
        return key;
    }
}