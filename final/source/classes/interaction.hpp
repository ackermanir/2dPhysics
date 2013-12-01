#ifndef __INTERACTION_H
#define __INTERACTION_H

#include <glm.hpp>
#include <cmath>

#include <glm.hpp>
#include <gtx/transform.hpp>
#include <gtc/matrix_transform.hpp>

//GLFW_KEY_KP_SUBTRACT
// or GLFW_KEY_KP_ADD
glm::mat4 projection(void) {
	static float fov = 45.0f;
	float step = 1.0f;

	if (glfwGetKey( '0' ) == GLFW_PRESS){
		if (fov > 0.001f + step) {
			fov -= step;
		}
	}
	if (glfwGetKey( '-' ) == GLFW_PRESS){
		if (fov < 179.999 - step) {
			fov += step;
		}
	}
	// Projection matrix : FOV, ratio, near, far
	return glm::perspective(fov, 4.0f / 3.0f, 0.1f, 1000.0f);
}

#endif
