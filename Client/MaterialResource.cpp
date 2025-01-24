#include "pch.h"
#include "Engine.h"
#include "MaterialResource.h"
#include "ResourceManager.h"
#include "Logger.h"

namespace Resource
{
    MaterialResource::~MaterialResource()
    {
        Unload();
    }

    bool MaterialResource::Load(const std::string& path)
    {
        m_path = path;
        SetState(ResourceState::Loading);

        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                SetError("머티리얼 파일을 열 수 없음: " + path);
                return false;
            }

            json materialData = json::parse(file);
            auto& resourceManager = Resource::ResourceManager::Instance();

            // 셰이더 로드
            if (materialData.contains("shaders")) {
                auto& shaders = materialData["shaders"];
                std::string vsPath = shaders["vertex"].get<std::string>();
                std::string psPath = shaders["pixel"].get<std::string>();

                m_vertexShader = resourceManager.LoadShader(vsPath, ShaderType::Vertex);
                m_pixelShader = resourceManager.LoadShader(psPath, ShaderType::Pixel);

                if (!m_vertexShader || !m_pixelShader) {
                    SetError("셰이더 로드 실패");
                    return false;
                }
            }

            // 텍스처 로드
            if (materialData.contains("textures")) {
                auto& textures = materialData["textures"];
                for (auto& [paramName, texPath] : textures.items()) {
                    auto texture = resourceManager.LoadTexture(texPath.get<std::string>());
                    if (texture) {
                        m_textures[paramName] = texture;
                    }
                    else {
                        Logger::Instance().Warning("텍스처 로드 실패: {}", texPath.get<std::string>());
                    }
                }
            }

            // 파라미터 정의 및 상수 버퍼 생성
            if (materialData.contains("parameters")) {
                auto& params = materialData["parameters"];
                // 먼저 모든 파라미터 정의
                for (auto& [paramName, paramData] : params.items()) {
                    std::string typeStr = paramData["type"].get<std::string>();
                    MaterialParameterType type = ParseParameterType(typeStr);
                    DefineParameter(paramName, type);
                }
                // 상수 버퍼 생성
                if (!CreateConstantBuffer()) {
                    SetError("상수 버퍼 생성 실패");
                    return false;
                }
                // 그 다음 기본값 설정
                for (auto& [paramName, paramData] : params.items()) {
                    if (paramData.contains("value")) {
                        SetParameterFromJson(paramName, paramData["value"]);
                    }
                }
            }

            // 파이프라인 상태 설정
            if (materialData.contains("renderState")) {
                LoadPipelineSettings(materialData["renderState"]);
            }

