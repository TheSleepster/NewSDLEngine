struct vertex_output_t
{
    float4 v_position : SV_Position;
    float4 v_color    : COLOR0;
};

struct vertex_t 
{
    float3 position;
    float4 color;
};

static const vertex_t VERTICES[3] = {
    { { -0.5f, -0.5f,  0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { -0.5f,  0.5f,  0.0f }, { 0.0f, 1.0f, 0.0f } },
    { {  0.5f,  0.5f,  0.0f }, { 0.0f, 0.0f, 1.0f } }
};

[shader("vertex")]
vertex_output_t
vertex_main(uint vertex_index: SV_VertexID)
{
    vertex_output_t result;

    vertex_t vertex = VERTICES[vertex_index];
    result.v_position = float4(vertex.position, 1.0);
    result.v_color    = vertex.color;

    return(result);
}

struct pixel_output 
{
    [vk_location(0)]
    float4 v_frag_color : SV_Target;
};

[shader("fragment")]
pixel_output
fragment_main(vertex_output_t input)
{
    pixel_output output;
    output.v_frag_color = input.v_color;

    return(output);
}

