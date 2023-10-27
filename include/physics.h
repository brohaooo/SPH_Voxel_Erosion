#ifndef PHYSICS_H
#define PHYSICS_H

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include <vector>
#include <unordered_map>



// this will inicate the beginning of the voxel field(x=y=z=0) in world space
extern const int voxel_x_origin = -1;
extern const int voxel_y_origin = -1;
extern const int voxel_z_origin = -1;
// this will adjust voxel size, the voxel size will be voxel_size_scale * 1
extern const float voxel_size_scale = 0.5;


// definition of the voxel
struct voxel {
    bool exist; // whether the voxel exists, if not, the density and color are meaningless
    float density;
    glm::vec4 color;
};
// definition of the field, contains a 3D array of voxels
class voxel_field {
public:
    std::vector<std::vector<std::vector<voxel>>> field;
    int x_size, y_size, z_size;
    voxel_field(int x, int y, int z) {
		x_size = x;
		y_size = y;
		z_size = z;
		field.resize(x);
		for (int i = 0; i < x; i++) {
			field[i].resize(y);
			for (int j = 0; j < y; j++) {
				field[i][j].resize(z);
				for (int k = 0; k < z; k++) {
					field[i][j][k].exist = false;
					field[i][j][k].density = 0.0f;
					field[i][j][k].color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				}
			}
		}
	}
	void set_voxel(int x, int y, int z, float density, glm::vec4 color) {
		field[x][y][z].exist = true;
		field[x][y][z].density = density;
		field[x][y][z].color = color;
	}
	void set_voxel(int x, int y, int z, voxel v) {
		field[x][y][z] = v;
	}
	voxel get_voxel(int x, int y, int z) {
		return field[x][y][z];
	}
	void clear_voxel(int x, int y, int z) {
		field[x][y][z].exist = false;
		field[x][y][z].density = 0.0f;
		field[x][y][z].color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	}
	void clear_all() {
		for (int i = 0; i < x_size; i++) {
			for (int j = 0; j < y_size; j++) {
				for (int k = 0; k < z_size; k++) {
					field[i][j][k].exist = false;
					field[i][j][k].density = 0.0f;
					field[i][j][k].color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				}
			}
		}
	}
	void print_field() {
		for (int i = 0; i < field.size(); i++) {
			for (int j = 0; j < field[i].size(); j++) {
				for (int k = 0; k < field[i][j].size(); k++) {
					if (field[i][j][k].exist) {
						std::cout << "1 ";
					}
					else {
						std::cout << "0 ";
					}
				}
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}
	}

    
};

// used when need to render the voxel in world space
glm::vec3 voxel_to_world(int x, int y, int z) {
    return glm::vec3(x * voxel_size_scale + voxel_x_origin, y * voxel_size_scale + voxel_y_origin, z * voxel_size_scale + voxel_z_origin);
};
// used when need to get the voxel index from world space(might be a float, which need to further check the closest int)
glm::vec3 world_to_voxel(glm::vec3 world) {
	return glm::vec3((world.x - voxel_x_origin) / voxel_size_scale, (world.y - voxel_y_origin) / voxel_size_scale, (world.z - voxel_z_origin) / voxel_size_scale);
};


// definition of the bounding barrier, contains the max and min coordinates of the box
// particles cannot go beyond the box
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