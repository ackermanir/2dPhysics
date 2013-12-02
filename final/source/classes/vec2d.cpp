#include "vec2d.hpp"
#include "xmmintrin.h"
#include "pmmintrin.h"


Vec2d::Vec2d(float x, float y)
{
	this->x = x;
	this->y = y;
}

float Vec2d::dot(Vec2d &v2)
{
	float vals1[4] = {x, y, 0, 0};
	float vals2[4] = {v2.x, v2.y, 0, 0};
	__m128 mm1 = _mm_load_ps(vals1);
	__m128 mm2 = _mm_load_ps(vals2);
	mm1 = _mm_mul_ps(mm1, mm2);
	mm1 = _mm_hadd_ps(mm1, mm1);
	_mm_store_ss(vals1, mm1);
	return vals1[0];
}

Vec2d Vec2d::norm(void)
{
	return *this;
}

float Vec2d::length(void)
{
	return 0.0;
}


Vec2d::~Vec2d(void)
{
}
