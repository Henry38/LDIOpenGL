#version 430
layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

in vec3 teCoordBary[];
flat in uvec3 teIndice[];
flat out vec3 gCoarseCoordBary[3];
flat out uvec3 gIndice;

out vec3 gDetailCoordBary;

void main() {

    //pour pouvoir identifier les sommets participant à la création du triangle
    gIndice.xyz = teIndice[0].xyz;

    gCoarseCoordBary[0] = teCoordBary[0];
    gCoarseCoordBary[1] = teCoordBary[1];
    gCoarseCoordBary[2] = teCoordBary[2];

    gDetailCoordBary = vec3(1.0, 0.0, 0.0);
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    gDetailCoordBary = vec3(0.0, 1.0, 0.0);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    gDetailCoordBary = vec3(0.0, 0.0, 1.0);
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
}
