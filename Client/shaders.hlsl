// 상수 버퍼 정의
cbuffer ObjectConstants : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
}

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL; // 나중의 라이팅 계산을 위해 추가
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL; // 나중의 라이팅 계산을 위해 추가
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    
    // World-View-Projection 변환 적용
    float4 worldPosition = mul(float4(input.position, 1.0f), worldMatrix);
    float4 viewPosition = mul(worldPosition, viewMatrix);
    result.position = mul(viewPosition, projectionMatrix);
    
    // 노말 벡터를 월드 공간으로 변환 (나중의 라이팅 계산을 위해)
    result.normal = mul(input.normal, (float3x3) worldMatrix);
    
    result.color = input.color;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}