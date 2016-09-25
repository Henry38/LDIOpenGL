#version 430

uniform sampler2D textureColor;
uniform sampler2D textureNormal;
uniform sampler2D textureDepth;

out vec4 color;

void main()
{
    vec2 texCoord = (gl_FragCoord.xy / vec2(640, 480));

    vec3 bufferColor = texture2D(textureColor, texCoord).xyz;
//    vec3 bufferColor = texture2D(textureNormal, texCoord).xyz;
//    float bufferColor = texture2D(textureDepth, texCoord).x;

    color = vec4(vec3(bufferColor), 1.0);
}
