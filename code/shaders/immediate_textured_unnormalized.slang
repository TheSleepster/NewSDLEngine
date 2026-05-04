#include <test.slh>

// VERTEX
struct vInput_t
{
    float4 vPosition : POSITION;
    float4 vColor    : TEXCOORD0;
    float2 vTexCoord : TEXCOORD1;
    float2 vPadding  : TEXCOORD2;
};

struct VertexOutput_t 
{
    [[vk::location(0)]] float4 vOutPosition : SV_POSITION;
    [[vk::location(1)]] float4 vOutColor    : SV_TARGET;
    [[vk::location(2)]] float2 vOutTexCoord : TEXCOORD3;
};

struct uCameraMatrices_t
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
};

ConstantBuffer<uCameraMatrices_t> CameraMatrices;

[shader("vertex")]
VertexOutput_t
VSMain(vInput_t Input)
{
    VertexOutput_t result;
    
    float4 vPositionFloored = floor(Input.vPosition);

    result.vOutPosition = mul(CameraMatrices.ProjectionMatrix, mul(CameraMatrices.ViewMatrix, vPositionFloored));
    result.vOutColor    = Input.vColor + (float4(Input.vPadding, 0, 0) * float4(0.001));
    result.vOutTexCoord = Input.vTexCoord;

    return(result);
}

// FRAGMENT
struct PixelOutput_t 
{
    float4 vFragColor : SV_TARGET;
};

Sampler2D TextureSampler;

[shader("fragment")]
PixelOutput_t
PSMain(VertexOutput_t VertexOutput)
{
    PixelOutput_t result;
    
    float4 TextureColor = TextureSampler.SampleLevel(VertexOutput.vOutTexCoord, 0.0);
    if(TextureColor.a == 0.0) 
    {
        discard;
    }

    result.vFragColor = TextureColor * VertexOutput.vOutColor;
    return(result);
}
