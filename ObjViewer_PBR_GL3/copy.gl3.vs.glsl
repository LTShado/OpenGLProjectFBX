#version 330

// quadrilatere ordonnee en N inverse pour un rendu TRIANGLE_STRIP
const vec2[4] _Position = vec2[4](vec2(-1.0, 1.0), vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(1.0, -1.0));

out vec2 v_UV;

void main(void)
{
	vec2 position = _Position[gl_VertexID];

	// conversion des positions en coordonnees de textures normalisees
	// suppose que les positions des sommets sont normalisees NDC	
	v_UV = position.xy * 0.5 + 0.5;
	gl_Position = vec4(position, 0.0, 1.0);
}