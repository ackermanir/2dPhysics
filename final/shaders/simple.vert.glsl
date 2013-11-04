#version 330 core

//positions in model coords
layout(location = 0) in vec2 worldPos;

//Passed in constants for object
uniform mat4 mvpMat;

void main(){
    // Output position of the vertex, in clip space : MVP * position
	gl_Position = mvpMat * vec4(worldPos, 0, 1);
}
