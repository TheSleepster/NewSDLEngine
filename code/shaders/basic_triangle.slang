// VERTEX
struct vVertexInput_t
{
    float4 vPosition : POSITION;
    float2 vCorner   : TEXCOORD0;
    float2 vPadding  : TEXCOORD1;
};

struct VertexOutput_t 
{
    float4 vOutPosition : SV_POSITION;
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
    result.vOutPosition = mul(CameraMatrices.ProjectionMatrix, mul(CameraMatrices.ViewMatrix, Input.vPosition));

    return(result);
}

// FRAGMENT
struct PixelOutput_t 
{
    float4 vFragColor : SV_TARGET;
};

struct PushConstantData_t
{
    float4 Color;
};

[[vk::push_constant]]
PushConstantData_t Constants;

[shader("fragment")]
PixelOutput_t
PSMain(VertexOutput_t Output)
{
    PixelOutput_t result;
    result.vFragColor = Constants.Color;
        
    return(result);
}
