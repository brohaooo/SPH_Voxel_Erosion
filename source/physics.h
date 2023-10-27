#ifndef PHYSICS_H
#define PHYSICS_H

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include <vector>
#include <unordered_map>


class voxel_object {
	


};

class bounding_box {
public:
	GLfloat x_max, x_min, y_max, y_min, z_max, z_min;
	std::vector<GLfloat> face_mesh;
    std::vector<GLfloat> face_edge;
	bounding_box(GLfloat x_max, GLfloat x_min, GLfloat y_max, GLfloat y_min, GLfloat z_max, GLfloat z_min) {
		this->x_max = x_max;
		this->x_min = x_min;
		this->y_max = y_max;
		this->y_min = y_min;
		this->z_max = z_max;
		this->z_min = z_min;
		
		face_mesh = {
            // Front face
            x_min, y_min, z_max,
            x_max, y_min, z_max,
            x_max, y_max, z_max,
            x_min, y_min, z_max,
            x_max, y_max, z_max,
            x_min, y_max, z_max,

            // Back face
            x_min, y_min, z_min,
            x_max, y_max, z_min,
            x_max, y_min, z_min,
            x_max, y_max, z_min,
            x_min, y_min, z_min,
            x_min, y_max, z_min,

            // Right face
            x_max, y_min, z_max,
            x_max, y_min, z_min,
            x_max, y_max, z_min,
            x_max, y_min, z_max,
            x_max, y_max, z_min,
            x_max, y_max, z_max,

            // Left face
            x_min, y_min, z_min,
            x_min, y_min, z_max,
            x_min, y_max, z_min,
            x_min, y_max, z_min,
            x_min, y_min, z_max,
            x_min, y_max, z_max,

            // Top face
            x_min, y_max, z_max,
            x_max, y_max, z_max,
            x_max, y_max, z_min,
            x_min, y_max, z_max,
            x_max, y_max, z_min,
            x_min, y_max, z_min,

            // Bottom face
            x_max, y_min, z_max,
            x_min, y_min, z_max,
            x_max, y_min, z_min,
            x_max, y_min, z_min,
            x_min, y_min, z_max,
            x_min, y_min, z_min
		};
        GLfloat x_max_edge = x_max - 0.005f;
        GLfloat x_min_edge = x_min + 0.005f;
        GLfloat y_max_edge = y_max - 0.005f;
        GLfloat y_min_edge = y_min + 0.005f;
        GLfloat z_max_edge = z_max - 0.005f;
        GLfloat z_min_edge = z_min + 0.005f;

        face_edge = {
            // Front face
            x_min_edge, y_min_edge, z_max_edge,
            x_max_edge, y_min_edge, z_max_edge,
            x_max_edge, y_min_edge, z_max_edge,
            x_max_edge, y_max_edge, z_max_edge,
            x_max_edge, y_max_edge, z_max_edge,
            x_min_edge, y_max_edge, z_max_edge,
            x_min_edge, y_max_edge, z_max_edge,
            x_min_edge, y_min_edge, z_max_edge,
            // Back face                
            x_min_edge, y_min_edge, z_min_edge,
            x_max_edge, y_min_edge, z_min_edge,
            x_max_edge, y_min_edge, z_min_edge,
            x_max_edge, y_max_edge, z_min_edge,
            x_max_edge, y_max_edge, z_min_edge,
            x_min_edge, y_max_edge, z_min_edge,
            x_min_edge, y_max_edge, z_min_edge,
            x_min_edge, y_min_edge, z_min_edge,
            // Right face                
            x_max_edge, y_min_edge, z_max_edge,
            x_max_edge, y_min_edge, z_min_edge,
            x_max_edge, y_max_edge, z_min_edge,
            x_max_edge, y_max_edge, z_max_edge,
            // Left face                
            x_min_edge, y_min_edge, z_max_edge,
            x_min_edge, y_min_edge, z_min_edge,
            x_min_edge, y_max_edge, z_min_edge,
            x_min_edge, y_max_edge, z_max_edge,

        };

	}
	 


};





#endif