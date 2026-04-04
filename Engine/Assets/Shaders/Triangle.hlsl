struct VSOutput
{
    float4 position : SV_Position;
    float3 color    : COLOR;
};

VSOutput VSMain(uint aVertexID : SV_VertexID)
{
    // Fullscreen triangle — 3 vertices cover the entire screen
    float2 positions[3] =
    {
        float2(-1.0f, -1.0f),
        float2( 0.0f,  1.0f),
        float2( 1.0f, -1.0f),
    };

    float3 colors[3] =
    {
        float3(1.0f, 0.0f, 0.0f), // red
        float3(0.0f, 1.0f, 0.0f), // green
        float3(0.0f, 0.0f, 1.0f), // blue
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