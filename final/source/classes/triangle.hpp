#ifndef __TRIANGLE_H
#define __TRIANGLE_H

//OpenGL headers
#include <GL/glew.h>
#include <GL/glfw.h>
#include <glm.hpp>
#include <algorithm>
#include <vector>

#define HOOKE_CONSTANT 10.0f

class Collision;

class Triangle {
public:
    //Triangle vertices
    glm::vec2 verts[3];
	//Physics info
    glm::vec2 velocity;
    float rotationalVelocity;
    float invMass;
	//OpenGl info
	GLuint vertexBuf;

    //Create triangle with these 3 points
    Triangle(glm::vec2 &v0, glm::vec2 &v1, glm::vec2 &v2);
    //Equilateral triangle construct
    Triangle(glm::vec2 &center, float width);
    void init(glm::vec2 &v0, glm::vec2 &v1, glm::vec2 &v2);
    //Static functions for collisions between triangles
    static Collision * ptsColliding(Triangle *outerT, Triangle *innerT);
    static glm::vec2
    lineCollision(const glm::vec2 &a1, const glm::vec2 &a2,
                  const glm::vec2 &b1, const glm::vec2 &b2, bool &none);
    static void sidesColliding(Triangle *innerT, Triangle *outerT,
                               Collision *cols);
    static void handleCollisions(Collision * col);
    static bool isCollision(Triangle *triA, Triangle *triB, glm::vec2 &colVec);
    // Return all collisions between this and other triangle
    Collision * Triangle::testColliding(Triangle *other);
	void timeStep(float delta);
    glm::vec2 midPt(void);
    // OpenGL functions
	void glArraySetup(void);
	void glRedoVBO(void);
	void drawSelf();
    // Debug print
	void print(void);
    //Cleans up opengl buffer
	~Triangle(void);
};


class Collision {
public:
    // Vector of all collisions between t1 and t2
    // Pair is in form <collision pt, spring vector>
    std::vector<std::pair<glm::vec2, glm::vec2>> cols;
    //T1 is triangle collision is applied from
    Triangle * t1;
    Triangle * t2;

    Collision(Triangle * tr1, Triangle * tr2);
    void addCollision(glm::vec2 &point, glm::vec2 &dir);
    // Adds points from other collision with spring direction reversed
    void mergeReverse(Collision *other);
    void print(void);
};

#endif
