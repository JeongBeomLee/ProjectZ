// 상수 버퍼 정의
cbuffer ObjectConstants : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
}

cbuffer MaterialConstants : register(b1)
{
    float4 albedo;
    float4 emissive;
    float roughness;
    float metallic;
    float ambientOcclusion;
    float padding;
}

cbuffer LightConstants : register(b2)
{
    float4 lightDirection;
    float4 lightColor;
    float4 ambientColor;
    float4 eyePosition;
}

cbuffer TextureFlags : register(b3)
{
    bool hasAlbedoTexture;
    bool hasNormalTexture;
    bool hasMetallicRoughnessTexture;
    bool hasEmissiveTexture;
    bool hasOcclusionTexture;
}

// 텍스처가 없는 경우 사용할 기본값들
static const float4 DEFAULT_ALBEDO = float4(1, 1, 1, 1);
static const float4 DEFAULT_EMISSIVE = float4(0, 0, 0, 1);
static const float2 DEFAULT_METALLIC_ROUGHNESS = float2(0, 0.5); // x: metallic, y: roughness
static const float DEFAULT_OCCLUSION = 1.0;

// 텍스처와 샘플러 정의
Texture2D g_albedoTexture : register(t0);
Texture2D g_normalTexture : register(t1);
Texture2D g_metallicRoughnessTexture : register(t2);
Texture2D g_emissiveTexture : register(t3);
Texture2D g_occlusionTexture : register(t4);
SamplerState g_sampler : register(s0);

// 입력 구조체
struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD;
};

// 출력 구조체
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
    
    // 노말 벡터를 월드 공간으로 변환
    result.normal = normalize(mul(input.normal, (float3x3) worldMatrix));
    
    // 탄젠트 벡터를 월드 공간으로 변환
    result.tangent = normalize(mul(input.tangent, (float3x3) worldMatrix));
    
    result.color = input.color;
    result.texCoord = input.texCoord;
    
    return result;
}

// PBR 라이팅을 적용한 픽셀 셰이더
float4 PSMain(PSInput input) : SV_TARGET
{
    // 텍스처에서 머티리얼 속성 샘플링
    float4 albedoTexture = hasAlbedoTexture ? 
                           g_albedoTexture.Sample(g_sampler, input.texCoord) : DEFAULT_ALBEDO;
    float4 emissiveTexture = hasEmissiveTexture ?
                             g_emissiveTexture.Sample(g_sampler, input.texCoord) : DEFAULT_EMISSIVE;
    float4 metallicRoughnessTexture = hasMetallicRoughnessTexture ?
                                      g_metallicRoughnessTexture.Sample(g_sampler, input.texCoord) : float4(DEFAULT_METALLIC_ROUGHNESS, 0, 0);
    float occlusionTexture = hasOcclusionTexture ?
                             g_occlusionTexture.Sample(g_sampler, input.texCoord).r : DEFAULT_OCCLUSION;
    
    // 머티리얼 속성 계산
    float3 finalAlbedo = albedo.rgb * albedoTexture.rgb;
    float3 finalEmissive = emissive.rgb * emissiveTexture.rgb;
    float finalRoughness = roughness * metallicRoughnessTexture.g;
    float finalMetallic = metallic * metallicRoughnessTexture.b;
    float finalAO = ambientOcclusion * occlusionTexture;
    
    float3 normal = normalize(input.normal);
    float3 toEye = normalize(eyePosition.xyz - input.worldPos);
    float3 lightDir = normalize(-lightDirection.xyz);
    
    // 주변광
    float3 ambient = ambientColor.rgb * ambientColor.a * finalAlbedo * finalAO;
    
    // 확산광
    float diffuseFactor = max(dot(lightDir, normal), 0.0f);
    float3 diffuse = lightColor.rgb * lightColor.a * diffuseFactor * finalAlbedo;
    
    // 반사광 계산 개선 (러프니스 적용)
    float3 halfVec = normalize(lightDir + toEye);
    float NdotH = max(dot(normal, halfVec), 0.0f);
    float specPower = (1.0f - finalRoughness) * 256.0f;
    float specularFactor = pow(NdotH, specPower);
    float3 specular = lightColor.rgb * lightColor.a * specularFactor * lerp(0.04f, finalAlbedo, finalMetallic);
    
    // 최종 색상 계산
    float3 finalColor = ambient + diffuse + specular + finalEmissive;
    
    return float4(finalColor, albedoTexture.a);
}