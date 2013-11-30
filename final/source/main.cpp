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
#include <cstdlib> //for srand
#include <omp.h>

#include "pthread.h"

using namespace std;

//Header for helpers
#include "shaderLoad.hpp"
#include "interaction.hpp"

#include "triangle.hpp"
#include "grid.hpp"

//****************************************************
// Main loop
//****************************************************
int main(int argc, char *argv[]) {

	//collection of triangles in scene
	std::vector<Triangle> tris = std::vector<Triangle>();
	//Location of camera in the scene
	glm::vec3 camLoc(0.0f, -250.0f, 750.0f);
    //Camera view matrix
    glm::mat4 view = glm::lookAt(camLoc, glm::vec3(0,-250.0f,0), glm::vec3(0,1,0));

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
	if( !glfwOpenWindow( 1366, 768, 0,0,0,0, 32,0, GLFW_WINDOW)) {
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
	glEnable(GL_CULL_FACE);

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

    srand(6);
    glm::vec2 upLeft(-500.0f, 500.0f);
    glm::vec2 downRight(500.0f, -500.0f);
    int divs = 80;
    float width = (downRight[0] - upLeft[0]) / divs / 3.0f;
    for (int i = 0; i < divs; i++) {
        float port = (downRight[0] - upLeft[0]) / divs;
        for (int j = 0; j < divs; j++) {
            float centerX = upLeft[0] + port * i + (float)rand() / RAND_MAX;
            float centerY = downRight[1] + port * j + (float)rand() / RAND_MAX;
            glm::vec2 center(centerX, centerY);
            Triangle t = Triangle(center, width);
            t.invMass = 1.0f;
            tris.push_back(t);
        }
    }

    //Base triangle
    Triangle base = Triangle(glm::vec2(-600.0f, -530.0f), glm::vec2(0.0f, -1200.0f), glm::vec2(600.0f, -530.0f));
    // Inverse mass of 0 means infinite mass a.k.a. static object
    base.invMass = 0.0f;
	std::vector<Triangle> statics = std::vector<Triangle>();
    statics.push_back(base);

    Grid gr(tris, statics, width * 2.0f);

	//****************************************************
	// Setup uniforms
	//****************************************************
	GLuint mvpId = glGetUniformLocation(programID, "mvpMat");

	//time keeping
	double engineTime = 0.0;
	double midEngineTime = 0.0;
	int numFrames = 0;

	do{
        //****************************************************
        // FPS / time calculations
        //****************************************************
        numFrames++;
        if ( numFrames > 10 ){
            double perEngineTime = engineTime / 10 * 1000;
            double midEng = midEngineTime / engineTime * 100;
            std::cout << " Engine: " << perEngineTime << "ms per frame\n";
            std::cout << " Collis: " << midEng << "% per frame\n\n";
            engineTime = 0.0;
            midEngineTime = 0.0;
            numFrames = 0;
        }
        //****************************************************
        // Physics engine
        //****************************************************
        double begEngine = glfwGetTime();
        double midEngine;

        //Simulation/collision
        // messy/bad at 0.001f with 900
        // ok at 0.0005f; up to 900
        float stepTime = 0.0005f;
        for (int i = 0; i < 10; i++) {
            gr.stepAll(stepTime);
            gr.initialSort();
            midEngine = glfwGetTime(); //timestamp again to see collisions
            gr.rebalance();
        }

        // Change fov, allowing user to move
        // Use - or 0 keys
        glm::mat4 proj = projection();

        double endEngine = glfwGetTime();
        engineTime += endEngine - begEngine;
        midEngineTime += endEngine - midEngine;

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
            tris[i].drawSelf();
        }

        for (unsigned int i = 0; i < statics.size(); i++) {
            statics[i].drawSelf();
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
