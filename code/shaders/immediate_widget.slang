// VERTEX
#include <immediate_vertex.slh>
#include <widget_instance_data.slh>

struct VertexOutput_t 
{
    [[vk::location(0)]] float4 vOutPosition        : SV_POSITION;
    [[vk::location(1)]] float4 vOutColor           : SV_TARGET;
    [[vk::location(2)]] float2 vOutTexCoord        : TEXCOORD3;
    [[vk::location(3)]] float2 vOutWidgetOptions   : TEXCOORD4;
    [[vk::location(4)]] float2 vOutFilteringOptions: TEXCOORD5;
};

struct uCameraMatrices_t
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
};

ConstantBuffer<uCameraMatrices_t> CameraMatrices;

[shader("vertex")]
VertexOutput_t
VSMain(vImmediateVertex_t Input)
{
    VertexOutput_t result;
    float2 v2DPositionFloored = floor(Input.vPosition.xy);
    float4 vPositionFloored   = float4(v2DPositionFloored, Input.vPosition.zw);

    result.vOutPosition         = mul(CameraMatrices.ProjectionMatrix, mul(CameraMatrices.ViewMatrix, vPositionFloored));
    result.vOutColor            = Input.vColor + (float4(Input.vPadding, 0, 0) * float4(0.001));
    result.vOutTexCoord         = Input.vTexCoord;
    result.vOutWidgetOptions    = Input.vPadding;
    result.vOutFilteringOptions = Input.vSDFInfo;

    return(result);
}

// FRAGMENT
struct PixelOutput_t 
{
    float4 vFragColor : SV_TARGET;
};

Sampler2D TextureSamplers[4];
StructuredBuffer<ImmediateWidgetData_t> WidgetInstanceData;

float 
RoundedBoxSDF(float2 CenterPosition, float2 Size, float Radius) 
{
    return length(max(abs(CenterPosition) - Size + Radius, 0.0)) - Radius;
}

[shader("fragment")]
PixelOutput_t
PSMain(VertexOutput_t VertexOutput)
{
    PixelOutput_t Result;
    int WidgetType = int(floor(VertexOutput.vOutWidgetOptions.x));

    // NOTE(Sleepster): Standard rectangle 
    if(WidgetType == 0)
    {
        ImmediateWidgetData_t Instance = WidgetInstanceData[int(VertexOutput.vOutWidgetOptions.y)];
        uint WidgetFlags = Instance.iFlags;

        float2 HalfSize     = Instance.iHalfSize;
        float  Radius       = min(Instance.iRadius, min(HalfSize.x, HalfSize.y));
        float  EdgeSoftness = Instance.iSDFSmoothness;

        // NOTE(Sleepster): Base shape, an iRadius of 0.0 gives a plain rect, anything non-zero gives rounded corners
        float  Distance    = RoundedBoxSDF(VertexOutput.vOutTexCoord, HalfSize, Radius);
        float  OuterAlpha  = 1.0f - smoothstep(0.0f, EdgeSoftness * 2.0f, Distance);

        if(OuterAlpha <= 0.001f) discard;

        float4 ColorOut = VertexOutput.vOutColor;

        // NOTE(Sleepster): Border will follow whatever shape is already defined
        if((WidgetFlags & ImmediateWidgetRenderFlags_t.UI_WIDGET_RENDER_FLAG_DRAW_BORDER) != 0)
        {
            float2 InnerHalfSize = max(HalfSize - Instance.iBorderThickness, float2(0.0f, 0.0f));
            float  InnerRadius   = min(max(Radius - Instance.iBorderThickness, 0.0f), min(InnerHalfSize.x, InnerHalfSize.y));

            float  InnerDist  = RoundedBoxSDF(VertexOutput.vOutTexCoord, InnerHalfSize, InnerRadius);
            float  InnerAlpha = 1.0f - smoothstep(0.0f, EdgeSoftness * 2.0f, InnerDist);
            float  BorderMask = saturate(OuterAlpha - InnerAlpha);

            ColorOut.rgb = lerp(ColorOut.rgb, Instance.iBorderColor.rgb, BorderMask);
        }

        ColorOut.a       *= OuterAlpha;
        Result.vFragColor = ColorOut;
    }
    // NOTE(Sleepster): Textured rectangle 
    else if(WidgetType == 1)
    {
        Sampler2D Texture   = TextureSamplers[int(VertexOutput.vOutWidgetOptions.y)];
        float4 TextureColor = Texture.SampleLevel(VertexOutput.vOutTexCoord, 0.0);

        //if(TextureColor.a <= 0.1) discard;
        Result.vFragColor = float4(TextureColor.rgb * VertexOutput.vOutColor.rgb, TextureColor.a);
    }
    // NOTE(Sleepster): Font Glyph 
    else if(WidgetType == 2)
    {
        Sampler2D Texture = TextureSamplers[int(VertexOutput.vOutWidgetOptions.y)];

        float4 TextureColor = Texture.SampleLevel(VertexOutput.vOutTexCoord, 0.0);
        if(TextureColor.a <= 0.2) discard;

        Result.vFragColor = float4(VertexOutput.vOutColor.rgb, TextureColor.a);
    }

    return(Result);
}
