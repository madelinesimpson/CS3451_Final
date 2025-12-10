#version 330 core

/*default camera matrices. do not modify.*/
layout(std140) uniform camera
{
    mat4 projection;	/*camera's projection matrix*/
    mat4 view;			/*camera's view matrix*/
    mat4 pvm;			/*camera's projection*view*model matrix*/
    mat4 ortho;			/*camera's ortho projection matrix*/
    vec4 position;		/*camera's position in world space*/
};

/* set light ubo. do not modify.*/
struct light
{
	ivec4 att; 
	vec4 pos; // position
	vec4 dir;
	vec4 amb; // ambient intensity
	vec4 dif; // diffuse intensity
	vec4 spec; // specular intensity
	vec4 atten;
	vec4 r;
};
layout(std140) uniform lights
{
	vec4 amb;
	ivec4 lt_att; // lt_att[0] = number of lights
	light lt[4];
};

uniform float iTime;
uniform mat4 model;

/*input variables*/
in vec3 vtx_normal; // vtx normal in world space
in vec3 vtx_position; // vtx position in world space
in vec3 vtx_model_position; // vtx position in model space
in vec4 vtx_color;
in vec2 vtx_uv;
in vec3 vtx_tangent;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float shininess;

/*output variables*/
out vec4 frag_color;

uniform vec2 iResolution;       /* window resolution */
uniform int iFrame;             /* frame index */
in vec2 fragCoord;              /* screen space coordinate */
out vec4 outputColor;           /* output color */

#define Time (iTime*1.0)            

#define PI 3.14159265359
#define TWO_PI 6.28318530718
#define Gravity 0.7             /* gravity */
#define NUM_STAR 30.            /* number of stars on the sky */
#define NUM_EMISSION 80.        /* number of emission particles */
#define NUM_FIREWORKS 12.        /* number of fireworks */
#define DURATION 2.2             /* duration of each fireworks period */

const vec2 g = vec2(.0, -Gravity); /* gravity */

//// This hash function takes input t and returns random float between 0 and 1
float hash1d(float t)
{
    t += 1.;
    return fract(sin(t * 674.3) * 453.2);
}

//// This hash function takes input t and returns random vec2 with each component between 0 and 1
vec2 hash2d(float t)
{
    t += 1.;
    float x = fract(sin(t * 674.3) * 453.2);
    float y = fract(sin((t + x) * 714.3) * 263.2);

    return vec2(x, y);
}

//// This hash function takes input t and returns random vec3 with each component between 0 and 1
vec3 hash3d(float t)
{
    t += 1.;
    float x = fract(sin(t * 674.3) * 453.2);
    float y = fract(sin((t + x) * 714.3) * 263.2);
    float z = fract(sin((t + y) * 134.3) * 534.2);

    return vec3(x, y, z);
}

//// This hash function takes input t and returns a random vec2 on a circle
vec2 hash2d_polar(float t)
{
    t += 1.;
    float a = fract(sin(t * 674.3) * 453.2) * TWO_PI;
    float d = fract(sin((t + a) * 714.3) * 263.2);
    return vec2(sin(a), cos(a)) * d;
}

vec3 renderParticle(vec2 fragPos, vec2 particlePos, float brightness, vec3 color)
{
    vec3 fragColor = vec3(0.0);
    vec2 d = fragPos - particlePos;
    // Make sure dist doesnt go to 0
    float dist = length(d) * 2.0;
    float decay = 1.0 / dist;
	fragColor = color * brightness * decay;

    return fragColor;
}

vec3 renderStars(vec2 fragPos)
{
    vec3 fragColor = vec3(0.01, 0.04, 0.3);
    float t = Time;

    for(float i = 0.; i < NUM_STAR; i++){
        vec2 pos = (hash2d(i) - .5) * iResolution.xy / iResolution.y;

        float brightness = .0004;

        // Randomize star color
        brightness = brightness * (1 + sin(t * i));
        vec3 color = vec3(0.6 + 0.4*sin(i*12.32), 0.6 + 0.4*sin(i*5.87 + 2.0), 0.6 + 0.4*sin(i*9.13 + 4.0));
        fragColor += renderParticle(fragPos, pos, brightness, color); 
    }

    return fragColor;
}


vec2 moveParticle(vec2 initPos, vec2 initVel, float t)
{
    vec2 currentPos = initPos;
    currentPos = currentPos + (initVel * t) + (0.5 * g * t * t);

    return currentPos;
}

vec3 simSingleParticle(vec2 fragPos, vec2 initPos, vec2 initVel, float t, float brightness, vec3 color)
{
    vec3 fragColor = vec3(0.0);

    vec2 currentPos = moveParticle(initPos, initVel, t);
    fragColor += renderParticle(fragPos, currentPos, brightness, color);

    return fragColor;
}

vec3 simSingleFirework(vec2 fragPos, vec2 launchPos, vec2 launchVel, float t, vec3 color)
{
    vec3 fragColor = vec3(0.0);
    float emitTime = 1.5;

    if(t < emitTime){
        float brightness = .002;
        vec2 initPos = launchPos;
        vec2 initVel = launchVel;
        fragColor += simSingleParticle(fragPos, initPos, initVel, t, brightness, color);
    }
    else{
        float emitT = t - emitTime;
        vec2 emitPos = moveParticle(launchPos, launchVel, emitTime);

        for(float i = 0.; i < NUM_EMISSION; i++){
            vec2 emitVel = hash2d_polar(i) * .7;

            float decay = exp(-5.0 * emitT);
            float flicker = 1.0 + sin(emitT * i);
            
            float brightness = 0.0005 * decay * flicker;
            fragColor += simSingleParticle(fragPos, emitPos, emitVel, emitT, brightness, color);
        }
    }

    return fragColor;
}

vec3 renderFireworks(vec2 fragPos)
{
    vec3 fragColor = vec3(0.0);

    for(float i = 0.; i < NUM_FIREWORKS; i++){
        float lauchTime = i;
        float relTime = Time - lauchTime;
        float t = mod(relTime, DURATION);
        float idx = floor(relTime / DURATION);

        float random = hash1d(idx*9.7);

        // Randomize launch position, velocity, and color
        float x = (hash1d(idx*3.1) - 0.5) * 0.1; 
        vec2 launchPos = vec2(x, -0.55);
        vec2 launchVel = vec2((hash1d(idx * 2.0) - 0.5) * 0.05, 1.1 + hash1d(idx * 4.0) * 0.2);
        vec3 base = hash3d(idx * 13.77);    
        vec3 color = 0.5 + 0.5*sin(Time*3.0 + base*8.0); 

        fragColor += simSingleFirework(fragPos, launchPos, launchVel, t, color);
    }

    return fragColor;
}

void mainImage(out vec4 outputColor, in vec2 fragCoord)
{
    //// fragPos's center is at the center of the screen, fragPos.y range is [-0.5, 0.5]
    vec2 fragPos = (fragCoord - .5 * iResolution.xy) / iResolution.y;

    vec3 fragColor = vec3(0.0);
    {
       fragColor = renderStars(fragPos) + renderFireworks(fragPos);
    }
    
    outputColor = vec4(fragColor, 1.0);
}

void main()
{
    mainImage(outputColor, fragCoord);
}
