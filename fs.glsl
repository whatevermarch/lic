uniform sampler2D g_velocity; // RG32F
uniform sampler2D noise_tex; // RGBA8 200x200

uniform vec2 cell_dim;

const int traverse_len = 8;

smooth in vec2 vtexcoord;

layout(location = 0) out vec4 fcolor;

float applyLowPassFilter()
{
    //  accumulate forward along the streamline
    //  sample just r value from noise texture
    //  traverse using RK4 method
    vec2 initialX = vtexcoord * cell_dim;
    float intensity = texture(noise_tex, vec2(initialX.x / cell_dim.y, initialX.y / cell_dim.y)).r;
    float time_step = 0.2;

    //  forward
    vec2 lastX = initialX;
    for( int i = 0; i < traverse_len; i++ )
    {
        vec2 k1 = texture(g_velocity, lastX / cell_dim).rg;
        vec2 k2 = texture(g_velocity, (lastX + time_step * k1 / 2) / cell_dim).rg;
        vec2 k3 = texture(g_velocity, (lastX + time_step * k2 / 2) / cell_dim).rg;
        vec2 k4 = texture(g_velocity, (lastX + time_step * k3) / cell_dim).rg;
        lastX = lastX + time_step * ( k1 + 2 * k2 + 2 * k2 + k4 ) / 6;

        intensity += texture(noise_tex, vec2(lastX.x / cell_dim.y, lastX.y / cell_dim.y)).r;
    }
    //  backward
    lastX = initialX;
    for( int i = 0; i < traverse_len; i++ )
    {
        vec2 k1 = -texture(g_velocity, lastX / cell_dim).rg;
        vec2 k2 = -texture(g_velocity, (lastX + time_step * k1 / 2) / cell_dim).rg;
        vec2 k3 = -texture(g_velocity, (lastX + time_step * k2 / 2) / cell_dim).rg;
        vec2 k4 = -texture(g_velocity, (lastX + time_step * k3) / cell_dim).rg;
        lastX = lastX + time_step * ( k1 + 2 * k2 + 2 * k2 + k4 ) / 6;

        intensity += texture(noise_tex, vec2(lastX.x / cell_dim.y, lastX.y / cell_dim.y)).r;
    }

    //  calculate the mean
    intensity /= (traverse_len * 2 + 1);

    return intensity;
}

vec3 interpolateColor( float magnitude )
{
    if( magnitude < 1e-7 )
        return vec3(0.0);
    else if( magnitude < 0.5 )
        return vec3(0.1, 0.1, 1.0) * ( 0.5 - magnitude ) * 2 + vec3(0.1, 1.0, 0.1) * magnitude * 2;
    else
        return vec3(0.1, 1.0, 0.1) * ( 1.0 - magnitude ) * 2 + vec3(1.0, 0.1, 0.1) * ( magnitude - 0.5 ) * 2;
}

void main(void)
{
    float lic_color = applyLowPassFilter();
    float magnitude = length(texture(g_velocity, vtexcoord).rg) / 1.33;
    vec3 color = vec3(lic_color) * interpolateColor( magnitude );
    fcolor = vec4(color, 1.0);
}
