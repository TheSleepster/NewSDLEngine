// VERTEX
struct VertexInput_t
{
    float4 vPosition : POSITION;
    float2 vCorner   : TEXCOORD0;
    float2 vPadding;
};

struct VertexOutput_t
{
                        float4 vOutPosition : SV_POSITION;
    [[vk::location(0)]] float4 vOutColor    : SV_TARGET;
    [[vk::location(1)]] float2 vOutTexCoord : TEXCOORD0;
};

// TODO(Sleepster): this will have to be buffered soon
struct GlobalMatrixUniforms_t
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
};

struct RenderGeometryInstance_t 
{
    float4x4 iTransform;
    float4   iColor;
    float2   iUVMin;
    float2   iUVMax;
    uint     iTextureIndex;
    uint     iCameraIndex;
};

[[vk::binding(0, 0)]]
ConstantBuffer<GlobalMatrixUniforms_t> Matrices;

[[vk::binding(1, 0)]]
StructuredBuffer<RenderGeometryInstance_t> RenderInstances;

[shader("vertex")]
VertexOutput_t
VSMain(VertexInput_t input, 
       uint          InstanceID : SV_InstanceID)
{
    VertexOutput_t result;
    RenderGeometryInstance_t instance = RenderInstances[InstanceID];

    float4 WorldPosition = mul(input.vPosition, instance.iTransform);

    float4 VSPosition   = mul(Matrices.ViewMatrix, WorldPosition);
    result.vOutPosition = mul(VSPosition, Matrices.ProjectionMatrix);
    result.vOutColor    = instance.iColor;

    float2 InterpolatedUV = lerp(instance.iUVMin.xy, instance.iUVMax.xy, input.vCorner);
    result.vOutTexCoord   = InterpolatedUV;

    return(result);
}
// VERTEX 

// FRAGMENT
struct PixelOutput_t
{
    float4 vFragColor : SV_TARGET;
};

// NOTE(Sleepster): binding first, set second 
[[vk::binding(2, 0)]]
Sampler2D TextureSampler;

// struct ConstantData {
//     float4 DrawColor;
// };

// [[vk::push_constant]]
// ConstantData PushConstants;

[shader("fragment")]
PixelOutput_t
PSMain(VertexOutput_t input)
{
    PixelOutput_t result;

    float4 TextureColor = TextureSampler.Sample(input.vOutTexCoord);
    if(TextureColor.a == 0.0)
    {
        discard;
    }
    // NOTE(Sleepster): This should be used for color override.  
    //result.vFragColor = TextureColor * input.vOutColor;

    result.vFragColor = TextureColor;

    return(result);
}
// FRAGMENT 
