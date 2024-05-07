#version 150

#define MAX_LIGHTS                      48 /* Must match up with BRender */
#define ENABLE_PSX_SIMULATION           0

struct br_light
{
    vec4 position;    /* (X, Y, Z, 1) */
    vec4 direction;   /* (X, Y, Z, 0), normalised */
    vec4 half_;       /* (X, Y, Z, 0), normalised */
    vec4 colour;      /* (R, G, B, 0), normalised */
    vec4 iclq;        /* (intensity, constant, linear, attenutation) */
    vec2 spot_angles; /* (inner, outer), if (0.0, 0.0), then this is a point light. */
};

layout(std140) uniform br_scene_state
{
    vec4 eye_view; /* Eye position in view-space */
    br_light lights[MAX_LIGHTS];
    uint num_lights;
};

layout(std140) uniform br_model_state
{
    mat4 model_view;
    mat4 projection;
    mat4 mvp;
    mat4 normal_matrix;
    mat4 environment;
    mat4 map_transform;
    vec4 surface_colour;
    vec4 clear_colour;
    vec4 eye_m; /* Eye position in model-space */

    float ka; /* Ambient mod */
    float ks; /* Specular mod (doesn't seem to be used by Croc) */
    float kd; /* Diffuse mod */
    float power;
    uint unlit; /* Is this surface unlit? */
    int uv_source;
    bool disable_colour_key;
};

in vec3 aPosition;
in vec2 aUV;
in vec3 aNormal;
in vec4 aColour;

out vec4 position;
out vec4 normal;
out vec2 uv;
out vec4 colour;

out vec3 rawPosition;
out vec3 rawNormal;

#if ENABLE_PSX_SIMULATION
vec4 PSXify_pos(in vec4 vertex, in vec2 resolution)
{
    vec4 snappedPos = vertex;
    snappedPos.xyz = vertex.xyz / vertex.w;
    snappedPos.xy = floor(resolution.xy * snappedPos.xy) / resolution;
    snappedPos.xyz *= vertex.w;
    return snappedPos;
}
#endif

void main()
{
    vec4 pos = vec4(aPosition, 1.0);

    position = model_view * pos;
    normal = vec4(normalize(mat3(normal_matrix) * aNormal), 0);
    uv = aUV;
    colour = aColour;

    rawPosition = aPosition;
    rawNormal   = aNormal;

#if ENABLE_PSX_SIMULATION
    pos = PSXify_pos(mvp * pos, vec2(200.0, 150.0));
#else
    pos = mvp * pos;
#endif

    gl_Position = pos;
}
