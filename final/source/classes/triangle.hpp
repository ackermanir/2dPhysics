#ifndef __TRIANGLE_H
#define __TRIANGLE_H

//OpenGL headers
#include <GL/glew.h>
#include <GL/glfw.h>
#include <glm.hpp>
#include <algorithm>
#include <vector>
//OpenCL
#include <CL/cl.h>

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

    // Find collisions and resolve between this and other
    bool collide(Triangle &other);

    bool aabbCollision(Triangle &other);
    bool isCollision(Triangle &other, glm::vec2 &colVec);
    bool findCollisionPt(Triangle &other, glm::vec2 colVec, glm::vec2 &colPt);
    //Static functions for collisions between triangles
    void handleCollisions(Triangle &other, glm::vec2 colPt, glm::vec2 sprVec);
    void bestEdge(glm::vec2 norm, glm::vec2 &chosenPt, glm::vec2 *edge);

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

typedef struct Tri_g
{
    cl_float2 verts[3];
    cl_float2 velocity;
    cl_float rot_vel;
    cl_float inv_mass;
    cl_uint vert_buf;
} Tri_g;


Tri_g createTriG(Triangle &t);

#endif