            SetState(ResourceState::Loaded);
            Logger::Instance().Info("머티리얼 로드 성공: {}", path);
            return true;
        }
        catch (const json::exception& e) {
            SetError(std::string("JSON 파싱 에러: ") + e.what());
            return false;
        }
        catch (const std::exception& e) {
            SetError(std::string("머티리얼 로드 에러: ") + e.what());
            return false;
        }
    }

    void MaterialResource::Unload()
    {
        m_vertexShader.reset();
        m_pixelShader.reset();
        m_textures.clear();

        SetState(ResourceState::Unloaded);
        Logger::Instance().Debug("머티리얼 언로드: {}", m_path);
    }

    void MaterialResource::SetShaders(
        std::shared_ptr<ShaderResource> vertex,
        std::shared_ptr<ShaderResource> pixel)
    {
        m_vertexShader = vertex;
        m_pixelShader = pixel;

        Logger::Instance().Debug("머티리얼 셰이더 설정: {}", m_path);
    }

    void MaterialResource::SetTexture(
        const std::string& paramName,
        std::shared_ptr<TextureResource> texture)
    {
        m_textures[paramName] = texture;

        Logger::Instance().Debug("머티리얼 텍스처 설정: {} - {}", m_path, paramName);
    }

    TextureResource* MaterialResource::GetTexture(const std::string& paramName) const
    {
        auto it = m_textures.find(paramName);
        if (it != m_textures.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    void MaterialResource::DefineParameter(const std::string& name, MaterialParameterType type)
    {
        size_t alignment = MaterialParameter::GetAlignment(type);
        size_t size = MaterialParameter::GetSize(type);

        // 정렬된 오프셋 계산
        m_totalParamSize = (m_totalParamSize + alignment - 1) & ~(alignment - 1);

        MaterialParameterInfo info{};
        info.type = type;
        info.offset = m_totalParamSize;
        info.size = size;

        m_parameters[name] = info;
        m_totalParamSize += size;

        Logger::Instance().Debug("머티리얼 파라미터 정의: {} (타입: {}, 오프셋: {}, 크기: {})",
            name, static_cast<int>(type), info.offset, info.size);
    }

    bool MaterialResource::SetParameterData(const std::string& name, const void* data)
    {
        auto it = m_parameters.find(name);
        if (it == m_parameters.end()) {
            Logger::Instance().Error("머티리얼 파라미터 없음: {}", name);
            return false;
        }

        if (!m_mappedData) {
            Logger::Instance().Error("상수 버퍼가 매핑되지 않음");
            return false;
        }

        const auto& info = it->second;
        memcpy(m_mappedData + info.offset, data, info.size);
        m_parametersDirty = true;
        return true;
    }

    const MaterialParameterInfo* MaterialResource::GetParameterInfo(const std::string& name) const
    {
		auto it = m_parameters.find(name);
		if (it != m_parameters.end()) {
			return &it->second;
		}

		Logger::Instance().Error("머티리얼 파라미터를 찾을 수 없음: {}", name);
		return nullptr;
    }

    bool MaterialResource::CreateConstantBuffer()
    {
        if (m_totalParamSize == 0) {
            return true;  // 파라미터가 없는 경우
        }

        // 상수 버퍼는 256바이트 정렬 필요
        const size_t alignedSize = (m_totalParamSize + 255) & ~255;

        auto device = Engine::Instance().GetDevice();
        if (!device) return false;

        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(alignedSize);

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constantBuffer));

        if (FAILED(hr)) {
            Logger::Instance().Error("상수 버퍼 생성 실패");
            return false;
        }

        CD3DX12_RANGE readRange(0, 0);  // CPU에서는 읽지 않음
        hr = m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedData));
        if (FAILED(hr)) {
            Logger::Instance().Error("상수 버퍼 매핑 실패");
            return false;
        }

        return true;
    }

    void MaterialResource::UpdateConstantBuffer()
    {
        if (!m_parametersDirty || !m_constantBuffer) return;
        m_parametersDirty = false;
    }

    void MaterialResource::SetPipelineSettings(const PipelineSettings& settings)
    {
        m_pipelineSettings = settings;
        m_pipelineState.Reset();  // PSO 재생성을 위해 기존 것을 해제

        Logger::Instance().Debug("머티리얼 파이프라인 설정 업데이트: {}", m_path);
    }

    ID3D12PipelineState* MaterialResource::GetPipelineState()
    {
        if (!m_pipelineState && !CreatePipelineState()) {
            Logger::Instance().Error("파이프라인 상태 객체 생성 실패: {}", m_path);
            return nullptr;
        }
        return m_pipelineState.Get();
    }

    bool MaterialResource::CreatePipelineState()
    {
        if (!m_vertexShader || !m_pixelShader) {
            Logger::Instance().Error("PSO 생성 실패: 셰이더가 설정되지 않음");
            return false;
        }

        auto device = Engine::Instance().GetDevice();
        auto rootSignature = Engine::Instance().GetRootSignature();
        if (!device || !rootSignature) {
            return false;
        }

        // 입력 레이아웃 정의
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 40,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 52,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = rootSignature;
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(m_vertexShader->GetShaderBlob());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(m_pixelShader->GetShaderBlob());
        psoDesc.RasterizerState = m_pipelineSettings.rasterizer;
        psoDesc.BlendState = m_pipelineSettings.blend;
        psoDesc.DepthStencilState = m_pipelineSettings.depthStencil;
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = m_pipelineSettings.primitiveType;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc,
            IID_PPV_ARGS(&m_pipelineState));

        if (FAILED(hr)) {
            Logger::Instance().Error("PSO 생성 실패: HRESULT = 0x{:X}",
                static_cast<unsigned int>(hr));
            return false;
        }

        Logger::Instance().Debug("PSO 생성 성공: {}", m_path);
        return true;
    }

    size_t MaterialResource::CalculatePipelineStateHash()
    {
        std::size_t hash = 0;

        // 해시 조합을 위한 헬퍼 함수
        auto hashCombine = [](size_t& seed, size_t value) {
            seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };

        // 래스터라이저 상태 해시
        hashCombine(hash, m_pipelineSettings.rasterizer.FillMode);
        hashCombine(hash, m_pipelineSettings.rasterizer.CullMode);
        hashCombine(hash, m_pipelineSettings.rasterizer.FrontCounterClockwise);
        hashCombine(hash, m_pipelineSettings.rasterizer.DepthBias);
        hashCombine(hash, *reinterpret_cast<const size_t*>(&m_pipelineSettings.rasterizer.DepthBiasClamp));
        hashCombine(hash, *reinterpret_cast<const size_t*>(&m_pipelineSettings.rasterizer.SlopeScaledDepthBias));
        hashCombine(hash, m_pipelineSettings.rasterizer.DepthClipEnable);
        hashCombine(hash, m_pipelineSettings.rasterizer.MultisampleEnable);
        hashCombine(hash, m_pipelineSettings.rasterizer.AntialiasedLineEnable);
        hashCombine(hash, m_pipelineSettings.rasterizer.ForcedSampleCount);
        hashCombine(hash, m_pipelineSettings.rasterizer.ConservativeRaster);

        // 블렌드 상태 해시
        hashCombine(hash, m_pipelineSettings.blend.AlphaToCoverageEnable);
        hashCombine(hash, m_pipelineSettings.blend.IndependentBlendEnable);
        for (int i = 0; i < 8; ++i) {
            const auto& rt = m_pipelineSettings.blend.RenderTarget[i];
            hashCombine(hash, rt.BlendEnable);
            hashCombine(hash, rt.LogicOpEnable);
            hashCombine(hash, rt.SrcBlend);
            hashCombine(hash, rt.DestBlend);
            hashCombine(hash, rt.BlendOp);
            hashCombine(hash, rt.SrcBlendAlpha);
            hashCombine(hash, rt.DestBlendAlpha);
            hashCombine(hash, rt.BlendOpAlpha);
            hashCombine(hash, rt.LogicOp);
            hashCombine(hash, rt.RenderTargetWriteMask);
        }

        // 깊이-스텐실 상태 해시
        hashCombine(hash, m_pipelineSettings.depthStencil.DepthEnable);
        hashCombine(hash, m_pipelineSettings.depthStencil.DepthWriteMask);
        hashCombine(hash, m_pipelineSettings.depthStencil.DepthFunc);
        hashCombine(hash, m_pipelineSettings.depthStencil.StencilEnable);
        hashCombine(hash, m_pipelineSettings.depthStencil.StencilReadMask);
        hashCombine(hash, m_pipelineSettings.depthStencil.StencilWriteMask);
        hashCombine(hash, m_pipelineSettings.depthStencil.FrontFace.StencilFailOp);
        hashCombine(hash, m_pipelineSettings.depthStencil.FrontFace.StencilDepthFailOp);
        hashCombine(hash, m_pipelineSettings.depthStencil.FrontFace.StencilPassOp);
        hashCombine(hash, m_pipelineSettings.depthStencil.FrontFace.StencilFunc);
        hashCombine(hash, m_pipelineSettings.depthStencil.BackFace.StencilFailOp);
        hashCombine(hash, m_pipelineSettings.depthStencil.BackFace.StencilDepthFailOp);
        hashCombine(hash, m_pipelineSettings.depthStencil.BackFace.StencilPassOp);
        hashCombine(hash, m_pipelineSettings.depthStencil.BackFace.StencilFunc);

        // 프리미티브 타입 해시
        hashCombine(hash, m_pipelineSettings.primitiveType);

        // 셰이더 포인터도 해시에 포함
        hashCombine(hash, reinterpret_cast<size_t>(m_vertexShader.get()));
        hashCombine(hash, reinterpret_cast<size_t>(m_pixelShader.get()));

        return hash;
    }

    MaterialParameterType MaterialResource::ParseParameterType(const std::string& typeStr)
    {
        static const std::unordered_map<std::string, MaterialParameterType> typeMap = {
           {"float", MaterialParameterType::Float},
           {"float2", MaterialParameterType::Float2},
           {"float3", MaterialParameterType::Float3},
           {"float4", MaterialParameterType::Float4},
           {"matrix", MaterialParameterType::Matrix4x4},
           {"int", MaterialParameterType::Int},
           {"bool", MaterialParameterType::Bool}
        };

        auto it = typeMap.find(typeStr);
        if (it == typeMap.end()) {
            throw std::runtime_error("알 수 없는 파라미터 타입: " + typeStr);
        }
        return it->second;
    }

    void MaterialResource::SetParameterFromJson(const std::string& name, const json& value)
    {
        const MaterialParameterInfo* info = GetParameterInfo(name);
        if (!info) {
            throw std::runtime_error("파라미터를 찾을 수 없음: " + name);
        }

        switch (info->type) {
        case MaterialParameterType::Float: {
            float data = value.get<float>();
            SetParameterData(name, &data);
            break;
        }
        case MaterialParameterType::Float2: {
            XMFLOAT2 data;
            auto arr = value.get<std::vector<float>>();
            if (arr.size() != 2) throw std::runtime_error("Float2 타입은 2개의 값이 필요함");
            data.x = arr[0];
            data.y = arr[1];
            SetParameterData(name, &data);
            break;
        }
        case MaterialParameterType::Float3: {
            XMFLOAT3 data;
            auto arr = value.get<std::vector<float>>();
            if (arr.size() != 3) throw std::runtime_error("Float3 타입은 3개의 값이 필요함");
            data.x = arr[0];
            data.y = arr[1];
            data.z = arr[2];
            SetParameterData(name, &data);
            break;
        }
        case MaterialParameterType::Float4: {
            XMFLOAT4 data;
            auto arr = value.get<std::vector<float>>();
            if (arr.size() != 4) throw std::runtime_error("Float4 타입은 4개의 값이 필요함");
            data.x = arr[0];
            data.y = arr[1];
            data.z = arr[2];
            data.w = arr[3];
            SetParameterData(name, &data);
            break;
        }
        case MaterialParameterType::Int: {
            int data = value.get<int>();
            SetParameterData(name, &data);
            break;
        }
        case MaterialParameterType::Bool: {
            bool data = value.get<bool>();
            SetParameterData(name, &data);
            break;
        }
        default:
            throw std::runtime_error("지원하지 않는 파라미터 타입");
        }
    }

    void MaterialResource::LoadPipelineSettings(const json& renderState)
    {
        PipelineSettings settings;

        if (renderState.contains("cullMode")) {
            std::string cullMode = renderState["cullMode"].get<std::string>();
            if (cullMode == "none") {
                settings.rasterizer.CullMode = D3D12_CULL_MODE_NONE;
            }
            else if (cullMode == "front") {
                settings.rasterizer.CullMode = D3D12_CULL_MODE_FRONT;
            }
            else if (cullMode == "back") {
                settings.rasterizer.CullMode = D3D12_CULL_MODE_BACK;
            }
        }

        if (renderState.contains("fillMode")) {
            std::string fillMode = renderState["fillMode"].get<std::string>();
            if (fillMode == "wireframe") {
                settings.rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
            }
            else if (fillMode == "solid") {
                settings.rasterizer.FillMode = D3D12_FILL_MODE_SOLID;
            }
        }

        if (renderState.contains("blend")) {
            auto& blend = renderState["blend"];
            if (blend.contains("enable")) {
                settings.blend.RenderTarget[0].BlendEnable = blend["enable"].get<bool>();
            }
            if (blend.contains("srcBlend")) {
                settings.blend.RenderTarget[0].SrcBlend = ParseBlendFactor(blend["srcBlend"]);
            }
            if (blend.contains("destBlend")) {
                settings.blend.RenderTarget[0].DestBlend = ParseBlendFactor(blend["destBlend"]);
            }
        }

        SetPipelineSettings(settings);
    }

    D3D12_BLEND MaterialResource::ParseBlendFactor(const json& value)
    {
        std::string factor = value.get<std::string>();
        static const std::unordered_map<std::string, D3D12_BLEND> blendMap = {
            {"zero", D3D12_BLEND_ZERO},
            {"one", D3D12_BLEND_ONE},
            {"srcAlpha", D3D12_BLEND_SRC_ALPHA},
            {"invSrcAlpha", D3D12_BLEND_INV_SRC_ALPHA},
            {"destAlpha", D3D12_BLEND_DEST_ALPHA},
            {"invDestAlpha", D3D12_BLEND_INV_DEST_ALPHA}
        };

        auto it = blendMap.find(factor);
        if (it == blendMap.end()) {
            throw std::runtime_error("알 수 없는 블렌드 팩터: " + factor);
        }
        return it->second;
    }
    
}