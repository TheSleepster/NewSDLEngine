struct VertexInput_t
{
    float3 vPosition : POSITION;
};

struct VertexOutput_t
{
    float4 vOutPosition : SV_POSITION;
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
float4
PSMain(VertexOutput_t input) : SV_Target
{
    float4 vFragColor = float4(1.0);

    return(vFragColor);
}
