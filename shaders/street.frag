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

uniform vec3 ka;            /* object material ambient */
uniform vec3 kd;            /* object material diffuse */
uniform vec3 ks;            /* object material specular */
uniform float shininess;    /* object material shininess */

uniform sampler2D tex_color;   /* texture sampler for color */
uniform sampler2D tex_normal;   /* texture sampler for normal vector */

/*output variables*/
out vec4 frag_color;

float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

float hash2d(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// markings on the street
vec3 add_concrete_detail(vec2 uv) {
    vec2 grid_uv = fract(uv * vec2(3.0, 6.0));
    
    float joint_width = 0.02;
    float joint = step(grid_uv.x, joint_width) + step(grid_uv.y, joint_width);
    joint = clamp(joint, 0.0, 1.0);
    
    vec3 joint_color = vec3(0.08, 0.08, 0.09);
    vec3 concrete_color = vec3(0.16, 0.16, 0.17);
    
    return mix(concrete_color, joint_color, joint * 0.7);
}

vec3 asphalt_texture(vec2 uv) {
    vec3 asphalt_base = vec3(0.15, 0.15, 0.16);
    float noise = hash2d(floor(uv * 50.0)) * 0.1;
    asphalt_base += vec3(noise * 0.2);

    float wear = sin(uv.y * 30.0 + hash2d(vec2(floor(uv.x * 10.0), 0.0)) * 0.1) * 0.5 + 0.5;
    wear *= 0.1;

    float oil = step(0.995, hash2d(floor(uv * 5.0)));
    asphalt_base = mix(asphalt_base, vec3(0.1, 0.1, 0.12), oil * 0.3);
    
    return asphalt_base - vec3(wear * 0.05);
}


vec3 shading_phong(light li, vec3 e, vec3 p, vec3 n) {
    vec3 v = normalize(e - p);
    vec3 l = normalize(li.pos.xyz - p);
    vec3 r = normalize(reflect(-l, n));

    vec3 ambColor = ka * li.amb.rgb;
    vec3 difColor = kd * li.dif.rgb * max(0., dot(n, l));
    vec3 specColor = ks * li.spec.rgb * pow(max(dot(v, r), 0.), shininess);

    return ambColor + difColor + specColor;
}

//vec3 read_normal_texture()
//{
    //vec3 normal = texture(tex_normal, vtx_uv).rgb;
    //normal = normalize(normal * 2.0 - 1.0);
    //return normal;
//

void main()
{
    vec3 e = position.xyz;              //// eye position
    vec3 p = vtx_position;              //// surface position
    vec3 N = normalize(vtx_normal);     //// normal vector
    vec3 T = normalize(vtx_tangent);    //// tangent vector

    // getting asphalt textyre
    vec3 asphalt = asphalt_texture(vtx_uv);
    
    // getting road markings
    vec3 markings = add_concrete_detail(vtx_uv);
    
    // combining
    vec3 base_color = mix(asphalt, markings, step(0.1, length(markings)));
    
    // lighting
    vec3 total_light = vec3(0.0);
    for(int i = 0; i < lt_att[0]; i++) {
        total_light += shading_phong(lt[i], e, p, N);
    }
    vec3 final_color = base_color * (0.5 + total_light * 0.5);
    
    frag_color = vec4(final_color, 1.0);
}