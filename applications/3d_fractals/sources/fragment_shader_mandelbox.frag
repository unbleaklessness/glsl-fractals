#version 330 core

uniform vec2 screenSize; // Width and height of the shader
uniform float time;      // Time elapsed
uniform vec2 offset;
uniform float zoom;

// Constants
#define PI 3.1415925359
#define TWO_PI 6.2831852
#define MARCHING_MAX_STEPS 100
#define MARCHING_MAX_DISTANCE 100.
#define MARCHING_SURFACE_DISTANCE .0005

vec3 orbitTrap = vec3(1e9);

float fixedRadius2 = 1.0;
float minRadius2 = 0.5;
float foldingLimit = 1;

void
sphereFold(inout vec3 z, inout float dz)
{
    float r2 = dot(z, z);
    if (r2 < minRadius2) {
        // linear inner scaling
        float temp = (fixedRadius2 / minRadius2);
        z *= temp;
        dz *= temp;
    } else if (r2 < fixedRadius2) {
        // this is the actual sphere inversion
        float temp = (fixedRadius2 / r2);
        z *= temp;
        dz *= temp;
    }
}
void
boxFold(inout vec3 z, inout float dz)
{
    z = clamp(z, -foldingLimit, foldingLimit) * 2.0 - z;
}

float
DE(vec3 z)
{
    // float Scale = -3;
    float Scale = -3 + sin(time);
    // float Scale = 3 + sin(time);
    int Iterations = 10;
    vec3 offset = z;
    float dr = 1.0;
    for (int n = 0; n < Iterations; n++) {
        boxFold(z, dr);    // Reflect
        sphereFold(z, dr); // Sphere Inversion

        z = Scale * z + offset; // Scale & Translate
        dr = dr * abs(Scale) + 1.0;
    }
    float r = length(z);
    // return r/abs(dr) - 5;
    // return r/abs(dr) - (2.5 * 2. * 1.5);
    // return r/abs(dr) - (10.0);
    // return r/abs(dr);
    float result = r / abs(dr);
    orbitTrap.x = min(orbitTrap.x, pow(length(result - vec3(1, 0, 0)), 2.));
    orbitTrap.y = min(orbitTrap.y, pow(length(result - vec3(0, 1, 0)), 2.));
    orbitTrap.z = min(orbitTrap.z, pow(length(result - vec3(0, 0, 1)), 2.));
    // return r/abs(dr);
    return result;
}

bool escapedForGlow = false;
float minimumDistanceForGlow = 1e9;
int stepsForOcclusion = 0;

float
rayMarching(vec3 rayOrigin, vec3 rayDirection)
{
    float distanceFromOrigin = 0.;
    for (int i = 0; i < MARCHING_MAX_STEPS; i++) {
        vec3 p = rayOrigin + rayDirection * distanceFromOrigin;
        float ds = DE(p);
        distanceFromOrigin += ds;
        minimumDistanceForGlow = min(minimumDistanceForGlow, ds);
        stepsForOcclusion = i;
        if (ds < MARCHING_SURFACE_DISTANCE) {
            break;
        }
        if (distanceFromOrigin > MARCHING_MAX_DISTANCE) {
            escapedForGlow = true;
            break;
        }
    }

    return distanceFromOrigin;
}

mat3
rotate(vec3 axis, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    float oneMinusC = 1.0 - c;

    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    return mat3(x * x * oneMinusC + c,
                x * y * oneMinusC - z * s,
                x * z * oneMinusC + y * s,
                y * x * oneMinusC + z * s,
                y * y * oneMinusC + c,
                y * z * oneMinusC - x * s,
                z * x * oneMinusC - y * s,
                z * y * oneMinusC + x * s,
                z * z * oneMinusC + c);
}

float
map(float value,
    float inputMin,
    float inputMax,
    float outputMin,
    float outputMax)
{
    // Calculate the normalized position of the value within the input range
    float normalizedValue = (value - inputMin) / (inputMax - inputMin);

    // Map the normalized value to the output range
    return outputMin + (outputMax - outputMin) * normalizedValue;
}

vec3
hue_shift(vec3 color, float dhue)
{
    float s = sin(dhue);
    float c = cos(dhue);
    return (color * c) +
           (color * s) * mat3(vec3(0.167444, 0.329213, -0.496657),
                              vec3(-0.327948, 0.035669, 0.292279),
                              vec3(1.250268, -1.047561, -0.202707)) +
           dot(vec3(0.299, 0.587, 0.114), color) * (1.0 - c);
}

void
main()
{
    vec2 uv = (gl_FragCoord.xy - .5 * screenSize.xy) / screenSize.y;

    vec3 rayOrigin = vec3(0, 0, 1);
    vec3 rayDirection = normalize(vec3(uv.x, uv.y, -1.));

    float rotationSensitivity = 0.5;
#if 1
    mat3 rotationX =
      rotate(vec3(1, 0, 0),
             2. * PI * offset.y / screenSize.y * rotationSensitivity - PI);
    mat3 rotationY = rotate(
      vec3(0, 1, 0), 2. * PI * offset.x / screenSize.x * rotationSensitivity);
#else
    mat3 rotationX = rotate(vec3(1, 0, 0), PI * sin(time / 12.));
    mat3 rotationY = rotate(vec3(0, 1, 0), PI * cos(time / 12));
#endif
    mat3 rotation = rotationX * rotationY;

    rayDirection = rotation * rayDirection;
    float zoom_new = zoom + 5;
    rayOrigin = rotation * vec3(0, 0, 2) * zoom_new * 1.5;

    float distance = rayMarching(rayOrigin, rayDirection);
    distance = 1 - map(distance, 0, MARCHING_MAX_DISTANCE, 0, 1);

    vec3 color;

    if (escapedForGlow) {

        minimumDistanceForGlow = min(max(minimumDistanceForGlow * 4, 0.), 1.);
        minimumDistanceForGlow = 1 - minimumDistanceForGlow;
        minimumDistanceForGlow *= 0.5;
        color = vec3(minimumDistanceForGlow, 0., 0.);
        color = hue_shift(color, PI / 4);

    } else {

        float occlusion =
          1. - float(stepsForOcclusion) / float(MARCHING_MAX_STEPS);

        orbitTrap.x = map(orbitTrap.x, 0, MARCHING_MAX_DISTANCE, 0, 1);
        orbitTrap.y = map(orbitTrap.y, 0, MARCHING_MAX_DISTANCE, 0, 1);
        orbitTrap.z = map(orbitTrap.z, 0, MARCHING_MAX_DISTANCE, 0, 1);

        color = vec3(orbitTrap.x, 0, orbitTrap.z);
        color = hue_shift(color, PI / 12);
    }

    // gl_FragColor = vec4(color, 1);
    gl_FragColor = vec4(distance, distance, distance, 1);
    // gl_FragColor = vec4(occlusion, occlusion, occlusion, 1);
}
