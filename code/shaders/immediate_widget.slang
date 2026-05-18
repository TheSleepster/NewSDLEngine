// VERTEX
#include <immediate_vertex.slh>
#include <widget_instance_data.slh>

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
VSMain(vImmediateVertex_t Input)
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
StructuredBuffer<ImmediateWidgetData_t> WidgetInstanceData;

float 
sdRoundedRect(float2 Position, float2 HalfSize, float Radius) 
{
    float2 q = abs(Position) - HalfSize + Radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - Radius;
}

[shader("fragment")]
PixelOutput_t
PSMain(VertexOutput_t VertexOutput)
{
    PixelOutput_t result;
    int WidgetFlag = int(floor(VertexOutput.vWidgetOptions.x));

    // NOTE(Sleepster): Standard rectangle 
    if(WidgetFlag == 0)
    {
        ImmediateWidgetData_t Instance = WidgetInstanceData[int(VertexOutput.vWidgetOptions.y)];

        float vUV       = VertexOutput.vOutTexCoord.x - float(0.5f);
        float2 HalfSize = float2(0.3, 0.2);
        float Radius    = Instance.iTopLeftRadius;

        float Distance = sdRoundedRect(vUV, HalfSize, Radius);

        float Rectangle = smoothstep(0.005, -0.005, Distance);
        result.vFragColor = float4(float3(Rectangle), 1.0);
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
