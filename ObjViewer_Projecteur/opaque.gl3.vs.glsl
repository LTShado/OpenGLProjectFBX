#version 330

in vec3 a_Position;
in vec3 a_Normal;
in vec2 a_TexCoords;

uniform mat4 u_WorldMatrix;

uniform Matrices 
{
	mat4 u_ViewMatrix;
	mat4 u_ProjectionMatrix;
};

out vec3 v_Position;
out vec3 v_Normal;
out vec2 v_TexCoords;

void main(void)
{
	v_TexCoords = a_TexCoords;

	v_Position = vec3(u_WorldMatrix * vec4(a_Position, 1.0));
	
	// note: techniquement il faudrait passer une normal matrix du C++ vers le GLSL
	// pour les raisons que l'on a vu en cours. A defaut on pourrait la calculer ici
	// normalMatrix = (World^-1)^T 	
	mat3 NormalMatrix = transpose(inverse(mat3(u_WorldMatrix)));
	v_Normal = NormalMatrix * a_Normal;

	gl_Position = u_ProjectionMatrix * u_ViewMatrix * u_WorldMatrix * vec4(a_Position, 1.0);
}