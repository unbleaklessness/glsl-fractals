#version 330 core

uniform vec2 screenSize;
uniform vec2 offset;
uniform float zoom;

out vec4 returnColor;

vec2
multiplyComplex(vec2 a, vec2 b)
{
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

void
main()
{
    vec2 uv =
      (gl_FragCoord.xy / screenSize - 0.5) * zoom + 0.5 + offset / screenSize;

    int jumps = 500;

    vec3 color = vec3(1.0, 0.0, 0.0);

    vec2 z = vec2(0.0, 0.0);
    vec2 c = uv.xy;

    for (int i = 0; i <= jumps; i++) {
        z = multiplyComplex(z, z) + c;
        if (length(z) >= 10.0) {
            float n = float(i) / float(jumps);
            color = vec3(n, 0.0, 0.0);
            break;
        }
    }

    returnColor = vec4(color, 1.0);
}
