#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace Nalta
{
    class BinaryWriter
    {
    public:
        template<typename T>
        void Write(const T& aValue)
        {
            const auto* bytes{ reinterpret_cast<const uint8_t*>(&aValue) };
            myBuffer.insert(myBuffer.end(), bytes, bytes + sizeof(T));
        }

        void WriteBytes(std::span<const uint8_t> aData)
        {
            myBuffer.insert(myBuffer.end(), aData.begin(), aData.end());
        }

        void WriteString(const std::string& aStr)
        {
            const uint32_t len{ static_cast<uint32_t>(aStr.size()) };
            Write(len);
            myBuffer.insert(myBuffer.end(), aStr.begin(), aStr.end());
        }

        [[nodiscard]] bool SaveToFile(const std::filesystem::path& aPath) const;

    private:
        std::vector<uint8_t> myBuffer;
    };
    
    class BinaryReader
    {
    public:
        explicit BinaryReader(std::vector<uint8_t> aBuffer) : myBuffer(std::move(aBuffer)) {}
        
        [[nodiscard]] size_t GetOffset() const { return myOffset; }

        template<typename T>
        T Read()
        {
            N_CORE_ASSERT(myOffset + sizeof(T) <= myBuffer.size(),"BinaryReader: read out of bounds");
            T value;
            std::memcpy(&value, myBuffer.data() + myOffset, sizeof(T));
            myOffset += sizeof(T);
            return value;
        }

        void ReadBytes(std::span<uint8_t> aOut)
        {
            N_CORE_ASSERT(myOffset + aOut.size() <= myBuffer.size(), "BinaryReader: read out of bounds");
            std::memcpy(aOut.data(), myBuffer.data() + myOffset, aOut.size());
            myOffset += aOut.size();
        }

        [[nodiscard]] std::string ReadString()
        {
            const uint32_t len{ Read<uint32_t>() };
            N_CORE_ASSERT(myOffset + len <= myBuffer.size(), "BinaryReader: string out of bounds");
            std::string str(myBuffer.begin() + myOffset, myBuffer.begin() + myOffset + len);
            myOffset += len;
            return str;
        }

        [[nodiscard]] bool IsValid() const { return !myBuffer.empty(); }
        [[nodiscard]] bool IsAtEnd() const { return myOffset >= myBuffer.size(); }

        [[nodiscard]] static BinaryReader FromFile(const std::filesystem::path& aPath);

    private:
        std::vector<uint8_t> myBuffer;
        size_t myOffset{ 0 };
    };
}