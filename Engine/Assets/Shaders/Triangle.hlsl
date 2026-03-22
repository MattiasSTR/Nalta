struct VSOutput
{
    float4 position : SV_Position;
    float3 color    : COLOR;
};

VSOutput VSMain(uint aVertexID : SV_VertexID)
{
    static const float2 positions[3] =
    {
        float2( 0.0f,  0.5f),
        float2( 0.5f, -0.5f),
        float2(-0.5f, -0.5f)
    };

    static const float3 colors[3] =
    {
        float3(1.0f, 0.0f, 0.0f),
        float3(0.0f, 1.0f, 0.0f),
        float3(0.0f, 0.0f, 1.0f)
    };

    VSOutput output;
    output.position = float4(positions[aVertexID], 0.0f, 1.0f);
    output.color    = colors[aVertexID];
    return output;
}

float4 PSMain(VSOutput aInput) : SV_Target
{
    return float4(aInput.color, 1.0f);
}