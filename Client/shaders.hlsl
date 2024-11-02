// ��� ���� ����
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

// �ؽ�ó�� ���÷� ����
Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD; // �ؽ�ó ��ǥ �߰�
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float3 worldPos : POSITION;
    float2 texCoord : TEXCOORD; // �ؽ�ó ��ǥ �߰�
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    
    // ��ġ ��ȯ
    float4 worldPosition = mul(float4(input.position, 1.0f), worldMatrix);
    result.worldPos = worldPosition.xyz;
    float4 viewPosition = mul(worldPosition, viewMatrix);
    result.position = mul(viewPosition, projectionMatrix);
    
    // �븻 ���͸� ���� �������� ��ȯ
    result.normal = normalize(mul(input.normal, (float3x3) worldMatrix));
    
    result.color = input.color;
    result.texCoord = input.texCoord; // �ؽ�ó ��ǥ ����
    
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float3 toEye = normalize(eyePosition.xyz - input.worldPos);
    float3 lightDir = normalize(-lightDirection.xyz);
    
    // �ֺ���
    float3 ambient = ambientColor.rgb * ambientColor.a;
    
    // Ȯ�걤
    float diffuseFactor = max(dot(lightDir, normal), 0.0f);
    float3 diffuse = lightColor.rgb * lightColor.a * diffuseFactor;
    
    // �ݻ籤
    float3 reflection = reflect(-lightDir, normal);
    float specularFactor = pow(max(dot(reflection, toEye), 0.0f), 32.0f);
    float3 specular = lightColor.rgb * lightColor.a * specularFactor * 0.5f;
    
    // �ؽ�ó ���� ���ø�
    float4 texColor = g_texture.Sample(g_sampler, input.texCoord);
    
    // ���� ���� ��� (�ؽ�ó ����� �������� ����)
    float3 finalColor = texColor.rgb * (ambient + diffuse) + specular;
    
    return float4(finalColor, texColor.a);
}