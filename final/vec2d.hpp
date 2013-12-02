#ifndef __VEC2D_H
#define __VEC2D_H

class Vec2d
{
	float x;
	float y;
public:
	Vec2d(float x, float y);
	float dot(Vec2d &v2);
	Vec2d norm(void);
	float length(void);
	~Vec2d(void);
};

#endif