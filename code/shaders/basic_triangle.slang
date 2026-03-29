// VERTEX
struct vVertexInput_t
{
    float4 vPosition : POSITION;
    float4 vColor    : TEXCOORD0;
    float2 vCorner   : TEXCOORD1;
    float2 vPadding  : TEXCOORD2;
};

struct VertexOutput_t 
{
    float4 vOutPosition : SV_POSITION;
    float4 vOutColor    : SV_TARGET;
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

    result.vOutColor = Input.vColor;

    return(result);
}

// FRAGMENT
struct PixelOutput_t 
{
    float4 vFragColor : SV_TARGET;
};

[shader("fragment")]
PixelOutput_t
PSMain(VertexOutput_t Output)
{
    PixelOutput_t result;
    result.vFragColor = Output.vOutColor;
        
    return(result);
}
