#pragma once
#include "IResource.h"

namespace Resource
{
    class ShaderResource : public IResource {
    public:
        enum class ShaderType {
            Vertex,
            Pixel,
            Compute,
            Geometry,
            Domain,
            Hull
        };

        ShaderResource(const std::string& path, ShaderType type, const std::string& entryPoint)
            : IResource(Type::Shader, path, std::filesystem::path(path).filename().string())
            , m_shaderType(type)
            , m_entryPoint(entryPoint)
            , m_shaderBlob(nullptr) {
            Logger::Instance().Debug("셰이더 리소스 생성: {} (타입: {})", GetName(),
                static_cast<int>(type));
        }

        ~ShaderResource() override {
            Unload();
        }

        bool Load() override;

        void Unload() override;

        ID3DBlob* GetShaderBlob() const { return m_shaderBlob.Get(); }
        const std::string& GetEntryPoint() const { return m_entryPoint; }
        ShaderType GetShaderType() const { return m_shaderType; }

    private:
        std::string GetShaderTarget() const {
            switch (m_shaderType) {
            case ShaderType::Vertex:   return "vs_5_0";
            case ShaderType::Pixel:    return "ps_5_0";
            case ShaderType::Compute:  return "cs_5_0";
            case ShaderType::Geometry: return "gs_5_0";
            case ShaderType::Domain:   return "ds_5_0";
            case ShaderType::Hull:     return "hs_5_0";
            default:
                throw std::runtime_error("Unknown shader type");
            }
        }

        ShaderType m_shaderType;
        std::string m_entryPoint;
        ComPtr<ID3DBlob> m_shaderBlob;
    };
}