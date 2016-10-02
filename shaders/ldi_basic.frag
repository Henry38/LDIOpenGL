#version 430

in vec2 texCoord;

uniform sampler2D textureColor;
uniform sampler2D textureDepth;
uniform sampler2D textureID;

out vec4 color;

void main()
{
    //ivec2 windowSize = textureSize(textureColor, 0);
    //vec2 t = vec2(640,480) / windowSize;
    //vec2 t = gl_FragCoord.xy / vec2(1000, 1000);

    vec3 bufferColor = texture2D(textureColor, texCoord).xyz;
    //vec3 bufferColor = texelFetch(textureColor, ivec2(gl_FragCoord.xy), 0).xyz;
    color = vec4(bufferColor, 1.0);

//    color = vec4(texCoord, 0.0, 1.0);
}
