#version 430
layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

flat in uint vvIndice [];
flat out uvec3 gIndice;

out vec3 coordBary;

void main() {

    //pour pouvoir identifier les sommets participant à la création du triangle
    gIndice.x = vvIndice[0];
    gIndice.y = vvIndice[1];
    gIndice.z = vvIndice[2];

    coordBary = vec3(1.0, 0.0, 0.0);
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    coordBary = vec3(0.0, 1.0, 0.0);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    coordBary = vec3(0.0, 0.0, 1.0);
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
}
