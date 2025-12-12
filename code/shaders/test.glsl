@vs vertex
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vColor;
layout(location = 2) in vec2 vTexCoords;

layout(binding = 0) uniform VSParams
{
    mat4 uProjectionMatrix;
    mat4 uViewMatrix;
};

out vec4 vOutColor;
out vec2 vOutTexCoords;

void main() 
{
    vOutColor     = vColor;
    vOutTexCoords = vTexCoords;
    gl_Position   = uProjectionMatrix * uViewMatrix * vPosition;
}
@end

@fs fragment
in vec4 vOutColor;
in vec2 vOutTexCoords;

layout(binding = 0) uniform texture2D uTexture;
layout(binding = 1) uniform sampler   uSampler;

out vec4 vOutFragColor;

void main() 
{
    vOutFragColor = texelFetch(sampler2D(uTexture, uSampler), ivec2(vOutTexCoords), 0);
}
@end

@program test vertex fragment 
