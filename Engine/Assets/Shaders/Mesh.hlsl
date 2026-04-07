struct Vertex
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 texCoord;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct CameraConstants
{
    float4x4 viewProj;
};

struct MeshDrawConstants
{
    uint     vertexBufferIndex;
    uint     albedoIndex;
    uint     pad2;
    uint     pad3;
    float4x4 transform;
};

static const uint SAMPLER_LINEAR_WRAP      = 0;
static const uint SAMPLER_LINEAR_CLAMP     = 1;
static const uint SAMPLER_POINT_WRAP       = 2;
static const uint SAMPLER_POINT_CLAMP      = 3;
static const uint SAMPLER_ANISOTROPIC_WRAP = 4;

ConstantBuffer<CameraConstants> gCamera : register(b1, space0);
ConstantBuffer<MeshDrawConstants> gMeshConstants : register(b0, space0);

VertexOutput VSMain(const uint aVertexID : SV_VertexID)
{
    StructuredBuffer<Vertex> vertices = ResourceDescriptorHeap[gMeshConstants.vertexBufferIndex];
    const Vertex v = vertices[aVertexID];

    const float4 worldPos = mul(float4(v.position, 1.0f), gMeshConstants.transform);

    VertexOutput output;
    output.position = mul(worldPos, gCamera.viewProj);
    output.normal   = mul(v.normal, (float3x3)gMeshConstants.transform);
    output.texCoord = v.texCoord;
    return output;
}

float4 PSMain(VertexOutput aInput) : SV_Target
{
    SamplerState sampler = SamplerDescriptorHeap[SAMPLER_LINEAR_WRAP];
    Texture2D albedo = ResourceDescriptorHeap[gMeshConstants.albedoIndex];
    const float4 albedoColor = albedo.Sample(sampler, aInput.texCoord);

    const float3 lightDir = normalize(float3(0.5f, 1.0f, 0.3f));
    const float3 normal   = normalize(aInput.normal);
    const float  diffuse  = max(dot(normal, lightDir), 0.0f);
    const float3 color    = albedoColor.rgb * diffuse + float3(0.05f, 0.05f, 0.05f);
    return float4(color, 1.0f);
}