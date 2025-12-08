@vs vertex
layout(location = 0) in vec4 vPosition;
layout(location = 1) in vec4 vColor;

layout(binding = 0) uniform VSParams {
    mat4 uProjectionMatrix;
    mat4 uViewMatrix;
};

out vec4 vOutColor;

void main() 
{
    gl_Position = uProjectionMatrix * uViewMatrix * vPosition;
    vOutColor   = vColor;
}
@end

@fs fragment
in  vec4 vOutColor;
out vec4 vOutFragColor;

void main() 
{
    vOutFragColor = vOutColor;
}
@end

@program test vertex fragment 
