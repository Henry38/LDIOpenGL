#version 430 core
#extension GL_ARB_shading_language_include : require
#include "/extensions.hglsl"
#include "/uniforms.hglsl"
#include "/tessellationEvaluation.hglsl"

layout(triangles, equal_spacing, ccw) in;

in vec3 tcPosition[];
in vec3 tcNormal[];
in vec2 tcTexCoords[];
flat in uint tcIndice[];

flat out uvec3 teIndice;
out vec3 teCoordBary;

//vec3 displacement(vec3 position, vec3 normal, vec2 texCoords)
//{
//    vec4 dv = texture(texture1, texCoords);
//    float df = 0.3*dv.x+0.59*dv.y+0.11*dv.z;
//    return position + df*normal;
//}

#include "/displacement.glsl"

void main()
{
    teIndice.x = tcIndice[0];
    teIndice.y = tcIndice[1];
    teIndice.z = tcIndice[2];
    teCoordBary = gl_TessCoord;
    vec3 tePosition = interpolate3D(tcPosition[0], tcPosition[1], tcPosition[2]);
    vec3 teNormal = interpolate3D(tcNormal[0], tcNormal[1], tcNormal[2]);
    vec2 teTexCoords = interpolate2D(tcTexCoords[0], tcTexCoords[1], tcTexCoords[2]);
    vec3 newPosition = displacement(tePosition, teNormal, teTexCoords);
    gl_Position = projMat*viewMat*vec4(newPosition, 1);
}
