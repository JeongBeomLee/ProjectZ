#pragma once

namespace Resource
{
    enum class MaterialParameterType {
        Float,
        Float2,
        Float3,
        Float4,
        Matrix4x4,
        Int,
        Bool
    };

    struct MaterialParameterInfo {
        MaterialParameterType type;
        size_t offset;
        size_t size;
    };

    class MaterialParameter {
    public:
        // HLSL 레지스터 정렬 규칙에 맞춰 각 타입의 크기 반환
        static size_t GetSize(MaterialParameterType type) {
            switch (type) {
            case MaterialParameterType::Float:    return 4;
            case MaterialParameterType::Float2:   return 8;
            case MaterialParameterType::Float3:   return 12;
            case MaterialParameterType::Float4:   return 16;
            case MaterialParameterType::Matrix4x4: return 64;
            case MaterialParameterType::Int:      return 4;
            case MaterialParameterType::Bool:     return 4;
            default: return 0;
            }
        }

        // HLSL 레지스터 정렬 규칙에 맞춰 각 타입의 정렬 요구사항 반환
        static size_t GetAlignment(MaterialParameterType type) {
            switch (type) {
            case MaterialParameterType::Float:    return 4;
            case MaterialParameterType::Float2:   return 8;
            case MaterialParameterType::Float3:   return 16; // float3는 float4로 정렬
            case MaterialParameterType::Float4:   return 16;
            case MaterialParameterType::Matrix4x4: return 16;
            case MaterialParameterType::Int:      return 4;
            case MaterialParameterType::Bool:     return 4;
            default: return 0;
            }
        }
    };
}