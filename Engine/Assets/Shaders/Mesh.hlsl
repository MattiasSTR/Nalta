struct TransformData
{
    float4x4 model;
    float4x4 viewProjection;
};

ConstantBuffer<TransformData> gTransform : register(b0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD0;
};

VSOutput VSMain(VSInput aInput)
{
    VSOutput output;
    float4 worldPos = mul(float4(aInput.position, 1.0f), gTransform.model);
    output.position = mul(worldPos, gTransform.viewProjection);
    output.normal   = mul(float4(aInput.normal, 0.0f), gTransform.model).xyz;
    output.texCoord = aInput.texCoord;
    return output;
}

float4 PSMain(VSOutput aInput) : SV_Target
{
    float3 lightDir = normalize(float3(0.5f, 1.0f, 0.3f));
    float3 normal = normalize(aInput.normal);
    float diffuse = max(dot(normal, lightDir), 0.0f);
    float3 color = diffuse * float3(1.0f, 1.0f, 1.0f) + float3(0.1f, 0.1f, 0.1f);
    return float4(color, 1.0f);
}