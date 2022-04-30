#include "RandomFloat.h"

#include <cstdlib>

float RandomFloat()
{
    return ((float)rand() / (float)RAND_MAX);
}
