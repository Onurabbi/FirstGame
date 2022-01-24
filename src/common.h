#ifndef COMMON_H
#define COMMON_H

#define array_count(arr) (sizeof(arr) / sizeof((arr)[0]))

#define PI 3.14159265358979323846

#define swap_f(x,y) do  \
{						\
	auto temp = x;	    \
	x = y;				\
	y = temp;			\
} while(0)				

#endif