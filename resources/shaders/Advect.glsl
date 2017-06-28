#version 330 core

out vec4 FragColor;

uniform sampler2D VelocityTexture;
uniform sampler2D SourceTexture;

uniform vec2 InverseSize;
uniform float TimeStep;
uniform float Dissipation;

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;

    vec2 u = texture(VelocityTexture, InverseSize * fragCoord).xy;
    vec2 coord = InverseSize * (fragCoord - TimeStep * u);
    FragColor = Dissipation * texture(SourceTexture, coord);

//    if (fragCoord.x > 900.0f && fragCoord.x < 1000.0f &&
//        fragCoord.y > 500.0f && fragCoord.y < 600.0f) {
//        FragColor = vec4(100.0f, 0.0f, 0.0f, 0.0f);
//    }

//    FragColor = texture(VelocityTexture, fragCoord);

//    FragColor = vec4(1.0f, 0.0f, 0.0f, 0.0f);
}
