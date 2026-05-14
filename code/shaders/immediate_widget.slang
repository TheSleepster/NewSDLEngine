// VERTEX
struct vVertexInput_t
{
    float4 vPosition : POSITION;
    float4 vColor    : TEXCOORD0;
    float2 vTexCoord : TEXCOORD1;
    float2 vPadding  : TEXCOORD2;
};

struct VertexOutput_t 
{
    [[vk::location(0)]] float4 vOutPosition   : SV_POSITION;
    [[vk::location(1)]] float4 vOutColor      : SV_TARGET;
    [[vk::location(2)]] float2 vOutTexCoord   : TEXCOORD3;
    [[vk::location(3)]] float2 vWidgetOptions : TEXCOORD3;
};

struct uCameraMatrices_t
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
};

ConstantBuffer<uCameraMatrices_t> CameraMatrices;

[shader("vertex")]
VertexOutput_t
VSMain(vVertexInput_t Input)
{
    VertexOutput_t result;
    float2 v2DPositionFloored = floor(Input.vPosition.xy);
    float4 vPositionFloored   = float4(v2DPositionFloored, Input.vPosition.zw);

    result.vOutPosition   = mul(CameraMatrices.ProjectionMatrix, mul(CameraMatrices.ViewMatrix, vPositionFloored));
    result.vOutColor      = Input.vColor + (float4(Input.vPadding, 0, 0) * float4(0.001));
    result.vOutTexCoord   = Input.vTexCoord;
    result.vWidgetOptions = Input.vPadding;

    return(result);
}

// FRAGMENT
struct PixelOutput_t 
{
    float4 vFragColor : SV_TARGET;
};

Sampler2D TextureSamplers[4];

[shader("fragment")]
PixelOutput_t
PSMain(VertexOutput_t VertexOutput)
{
    PixelOutput_t result;
    int WidgetFlag = int(floor(VertexOutput.vWidgetOptions.x));

    // NOTE(Sleepster): Standard rectangle 
    if(WidgetFlag == 0)
    {
        result.vFragColor = VertexOutput.vOutColor;
    }
    // NOTE(Sleepster): Textured rectangle 
    else if(WidgetFlag == 1)
    {
        Sampler2D Texture = TextureSamplers[int(VertexOutput.vWidgetOptions.y)];

        float4 TextureColor = Texture.Sample(VertexOutput.vOutTexCoord);
        if(TextureColor.a <= 0.3) discard;

        result.vFragColor = float4(VertexOutput.vOutColor.rgb, TextureColor.a);
    }
    // NOTE(Sleepster): Font Glyph 
    else if(WidgetFlag == 2)
    {
        Sampler2D Texture = TextureSamplers[int(VertexOutput.vWidgetOptions.y)];

        float4 TextureColor = Texture.Sample(VertexOutput.vOutTexCoord);
        if(TextureColor.a <= 0.3) discard;

        result.vFragColor = float4(VertexOutput.vOutColor.rgb, TextureColor.a);
    }

    return(result);
}
