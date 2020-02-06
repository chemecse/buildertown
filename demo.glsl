@ctype mat4 mat4

@vs vs
uniform vs_params {
    mat4 mvp;
};

in vec3 position;
in vec3 normal;
in vec2 texcoord;

out vec4 color;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    color = vec4((normal + 1.0) * 0.5 + 0.000001 * texcoord.x, 1.0);
}
@end

@fs fs
in vec4 color;
out vec4 frag_color;

void main() {
    frag_color = color;
}
@end

@program demo vs fs
