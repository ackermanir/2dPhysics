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
	std::vector<Triangle *> tris = std::vector<Triangle *>();
	//Location of camera in the scene
	glm::vec3 camLoc(0.0f, 0.0f, 99.0f);
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



    //Testing line collision code
    // glm::vec2 tr1[3];
    // glm::vec2 tr2[3];

    // tr1[0] = glm::vec2(-5.0f, 0.0f);
    // tr1[1] = glm::vec2(-2.0f, 3.0f);
    // tr1[2] = glm::vec2(0.0f, 0.0f);

    // tr2[0] = glm::vec2(-0.2f, 3.0f);
    // tr2[1] = glm::vec2(-0.05f, 3.0f);
    // tr2[2] = glm::vec2(-0.1f, -0.5f);
    // Triangle *t = new Triangle(tr1[0], tr1[1], tr1[2]);
    // t->invMass = 0.0f;
    // tris.push_back(t);
    // t = new Triangle(tr2[0], tr2[1], tr2[2]);
    // t->invMass = 1.0f;
    // tris.push_back(t);

    // Triangle *t = new Triangle(glm::vec2(.2,5), 5.0);
    // t->invMass = 1.0f;
    // tris.push_back(t);

    // Triangle *t2 = new Triangle(glm::vec2(0,11), 5.0);
    // t2->invMass = 1.0f;
    // tris.push_back(t2);

    // Triangle base = Triangle(glm::vec2(-60.0f, -0.0f), glm::vec2(60.0f, -0.0f), glm::vec2(0.0f, -120.0f));

    //Testing line collision code


    srand(6);
    glm::vec2 upLeft(-50.0f, 50.0f);
    glm::vec2 downRight(50.0f, -50.0f);;
    int divs = 10;
    float width = (downRight[0] - upLeft[0]) / divs / 2.0f;
    for (int i = 0; i < divs; i++) {
        float port = (downRight[0] - upLeft[0]) / divs;
        for (int j = 0; j < divs; j++) {
            float centerX = upLeft[0] + port * i + (float)rand() / RAND_MAX;
            float centerY = downRight[1] + port * j + (float)rand() / RAND_MAX;
            glm::vec2 center(centerX, centerY);
            Triangle *t = new Triangle(center, width);
            t->invMass = 1.0f;
            tris.push_back(t);
        }
    }

    //Base triangle
    Triangle base = Triangle(glm::vec2(-60.0f, -53.0f), glm::vec2(60.0f, -53.0f), glm::vec2(0.0f, -120.0f));
    // Inverse mass of 0 means infinite mass a.k.a. static object
    base.invMass = 0.0f;
	std::vector<Triangle *> statics = std::vector<Triangle *>();
    statics.push_back(&base);

    Grid gr(tris, statics, width);

	//****************************************************
	// Setup uniforms
	//****************************************************
	GLuint mvpId = glGetUniformLocation(programID, "mvpMat");

	//time keeping
	double oldTime = glfwGetTime();
	double lastDispTime = glfwGetTime();
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
        if ( numFrames > 9 ){
            double perEngineTime = engineTime / 10 * 1000;
            std::cout << " Engine: " << perEngineTime << "ms per frame\n";
            engineTime = 0.0;
            numFrames = 0;
            lastDispTime = curTime;
        }
        //****************************************************
        // Physics engine
        //****************************************************
        double begEngine = glfwGetTime();

        //Simulation/collision
        //bad at 0.001f barely
        // bad at 0.0005f;
        // seemed ok at 0.0001f;
        float stepTime = 0.0005f;
        for (int i = 0; i < 6; i++) {
            gr.rebalance(stepTime);
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

        for (unsigned int i = 0; i < statics.size(); i++) {
            statics[i]->drawSelf();
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
