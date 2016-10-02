#version 430

in vec3 n;

uniform vec3 light;

layout(location = 0) out vec3 color;

void main()
{
    //vec3 bufferColor = texture2D(textureColor, texCoord).xyz;
    //vec3 bufferColor = texture2D(textureNormal, texCoord).xyz;
    //float bufferColor = texture2D(textureDepth, texCoord).x;

    //color = vec3(1.0);
    color = max(0.0, -dot(n, light)) * vec3(0.8, 0.8, 0.8);
}
