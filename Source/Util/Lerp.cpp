#include "Lerp.h"

float Lerp(float a, float b, float f)
{
    return a + f * (b - a);
}
