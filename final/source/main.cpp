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
#include "vec2d.hpp"

//****************************************************
// Main loop
//****************************************************
int main(int argc, char *argv[]) {
	Vec2d * vals[10000];
	glm::vec2 *vals2[10000];
	for (int i = 0; i < 10000; i++) {
		float f1 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		float f2 = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		vals[i] = new Vec2d(f1, f2);
		vals2[i] = new glm::vec2(f1, f2);
	}
	float dotps[5000];
	DWORD dwStartTime = GetTickCount();
    DWORD dwElapsed;
    for (int j = 0; j < 10000; j++) {
	for (int i = 0; i <5000; i++) {
		dotps[i] = vals[2*i]->dot(*vals[(2*i)+1]);
	}
	}
    dwElapsed = GetTickCount() - dwStartTime;
    printf("It took %d.%3d seconds to complete\n", dwElapsed/1000, dwElapsed - dwElapsed/1000);
	float dotps2[5000];
	dwStartTime = GetTickCount();
    dwElapsed;
    for (int j = 0; j < 10000; j++) {
	for (int i = 0; i <5000; i++) {
		dotps2[i] = glm::dot(*vals2[2*i], *vals2[(2*i)+1]);
	}
	}
    dwElapsed = GetTickCount() - dwStartTime;
    printf("It took %d.%3d seconds to complete\n", dwElapsed/1000, dwElapsed - dwElapsed/1000);
	std::cin.ignore();
	return 0;
}

