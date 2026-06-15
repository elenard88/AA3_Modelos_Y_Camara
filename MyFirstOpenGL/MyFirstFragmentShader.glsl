#version 440 core

uniform vec2 windowSize;
uniform sampler2D textureSampler;
uniform vec4 tintColor;

in vec2 uvsFragmentShader;
in vec3 normalsFragmentShader;
in vec4 primitivePosition;

out vec4 fragColor;

void main() {

    vec2 adjustedTexCoord = vec2(uvsFragmentShader.x, 1.0 - uvsFragmentShader.y);
    vec4 baseColor = texture(textureSampler, adjustedTexCoord);

    vec3 sourceLight = vec3(0.0, 0.0, 1.0);
    vec3 lightDirection = normalize(sourceLight - primitivePosition.xyz);
    float sourceLightAngle = dot(normalize(normalsFragmentShader), lightDirection);

    sourceLightAngle = max(sourceLightAngle, 0.25);

    fragColor = vec4(baseColor.rgb * sourceLightAngle, baseColor.a) * tintColor;
}
