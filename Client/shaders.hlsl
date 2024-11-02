// ��� ���� ����
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
    float3 normal : NORMAL; // ������ ������ ����� ���� �߰�
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL; // ������ ������ ����� ���� �߰�
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    
    // World-View-Projection ��ȯ ����
    float4 worldPosition = mul(float4(input.position, 1.0f), worldMatrix);
    float4 viewPosition = mul(worldPosition, viewMatrix);
    result.position = mul(viewPosition, projectionMatrix);
    
    // �븻 ���͸� ���� �������� ��ȯ (������ ������ ����� ����)
    result.normal = mul(input.normal, (float3x3) worldMatrix);
    
    result.color = input.color;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}