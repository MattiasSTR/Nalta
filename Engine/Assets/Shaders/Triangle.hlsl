struct VSInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 color    : COLOR;
};

VSOutput VSMain(VSInput aInput)
{
    VSOutput output;
    output.position = float4(aInput.position, 1.0f);
    output.color    = aInput.color;
    return output;
}

float4 PSMain(VSOutput aInput) : SV_Target
{
    return float4(aInput.color, 1.0f);
}