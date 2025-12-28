struct VertexInput_t
{
    float3 vPosition : POSITION;
};

struct VertexOutput_t
{
    float4 vOutPosition : SV_POSITION;
};

struct PixelOutput_t
{
    float4 vFragColor : SV_Target;
};

[shader("vertex")]
VertexOutput_t
VSMain(VertexInput_t input)
{
    VertexOutput_t result;
    result.vOutPosition = float4(input.vPosition, 1.0);

    return(result);
}

[shader("fragment")]
PixelOutput_t
PSMain(VertexOutput_t input)
{
    PixelOutput_t result;
    result.vFragColor = float4(1.0);

    return(result);
}
