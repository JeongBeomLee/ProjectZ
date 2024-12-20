#pragma once
#include "IResource.h"
#include "pch.h"
#include "Logger.h"

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

        bool Load() override {
            if (IsReady()) return true;

            SetState(State::Loading);

            UINT compileFlags = 0;
            IFDEBUG(compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);

            ComPtr<ID3DBlob> errorBlob = nullptr;
            HRESULT hr = D3DCompileFromFile(
                std::wstring(GetPath().begin(), GetPath().end()).c_str(),
                nullptr,
                D3D_COMPILE_STANDARD_FILE_INCLUDE,
                m_entryPoint.c_str(),
                GetShaderTarget().c_str(),
                compileFlags,
                0,
                &m_shaderBlob,
                &errorBlob);

            if (FAILED(hr)) {
                SetState(State::Failed);
                std::string errorMsg = errorBlob ?
                    static_cast<const char*>(errorBlob->GetBufferPointer()) :
                    "Unknown error";
                Logger::Instance().Error("셰이더 컴파일 실패: {} - {}", GetPath(), errorMsg);
                return false;
            }

            SetState(State::Ready);
            SetSize(m_shaderBlob->GetBufferSize());

            Logger::Instance().Info("셰이더 로드 완료: {} ({}bytes)", GetName(), GetSize());
            return true;
        }

        void Unload() override {
            if (m_shaderBlob) {
                m_shaderBlob.Reset();
                SetState(State::Unloaded);
                Logger::Instance().Debug("셰이더 언로드: {}", GetName());
            }
        }

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