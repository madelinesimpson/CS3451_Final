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

float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

float hash2d(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// generating procedural windows
vec3 generate_windows(vec3 pos, vec3 normal) {
	// determining which faceforward
	vec3 abs_normal = abs(normal);
    vec2 uv;

	// projecting onto dominiant faceforward
	if(abs_normal.x > abs_normal.y && abs_normal.x > abs_normal.z) {
        // x dominant
        uv = pos.yz;
    } else if(abs_normal.y > abs_normal.z) {
        // y dominat
        return vec3(0.0);
    } else {
        // z dominnat
        uv = pos.xy;
    }

    // scaling
    float window_scale = 4.0;
    vec2 grid_uv = uv * window_scale;
    vec2 grid_id = floor(grid_uv);
    vec2 grid_fract = fract(grid_uv);

    // window dims
    float window_size = 0.7;
    float border = (1.0 - window_size) * 0.5;
    
    // check if inside window
    bool in_window = grid_fract.x > border && grid_fract.x < (1.0 - border) &&
                     grid_fract.y > border && grid_fract.y < (1.0 - border);
    
    if(!in_window) {
        // building exterior (dark)
        return vec3(0.0);
    }
    
    // window is on - calculate if it should be lit
    float window_id = hash2d(grid_id);
    
    // time-based flickering
    float flicker_speed = 0.5;
    float flicker = hash(window_id + floor(iTime * flicker_speed));
    
    // M=most windows are on, some flicker off
    float lit = step(0.15, flicker);
    
    // window color variations
    vec3 warm_light = vec3(1.0, 0.9, 0.6);
    vec3 cool_light = vec3(0.6, 0.8, 1.0);
    vec3 window_color = mix(warm_light, cool_light, hash(window_id * 2.0));
    
    // adding brightness variation
    float brightness = 0.6 + 0.4 * hash(window_id * 3.0);

    return window_color * brightness * lit;
}

// phong shading
vec4 shading_phong(light li, vec3 e, vec3 p, vec3 n) {
    vec3 v = normalize(e - p);
    vec3 l = normalize(li.pos.xyz - p);
    vec3 r = normalize(reflect(-l, n));

    vec3 ambColor = ka * li.amb.rgb;
    vec3 difColor = kd * li.dif.rgb * max(0., dot(n, l));
    vec3 specColor = ks * li.spec.rgb * pow(max(dot(v, r), 0.), shininess);

    return vec4(ambColor + difColor + specColor, 1);
}

// surface detail and texture
float surface_noise(vec2 uv) {
    vec2 i = floor(uv * 50.0);
    return hash2d(i) * 0.1;
}

void main()
{
    vec3 e = position.xyz;
    vec3 p = vtx_position;
    vec3 N = normalize(vtx_normal);
    
    // calculating base lighting
    vec4 total_light = vec4(0.0);
    for(int i = 0; i < lt_att[0]; i++) {
        total_light += shading_phong(lt[i], e, p, N);
    }
    
    // Generate procedural windows
    vec3 window_emission = generate_windows(vtx_model_position, N);
    
    // adding noise
    float noise = surface_noise(vtx_model_position.xy + vtx_model_position.yz);
    
    // building color
    vec3 building_color = total_light.rgb * (1.0 + noise);
    
    // windows
    building_color += window_emission * 1.5;
    
    // lighting
    vec3 atmospheric = vec3(0.0);
    for(int i = 0; i < lt_att[0]; i++)
    {
        vec3 l = normalize(lt[i].pos.xyz - p);
        float facing = max(0.0, dot(N, l));
        atmospheric += lt[i].dif.rgb * facing * 0.1;
    }
    building_color += atmospheric;
    
    frag_color = vec4(building_color, 1.0);
}