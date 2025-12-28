struct VertexInput_t
{
    float4 vPosition : POSITION;
    float4 vColor    : COLOR;
};

struct VertexOutput_t
{
    float4 vOutPosition : SV_POSITION;
    float4 vOutColor    : SV_TARGET;
};

[shader("vertex")]
VertexOutput_t
VSMain(VertexInput_t input)
{
    VertexOutput_t result;
    result.vOutPosition = float4(input.vPosition.xyz, 1.0);
    result.vOutColor    = input.vColor;

    return(result);
}

struct PixelOutput_t
{
    float4 vFragColor : SV_TARGET;
};

[shader("fragment")]
PixelOutput_t
PSMain(VertexOutput_t input)
{
    PixelOutput_t result;
    result.vFragColor = input.vOutColor;

    return(result);
}
