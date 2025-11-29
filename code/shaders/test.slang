struct vertex_input_t 
{
    float4 position : SV_Position;
};

struct vertex_output_t
{
    float4 normalized_position;
};

[shader("vertex")]
vertex_output_t
vertex_main(vertex_input_t input)
{
    vertex_output_t result;
    result.normalized_position = input.position;

    return(result);
}

[shader("fragment")]
float4
fragment_main(vertex_output_t output)
{
    float4 out_color = float4(1.0, 1.0, 1.0, 1.0);
    return(out_color);
}

