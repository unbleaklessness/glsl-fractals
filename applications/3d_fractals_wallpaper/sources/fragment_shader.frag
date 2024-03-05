#version 330 core

uniform vec2 screenSize;  // Width and height of the shader
uniform float time;  // Time elapsed
uniform vec2 offset;
uniform float zoom;

// Constants
#define PI 3.1415925359
#define TWO_PI 6.2831852
#define MARCHING_MAX_STEPS 100
#define MARCHING_MAX_DISTANCE 4.
#define MARCHING_SURFACE_DISTANCE .0005

vec3 orbitTrap = vec3(1e9);

#if 1
float DE(vec3 pos) {

    vec3 z = pos;
    float dr = 1.0;
    float r = 0.0;

    float amplitude = 3.;
    float power = 3. + sin(time / amplitude) * amplitude + amplitude;
//    float power = 4.;

    float bailout = 4;
    int iterations = 5;

    for (int i = 0; i < iterations; i++) {

        r = length(z);

        if (r > bailout) break;

        // Convert to polar coordinates.
//        float theta = acos(z.z / r);
//        float phi = atan(z.y, z.x);
        float theta = asin(z.z / r);
        float phi = atan(z.y, z.x);
        dr = pow(r, power - 1.0) * power * dr + 1.0;

        // Scale and rotate the point.
        float zr = pow(r, power);
        theta = theta * power;
        phi = phi * power;

        // Convert back to cartesian coordinates.
//        z = zr * vec3(sin(theta) * cos(phi), sin(phi) * sin(theta), cos(theta));
        z = zr * vec3(cos(theta) * cos(phi), cos(theta) * sin(phi), sin(theta));
        z += pos;

        orbitTrap.x = min(orbitTrap.x, pow(length(z - vec3(1, 0, 0)), 2.));
        orbitTrap.y = min(orbitTrap.y, pow(length(z - vec3(0, 1, 0)), 2.));
        orbitTrap.z = min(orbitTrap.z, pow(length(z - vec3(0, 0, 2)), 2.));
    }

    return 0.5 * log(r) * r / dr;
}
#endif

#if 0
// Computes the distance estimate (DE) for the Menger Sponge fractal using the distance field algorithm
float DE(vec3 pos) {
    // Center the position and scale it
    float x = pos.x * 0.5 + 0.5;
    float y = pos.y * 0.5 + 0.5;
    float z = pos.z * 0.5 + 0.5;

    // Compute the distance to the initial box
    float xx = abs(x - 0.5) - 0.5;
    float yy = abs(y - 0.5) - 0.5;
    float zz = abs(z - 0.5) - 0.5;
    float d1 = max(xx, max(yy, zz));

    // Initialize the current computed distance
    float d = d1;

    // Initialize the scaling factor
    float p = 1.0;

    // Set the number of iterations
    int n = 10;

    // Perform iterations to compute the DE
    for (int i = 1; i <= n; ++i) {
        // Compute the translated/rotated positions
        float xa = mod(3.0 * x * p, 3.0);
        float ya = mod(3.0 * y * p, 3.0);
        float za = mod(3.0 * z * p, 3.0);
        p *= 3.0;

        // Compute the distance inside the 3 axis-aligned square tubes
        float xx = 0.5 - abs(xa - 1.5);
        float yy = 0.5 - abs(ya - 1.5);
        float zz = 0.5 - abs(za - 1.5);
        d1 = min(max(xx, zz), min(max(xx, yy), max(yy, zz))) / p;

        // Take the intersection of the current computed distance and the previous distance
        d = max(d, d1);

        orbitTrap.x = min(orbitTrap.x, pow(length(d - vec3(1, 0, 0)), 2.));
        orbitTrap.y = min(orbitTrap.y, pow(length(d - vec3(0, 1, 0)), 2.));
        orbitTrap.z = min(orbitTrap.z, pow(length(d - vec3(0, 0, 1)), 2.));
    }

    // Return the final distance estimate
    return d;
}
#endif

bool escapedForGlow = false;
float minimumDistanceForGlow = 1e9;
int stepsForOcclusion = 0;

float rayMarching(vec3 rayOrigin, vec3 rayDirection)
{
    float distanceFromOrigin = 0.;
    for(int i = 0; i < MARCHING_MAX_STEPS; i++)
    {
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

mat3 rotate(vec3 axis, float angle) {
    float c = cos(angle);
    float s = sin(angle);
    float oneMinusC = 1.0 - c;

    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    return mat3(
        x * x * oneMinusC + c,      x * y * oneMinusC - z * s,  x * z * oneMinusC + y * s,
        y * x * oneMinusC + z * s,  y * y * oneMinusC + c,      y * z * oneMinusC - x * s,
        z * x * oneMinusC - y * s,  z * y * oneMinusC + x * s,  z * z * oneMinusC + c
    );
}

float map(float value, float inputMin, float inputMax, float outputMin, float outputMax) {
    // Calculate the normalized position of the value within the input range
    float normalizedValue = (value - inputMin) / (inputMax - inputMin);

    // Map the normalized value to the output range
    return outputMin + (outputMax - outputMin) * normalizedValue;
}

vec3 hue_shift(vec3 color, float dhue) {
    float s = sin(dhue);
    float c = cos(dhue);
    return (color * c) + (color * s) * mat3(
        vec3(0.167444, 0.329213, -0.496657),
        vec3(-0.327948, 0.035669, 0.292279),
        vec3(1.250268, -1.047561, -0.202707)
    ) + dot(vec3(0.299, 0.587, 0.114), color) * (1.0 - c);
}

void main()
{
    vec2 uv = (gl_FragCoord.xy - .5 * screenSize.xy) / screenSize.y;
    
    vec3 rayOrigin = vec3(0, 0, 1);
    vec3 rayDirection = normalize(vec3(uv.x, uv.y, -1.));
    
    float rotationSensitivity = 0.5;
#if 0
    mat3 rotationX = rotate(vec3(1, 0, 0), 2. * PI * offset.y / screenSize.y * rotationSensitivity - PI);
    mat3 rotationY = rotate(vec3(0, 1, 0), 2. * PI * offset.x / screenSize.x * rotationSensitivity);
#else
    mat3 rotationX = rotate(vec3(1, 0, 0), PI * sin(time / 12.));
    mat3 rotationY = rotate(vec3(0, 1, 0), PI * cos(time / 12));
#endif
    mat3 rotation = rotationX * rotationY;
    
    rayDirection = rotation * rayDirection;
    rayOrigin = rotation * vec3(0, 0, 2) * zoom * 1.5;
    
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
        
        float occlusion = 1. - float(stepsForOcclusion) / float(MARCHING_MAX_STEPS);

        orbitTrap.x = map(orbitTrap.x, 0, MARCHING_MAX_DISTANCE, 0, 1);
        orbitTrap.y = map(orbitTrap.y, 0, MARCHING_MAX_DISTANCE, 0, 1);
        orbitTrap.z = map(orbitTrap.z, 0, MARCHING_MAX_DISTANCE, 0, 1);

        color = vec3(orbitTrap.x, 0, orbitTrap.z);
        color = hue_shift(color, PI / 12);
    }
    
    gl_FragColor = vec4(color, 1);
}
