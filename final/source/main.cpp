#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

//OpenGL headers
#include <GL/glew.h>
#include <GL/glfw.h>
#include <glm.hpp>

//GLM matrices
#include <gtx/transform.hpp>
#include <gtc/matrix_transform.hpp>

#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <time.h>
#include <math.h>

using namespace std;

//Header for helpers
#include "shaderLoad.hpp"
#include "interaction.hpp"

#include "triangle.hpp"

//****************************************************
// Main loop
//****************************************************
int main(int argc, char *argv[]) {

	//collection of triangles in scene
	std::vector<Triangle *> tris = std::vector<Triangle *>();
	//Location of camera in the scene
	glm::vec3 camLoc(0.0f, 0.0f, 10.0f);;
    //Camera view matrix
    glm::mat4 view = glm::lookAt(camLoc, glm::vec3(0,0,0), glm::vec3(0,1,0));

	// Initialise GLFW
	if(!glfwInit()) {
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}

	glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	if( !glfwOpenWindow( 1350, 768, 0,0,0,0, 32,0, GLFW_WINDOW)) {
		fprintf( stderr, "Failed to open GLFW window\n");
		glfwTerminate();
		return -1;
	}

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	glfwSetWindowTitle( "2D Physics Simulator" );
	glfwEnable( GLFW_STICKY_KEYS );
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f); // Set background

	//Z-Buffer
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	//glEnable(GL_CULL_FACE);

	//Setup VAO
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	//****************************************************
	// Setup shader and textures
	//****************************************************
	GLuint programID = LoadShaders( "shaders/simple.vert.glsl",
									"shaders/simple.frag.glsl");

	//****************************************************
	// Load in objects to world
	//****************************************************

    //For now just generating some test triangles

    Triangle t = Triangle(glm::vec2(.00f, .4), glm::vec2(.4f, 2.0f), glm::vec2(1.0f, 0.0f));
    t.invMass = 1.0f;
    t.rotationalVelocity = 00.0f;
    tris.push_back(&t);

    Triangle t2 = Triangle(glm::vec2(-1.0f, -2), glm::vec2(0, .2f), glm::vec2(1.0f, -2));
    // Inverse mass of 0 means infinite mass a.k.a. static object
    t2.invMass = 0.0f;
    tris.push_back(&t2);

    Triangle t3 = Triangle(glm::vec2(1.05f, -2), glm::vec2(2.0f, 0.0f), glm::vec2(2.0f, -2));
    t3.invMass = 0.0f;
    tris.push_back(&t3);

	//****************************************************
	// Setup uniforms
	//****************************************************
	GLuint mvpId = glGetUniformLocation(programID, "mvpMat");

	//time keeping
	double oldTime = glfwGetTime();
	double lastFPSTime = glfwGetTime();
	double engineTime = 0.0;
	int numFrames = 0;
	do{
		//****************************************************
		// FPS / time calculations
		//****************************************************
		double curTime = glfwGetTime();
		float deltaTime = (float)(curTime - oldTime);
		oldTime = curTime;

		numFrames++;
		if ( curTime - lastFPSTime >= 1.0 ){
			double perEngineTime = 100 * engineTime;
			std::cout << 1000.0/double(numFrames) << " ms/frame.";
			std::cout << " Engine: " << perEngineTime << "%\n";
			engineTime = 0.0;
			numFrames = 0;
			lastFPSTime += 1.0;
		}
		//****************************************************
		// Physics engine
		//****************************************************
		double begEngine = glfwGetTime();

		//Simulation/collision
        // Divide step up further to allow better simulation
        int totalSubSteps = 10;
        for (int substep = 0; substep < totalSubSteps; substep++) {
            float stepTime = deltaTime / totalSubSteps;

            //Simulate triangles falling under gravity and rotating
            for (unsigned int i = 0; i < tris.size(); i++) {
                tris[i]->timeStep(stepTime);
            }

            // test collisions between all triangles
            // For now is an n^2 algorithm
            for (unsigned int i = 0; i < tris.size(); i++) {
                Triangle * tr1 = tris.at(i);
                for (unsigned int j = 0; j < i; j++) {
                    Triangle * tr2 = tris.at(j);
                    Collision * col = tr1->testColliding(tr2);
                    Triangle::handleCollisions(col);
                    // Here we cleanup collision as it was generated on heap
                    delete col;
                }
            }
        }

		// Change fov, allowing user to move
        // Use - or 0 keys
		glm::mat4 proj = projection();

		double endEngine = glfwGetTime();
		engineTime += endEngine - begEngine;

		//****************************************************
		// OpenGL rendering
		//****************************************************
		// Clear the screen and depth buffer
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Use our shader
		glUseProgram(programID);

        // Transform matrix only takes into account camera and perspective matrices
        glm::mat4 mvp = proj * view;
        glUniformMatrix4fv(mvpId, 1, GL_FALSE, &mvp[0][0]);

        //Go over each Triangle and have them draw themselves
		for (unsigned int i = 0; i < tris.size(); i++) {
			tris[i]->drawSelf();
		}

		// Swap buffers
		glfwSwapBuffers();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS &&
		   glfwGetWindowParam(GLFW_OPENED));

	glfwTerminate(); // Close OpenGL window and terminate GLFW
	glDeleteVertexArrays(1, &VertexArrayID); // Cleanup VBO

	return 0;
}
