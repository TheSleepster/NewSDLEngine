// VERTEX
struct VertexInput_t
{
    float4 vPosition : POSITION;
    float4 vColor    : COLOR;
    float2 vTexCoord : TEXCOORD0;
};

struct VertexOutput_t
{
                        float4 vOutPosition : SV_POSITION;
    [[vk::location(0)]] float4 vOutColor    : SV_TARGET;
    [[vk::location(1)]] float2 vOutTexCoord : TEXCOORD0;
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
    result.vOutTexCoord = input.vTexCoord;

    return(result);
}
// VERTEX 

// FRAGMENT
struct PixelOutput_t
{
    float4 vFragColor : SV_TARGET;
};

// NOTE(Sleepster): binding first, set second 
[[vk::binding(1, 0)]]
Sampler2D TextureSampler;

// struct ConstantData {
//     float4 DrawColor;
// };

// [[vk::push_constant]]
// ConstantData PushConstants;

[shader("fragment")]
PixelOutput_t
PSMain(VertexOutput_t input)
{
    PixelOutput_t result;

    float4 TextureColor = TextureSampler.Sample(input.vOutTexCoord);
    if(TextureColor.a == 0.0)
    {
        discard;
    }
    // NOTE(Sleepster): This should be used for color override.  
    //result.vFragColor = TextureColor * input.vOutColor;

    result.vFragColor = TextureColor;

    return(result);
}
// FRAGMENT 
