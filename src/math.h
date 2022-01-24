#ifndef MATH_H
#define MATH_H

#include <glm/glm.hpp>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

inline float length_sq(glm::vec2& a)
{
    return a.x*a.x + a.y*a.y;
}

inline void swap_float(float* a, float* b)
{
    float temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

struct rectangle2f
{
    glm::vec2 min;
    glm::vec2 max;
};


#endif