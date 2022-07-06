#pragma once

// M_PI est defini dans <math.h> et non plus dans <cmath>
// cependant il faut necessairement ajouter le define suivant 
// car M_PI n'est pas standard en C++
#define _USE_MATH_DEFINES 1
#include <math.h>		
#include <cstring>

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/type_ptr.hpp"