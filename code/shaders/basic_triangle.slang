struct VertexInput_t
{
    float4 vPosition : POSITION;
    float2 vCorner   : TEXCOORD0;
    float2 vPadding;
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
    result.vOutPosition = float4(input.vPosition.x, input.vPosition.y, input.vPosition.zw);

    return(result);
}

struct PixelOutput_t 
{
    float4 vFragColor : SV_TARGET;
};

struct PushConstantData_t
{
    float4 Color;
};

[[vk::push_constant]]
PushConstantData_t constants;

[shader("fragment")]
PixelOutput_t
PSMain()
{
    PixelOutput_t result;
    result.vFragColor = constants.Color;
        
    return(result);
}
