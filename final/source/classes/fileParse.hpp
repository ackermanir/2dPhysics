#ifndef __FILEPARSE_H
#define __FILEPARSE_H

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

//GLM header
#include <glm.hpp>

//My classes
#include "object.hpp"

void readObjFile(std::string file, Object * objSpec) {
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;

	std::ifstream inpfile(file.c_str());
	if(!inpfile.is_open()) {
		std::cout << "Unable to open file" << std::endl;
	} else {
		std::cout << "Parsing obj input\n";
		std::string line;
		while(inpfile.good()) {
			std::vector<std::string> splitline;
			std::string buf;
			std::getline(inpfile,line);
			std::stringstream ss(line);
			while (ss >> buf) {
				splitline.push_back(buf);
			}
			//Ignore blank lines and comments
			if(splitline.size() == 0) {
				continue;
			} if(splitline[0][0] == '#') {
				continue;
			} else if(!splitline[0].compare("v") && splitline[0].length() == 1) {
				float x,y,z;
				x = (float)atof(splitline[1].c_str());
				y = (float)atof(splitline[2].c_str());
				z = (float)atof(splitline[3].c_str());
				glm::vec3 vert(x, y, z);
				vertices.push_back(vert);
			} else if(!splitline[0].compare("vn")) {
				float x,y,z;
				x = (float)atof(splitline[1].c_str());
				y = (float)atof(splitline[2].c_str());
				z = (float)atof(splitline[3].c_str());
				glm::vec3 norm(x, y, z);
				normals.push_back(norm);
			} else if(!splitline[0].compare("f")) {
				for (int i = 0; i < 3; i++) {
					std::string pt = splitline[i + 1];
					unsigned int slash = pt.find_first_of("/");
					int v = atoi(pt.substr(0, slash).c_str()) - 1;
					int n = atoi(pt.substr(slash + 2, pt.length() - 2 - slash).
								 c_str()) - 1;
					glm::vec3 vert = vertices.at(v);
					glm::vec3 norm = normals.at(n);
					objSpec->addVertex(vert, norm);
				}
			} else {
				std::cerr << "Unknown command: " << splitline[0] << std::endl;
			}
		}
		inpfile.close();
	}
};

#endif
