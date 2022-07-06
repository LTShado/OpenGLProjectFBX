#version 330

in vec3 v_Position;
in vec3 v_Normal;
in vec2 v_TexCoords;

struct Material
{
	vec3 AmbientColor;
	vec3 DiffuseColor;
	vec3 SpecularColor;
	float Shininess;
};

uniform Materials
{
	Material u_Material;	
};

uniform vec3 u_CameraPosition;

uniform sampler2D u_DiffuseTexture;

// calcul du facteur diffus, suivant la loi du cosinus de Lambert
float Lambert(vec3 N, vec3 L)
{
	return max(0.0, dot(N, L));
}

// cosTheta = le resultat du produit scalaire (N.L, V.H etc...)
vec3 FresnelSchlick(vec3 f0, float cosTheta) {
	return f0 + (vec3(1.0) - f0) * pow(1.0 - cosTheta, 5.0);
}

// facteur de normalisation (modele plausible de Lewis)
float NormalisedPhong(float shininess)
{
	return (shininess + 1.0) / 2.0;
}

// calcul du facteur speculaire, methode de Phong
float Phong(vec3 N, vec3 L, vec3 V, float shininess)
{
	// reflexion du vecteur incident I (I = -L)
	// suivant la loi de ibn Sahl - Snell - Descartes
	vec3 R = reflect(-L, N);
	
	return (dot(N, L) > 0.0) ? NormalisedPhong(shininess) * pow(max(0.0, dot(R, V)), shininess) : 0.0;
}

// calcul du facteur speculaire, methode de Phong modifiee par Lafortune
float NormalisedPhongModified(float shininess)
{
	return (shininess + 2.0) / 2.0;
}

float PhongModified(vec3 N, vec3 L, vec3 V, float shininess)
{
	float NdotL = max(dot(N, L), 0.0);
	// reflexion du vecteur incident I (I = -L)
	// suivant la loi de ibn Sahl - Snell - Descartes
	vec3 R = reflect(-L, N);
	return NormalisedPhongModified(shininess) * pow(max(0.0, dot(R, V)), shininess) * NdotL;
}

// Calcule le F0 a utiliser en input de FresnelSchlick
// isolant : reflectance = 50% -> f0 = 4%
// conducteur (metaux) : reflectance = albedo (ou base color)
vec3 Reflectance(vec3 albedo, float metallic)
{
	return mix(vec3(0.04), albedo, metallic);
}

// Calcule la couleur speculaire du materiau selon qu'il soit metallique ou non
vec3 SpecularColor(vec3 albedo, float metallic)
{
	return mix(vec3(1.0), albedo, metallic);
}

// Calcule la couleur diffuse du materiau selon qu'il soit metallique ou non
vec3 DiffuseColor(vec3 albedo, float metallic)
{
	// on retourne 0.0 si metallic == true, l'albedo sinon
	return mix(albedo, vec3(0.0), metallic);
}

// Calcul un facteur de "glossiness/shininess" a partir de "roughness"
// roughness = 1.0 -> glossiness = 0.0
// roughness ~= 0.0 -> glossiness = tres tres elevee
float Glossiness(float roughness)
{
	roughness = max(roughness, 0.001);
	return (2.0 / (roughness * roughness)) - 2.0;
}


void main(void)
{
	// direction de notre lumiere directionnelle (fixe)
	// en repere main droite
	const vec3 L = normalize(vec3(0.0, 0.0, 1.0));
	const vec3 lightColor = vec3(1.0, 1.0, 1.0);
	const float attenuation = 1.0; // on suppose une attenuation faible ici
	// theoriquement, l'attenuation naturelle est proche de 1 / distance²

	// todo: en faire des parametres du materiaux
	const float metalness = 1.0; // 0 = isolant (non-metallique)
	const float roughness = 0.5; // 50% en perceptuel (todo: lineariser)

	vec3 N = normalize(v_Normal);
	vec3 V = normalize(u_CameraPosition - v_Position);

	vec4 baseTexel = texture2D(u_DiffuseTexture, v_TexCoords);
	// decompression gamma, les couleurs des texels ont ete specifies dans l'espace colorimetrique
	// du moniteur (en sRGB) il faut donc convertir en RGB lineaire pour que les maths soient corrects
	baseTexel.rgb = pow(baseTexel.rgb, vec3(2.2));
	vec3 baseColor = baseTexel.rgb;

	vec3 directColor = vec3(0.0);
	
	{
		vec3 albedo = baseColor * u_Material.DiffuseColor;
		// la couleur diffuse depend du type de materiaux et de l'orientation de la normale
		vec3 diffuseColor =  DiffuseColor(albedo, metalness) * Lambert(N, L);

		// la brillance ou clarte speculaire est calculee en fonction de la rugosite 
		float shininess = Glossiness(roughness);// u_Material.Shininess;
		vec3 specularColor = SpecularColor(albedo, metalness) * PhongModified(N, L, V, shininess);

		vec3 R = reflect(-L, N); // note: optimisation possible car deja calculee dans Phong

		// f0 correspond a la reflexivite max (chromatique, ou achromatique pour les isolants) vue de "face" (0 degre)
		vec3 f0 = Reflectance(albedo, metalness);
		// intensite speculaire reflechie en fonction du point de vue de l'observateur
		vec3 Fspec = FresnelSchlick(f0, dot(R, V));

		// Comme on ne doit pas generer plus d'energie que l'on en recoit, la composante diffuse
		// doit etre compensee en fonction de l'intensite du speculaire
		// note: c'est assez simple ici, mais ce n'est pas vraiment correct d'un point de vue physique
		//
		// intensite diffuse en fonction de la normale
		vec3 Fdiff = vec3(1.0) - FresnelSchlick(f0, dot(N, L));

		// la couleur directe finale est une somme ponderee
		vec3 color = (diffuseColor * Fdiff + specularColor * Fspec);

		directColor += color * lightColor * attenuation;
	}

	// la couleur ambiante traduit une approximation de l'illumination indirecte de l'objet
	vec3 ambientColor = baseColor * u_Material.AmbientColor;
	vec3 indirectColor = ambientColor;

	vec3 color = directColor + indirectColor;
	
	// compression gamma (pas necessaire ici car glEnable(GL_FRAMEBUFFER_SRGB) est deja present dans le code)
	//color = pow(color, vec3(1.0 / 2.2));


	gl_FragColor = vec4(color, 1.0);
}