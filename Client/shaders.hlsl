// 상수 버퍼 정의
cbuffer ObjectConstants : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
}

cbuffer LightConstants : register(b1)
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
    float4 eyePosition;
}

cbuffer MaterialConstants : register(b2)
{
    float4 baseColor;
    float4 materialParams; // x: metallic, y: roughness, z: ao, w: reserved
    float4 emissiveColor;
    uint useNormalMap;
    uint useMetallicMap;
    uint useRoughnessMap;
    uint useAOMap;
}

// 텍스처 및 샘플러 정의
Texture2D baseColorMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D metallicRoughnessMap : register(t2);
SamplerState defaultSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    
    // 위치 변환
    float4 worldPosition = mul(float4(input.position, 1.0f), worldMatrix);
    result.worldPos = worldPosition.xyz;
    float4 viewPosition = mul(worldPosition, viewMatrix);
    result.position = mul(viewPosition, projectionMatrix);
    
    // 노말과 탄젠트 변환
    result.normal = normalize(mul(input.normal, (float3x3) worldMatrix));
    result.tangent = normalize(mul(input.tangent, (float3x3) worldMatrix));
    
    result.color = input.color;
    result.texCoord = input.texCoord;
    
    return result;
}

// PBR 관련 유틸리티 함수들
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159 * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    // 텍스처에서 베이스 컬러 샘플링
    float4 texColor = baseColorMap.Sample(defaultSampler, input.texCoord);
    float3 albedo = texColor.rgb * baseColor.rgb;
    float alpha = texColor.a * baseColor.a;

    // 노말 계산
    float3 normal = normalize(input.normal);
    if (useNormalMap)
    {
        float3 normalFromMap = normalMap.Sample(defaultSampler, input.texCoord).xyz * 2.0 - 1.0;
        float3 bitangent = cross(input.normal, input.tangent);
        float3x3 TBN = float3x3(input.tangent, bitangent, input.normal);
        normal = normalize(mul(normalFromMap, TBN));
    }

    // 메탈릭/러프니스 값 가져오기
    float metallic = materialParams.x;
    float roughness = materialParams.y;
    if (useMetallicMap || useRoughnessMap)
    {
        float2 mrSample = metallicRoughnessMap.Sample(defaultSampler, input.texCoord).rg;
        metallic = useMetallicMap ? mrSample.r : metallic;
        roughness = useRoughnessMap ? mrSample.g : roughness;
    }

    // PBR 계산을 위한 기본 벡터들
    float3 N = normal;
    float3 V = normalize(eyePosition.xyz - input.worldPos);
    float3 L = normalize(-lightDirection.xyz);
    float3 H = normalize(V + L);
    
    // 프레넬 계산을 위한 기본 반사율
    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);

    // BRDF 계산
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    float3 specular = numerator / denominator;

    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    
    // 최종 색상 계산
    float3 color = (kD * albedo / 3.14159 + specular) * lightColor.rgb * lightColor.w * NdotL;
    color += albedo * ambientColor.rgb * ambientColor.w;
    color += emissiveColor.rgb * emissiveColor.w;

    return float4(color, alpha);
}