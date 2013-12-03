#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <glfw.h>
//GLM matrices
#include <gtc/matrix_transform.hpp>
#include <gtx/transform.hpp>

#include "shaderLoad.hpp"
#else
#include <sys/time.h>
#endif

//OpenGL headers
#include <GL/glew.h>
#include <glm.hpp>


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

#include "classes/interaction.hpp"

#include "classes/triangle.hpp"
#include "classes/grid.hpp"

int numThreads = 1;
int divs = 80;
//TODO: replace it with unix-friendly timer
double timestamp(){
	struct timeval tv;
	gettimeofday (&tv, 0);
	return tv.tv_sec + 1e-6*tv.tv_usec;
}
#define glfwGetTime() timestamp()
//****************************************************
// Main loop
//****************************************************
int main(int argc, char *argv[]) {
	if (argc >= 3){
		numThreads = atoi(argv[1]);
		divs = atoi(argv[2]);
	} else { printf("Usage: ./as4 <numThreads> <divs>\nExample: ./as4 4 200\n\n"); }
	printf("numThreads: %d,  divs: %d\n",numThreads,divs);
	// std::cout << std::endl;
	
	//collection of triangles in scene
	std::vector<Triangle> tris = std::vector<Triangle>();
    #ifdef _WIN32
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
    #endif
    
    srand(6);
    glm::vec2 upLeft(-500.0f, 500.0f);
    glm::vec2 downRight(500.0f, -500.0f);
    // int divs = 400;
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
    #ifdef _WIN32
	GLuint mvpId = glGetUniformLocation(programID, "mvpMat");
    #endif
    
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
            double perEngineTime = engineTime * 1000;
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
        for (int i = 0; i < 1; i++) {
            gr.stepAll(stepTime);
            gr.initialSort();
            midEngine = glfwGetTime(); //timestamp again to see collisions
            gr.rebalance();
        }

        // Change fov, allowing user to move
        // Use - or 0 keys
        #ifdef _WIN32
        glm::mat4 proj = projection();
        #endif
        
        double endEngine = glfwGetTime();
        engineTime += endEngine - begEngine;
        midEngineTime += endEngine - midEngine;

        #ifdef _WIN32
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
        #endif
        
	} // Check if the ESC key was pressed or the window was closed
    #ifdef _WIN32
	while( glfwGetKey(GLFW_KEY_ESC) != GLFW_PRESS &&
		   glfwGetWindowParam(GLFW_OPENED));
           
	glfwTerminate(); // Close OpenGL window and terminate GLFW
	glDeleteVertexArrays(1, &VertexArrayID); // Cleanup VBO
    #else
    while(true);
    #endif
	return 0;
}
