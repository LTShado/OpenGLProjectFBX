#pragma once

#include "mat4.h"

struct Material
{
	glm::vec3 ambientColor;
	float padding0;				// necessaire du fait de l'alignement des UBO
	glm::vec3 diffuseColor;
	float padding1;				// idem
	glm::vec3 specularColor;
	float shininess;

	uint32_t ambientTexture;	// optionnelle
	uint32_t diffuseTexture;
	uint32_t specularTexture;	// optionnelle

	static Material defaultMaterial;
};

