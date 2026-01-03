// VERTEX
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

struct global_matrix_uniforms 
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
};

[[vk::binding(0, 0)]]
ParameterBlock<global_matrix_uniforms> Matrices;

[shader("vertex")]
VertexOutput_t
VSMain(VertexInput_t input)
{
    VertexOutput_t result;

    float4 VSPosition   = mul(Matrices.ViewMatrix, float4(input.vPosition.xyz, 1.0));
    result.vOutPosition = mul(Matrices.ProjectionMatrix, VSPosition);
    result.vOutColor    = input.vColor;

    return(result);
}
// VERTEX 

// FRAGMENT
struct PixelOutput_t
{
    float4 vFragColor : SV_TARGET;
};

struct ConstantData {
    float4 DrawColor;
};

[[vk::push_constant]]
ConstantData PushConstants;

[shader("fragment")]
PixelOutput_t
PSMain(VertexOutput_t input)
{
    PixelOutput_t result;
    result.vFragColor = PushConstants.DrawColor;

    return(result);
}
// FRAGMENT 
