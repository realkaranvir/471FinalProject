#version 330 core

uniform sampler2D Texture0;
uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;

in vec2 vTexCoord;
in vec3 fragNor;
in vec3 lightDir;
in vec3 EPos;

out vec4 Outcolor;

void main() {
	vec4 texColor0 = texture(Texture0, vTexCoord);
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);
	float dC = max(dot(normal, light), 0.0);
	float specular = 0.0;
	if(dC > 0.0) {
		vec3 V = normalize(-EPos);
		vec3 H = normalize(light + V); // halfway vector
		float specAngle = max(dot(normal, H), 0.0);
		specular = pow(specAngle, MatShine);
	}
	vec3 lighting = MatAmb + (dC*MatDif) + (specular*MatSpec);
	vec3 finalColor = texColor0.rgb * lighting;
	Outcolor = vec4(finalColor, texColor0.a);
}