#ifndef PHYSICS_H
#define PHYSICS_H

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include <vector>
#include <unordered_map>

#include <random>

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
    // strict physical boundary
	GLfloat x_max, x_min, y_max, y_min, z_max, z_min;
    // mesh for rendering, not the true physical boundary
	std::vector<GLfloat> face_mesh;
    std::vector<GLfloat> face_edge;
	bounding_box(GLfloat _x_max, GLfloat _x_min, GLfloat _y_max, GLfloat _y_min, GLfloat _z_max, GLfloat _z_min) {
		this->x_max = _x_max;
		this->x_min = _x_min;
		this->y_max = _y_max;
		this->y_min = _y_min;
		this->z_max = _z_max;
		this->z_min = _z_min;

        // add a small offset because the boundary particles are not exactly 'on' the boundary of the box
        // the true physical boundary is slightly smaller than the rendered box
        GLfloat render_x_max = x_max;
        GLfloat render_x_min = x_min;
        GLfloat render_y_max = y_max;
        GLfloat render_y_min = y_min;
        GLfloat render_z_max = z_max;
        GLfloat render_z_min = z_min;
        render_x_max += 0.11f;
        render_x_min -= 0.11f;
        render_y_max += 0.11f;
        render_y_min -= 0.11f;
        render_z_max += 0.11f;
        render_z_min -= 0.11f;
		
		face_mesh = {
            // Front face
            render_x_min, render_y_min, render_z_max,
            render_x_max, render_y_min, render_z_max,
            render_x_max, render_y_max, render_z_max,
            render_x_min, render_y_min, render_z_max,
            render_x_max, render_y_max, render_z_max,
            render_x_min, render_y_max, render_z_max,
                                        
            // Back face                
            render_x_min, render_y_min, render_z_min,
            render_x_max, render_y_max, render_z_min,
            render_x_max, render_y_min, render_z_min,
            render_x_max, render_y_max, render_z_min,
            render_x_min, render_y_min, render_z_min,
            render_x_min, render_y_max, render_z_min,
                                        
            // Right face               
            render_x_max, render_y_min, render_z_max,
            render_x_max, render_y_min, render_z_min,
            render_x_max, render_y_max, render_z_min,
            render_x_max, render_y_min, render_z_max,
            render_x_max, render_y_max, render_z_min,
            render_x_max, render_y_max, render_z_max,
                                        
            // Left face                
            render_x_min, render_y_min, render_z_min,
            render_x_min, render_y_min, render_z_max,
            render_x_min, render_y_max, render_z_min,
            render_x_min, render_y_max, render_z_min,
            render_x_min, render_y_min, render_z_max,
            render_x_min, render_y_max, render_z_max,
                                        
            // Top face                 
            render_x_min, render_y_max, render_z_max,
            render_x_max, render_y_max, render_z_max,
            render_x_max, render_y_max, render_z_min,
            render_x_min, render_y_max, render_z_max,
            render_x_max, render_y_max, render_z_min,
            render_x_min, render_y_max, render_z_min,

            // Bottom face
            render_x_max, render_y_min, render_z_max,
            render_x_min, render_y_min, render_z_max,
            render_x_max, render_y_min, render_z_min,
            render_x_max, render_y_min, render_z_min,
            render_x_min, render_y_min, render_z_max,
            render_x_min, render_y_min, render_z_min
		};
        GLfloat x_max_edge = render_x_max - 0.005f;
        GLfloat x_min_edge = render_x_min + 0.005f;
        GLfloat y_max_edge = render_y_max - 0.005f;
        GLfloat y_min_edge = render_y_min + 0.005f;
        GLfloat z_max_edge = render_z_max - 0.005f;
        GLfloat z_min_edge = render_z_min + 0.005f;

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

// definition of the bounding box... needed here
extern const GLfloat x_max = 1.0f, x_min = -1.0f, y_max = 1.0f, y_min = -1.0f, z_max = 1.0f, z_min = -1.0f;

// generate a random vec3 in the min and max range
glm::vec3 generateRandomVec3(float _x_max = x_max, float _x_min = x_min, float _y_max = y_max, float _y_min = y_min, float _z_max = z_max, float _z_min = z_min) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> distributionX(_x_min, _x_max);
    std::uniform_real_distribution<float> distributionY(_y_min, _y_max);
    std::uniform_real_distribution<float> distributionZ(_z_min, _z_max);

    float randomX = distributionX(rng);
    float randomY = distributionY(rng);
    float randomZ = distributionZ(rng);

    return glm::vec3(randomX * 0.9f, randomY * 0.9f, randomZ * 0.9f);
}


// particle system part:
// definition of the constants
#define PI_FLOAT 3.1415927410125732421875f
// extern const int particle_num = 10;
extern const float particle_radius = 0.025f;
extern const float particle_resting_density = 1000.0f;
extern const float particle_mass = 65.0f;
extern const float smoothing_length = 24.0f * particle_radius;
extern const float particle_viscosity = 250.0f;
extern const glm::vec3 gravity_force = glm::vec3(0.0f, -10.0f, 0.0f);
extern const float particle_stiffness = 200.0f;
extern const float wall_damping = 0.8f;



// definition of the particle
struct particle {
    glm::vec3  currPos;
    glm::vec3  prevPos;
    glm::vec3  acceleration;
    glm::vec3  velocity;
    glm::vec3  pamameters;// density, pressure, neighbor number
    glm::vec3  deltaCs;
};

void calculate_SPH_movement(std::vector<particle> & p, float frameTimeDiff) {
    int particle_num = p.size();
    // for each particle, calculate the density and pressure
    for (int i = 0; i < particle_num; i++) {
        int cnt = 0;
        float density_sum = 0.f;
        for (int j = 0; j < particle_num; j++) {
            glm::vec3 delta = (p[i].currPos - p[j].currPos);
            float r = length(delta);
            if (r < smoothing_length)
            {
                cnt++;
                density_sum += particle_mass * /* poly6 kernel */ 315.f * glm::pow(smoothing_length * smoothing_length - r * r, 3.f) / (64.f * PI_FLOAT * glm::pow(smoothing_length, 9));
            }
        }
        p[i].pamameters[0] = density_sum;
        p[i].pamameters[1] = glm::max(particle_stiffness * (density_sum - particle_resting_density), 0.f);
        p[i].pamameters[2] = float(cnt);
    }
    // for each particle, calculate the force and acceleration
    for (int i = 0; i < particle_num; i++) {
        glm::vec3 pressure_force = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 viscosity_force = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 dCs = glm::vec3(0.0f, 0.0f, 0.0f);
        for (int j = 0; j < particle_num; j++) {
            if (i == j) {
				continue;
			}
            glm::vec3 delta = (p[i].currPos - p[j].currPos);
            float r = length(delta);
            if (r < smoothing_length) {
                if (r == 0.0f) {
					// if the two particles are at the same position, add a small random delta to avoid NaN
                    delta = generateRandomVec3(0.0001f,-0.0001f, 0.0001f, -0.0001f, 0.0001f, -0.0001f);
				}
				// calculate the pressure force
                pressure_force -= particle_mass * (p[i].pamameters[1] + p[j].pamameters[1]) / (2.f * p[j].pamameters[0]) *
                    // gradient of spiky kernel
                    -45.f / (PI_FLOAT * glm::pow(smoothing_length, 6.f)) * glm::pow(smoothing_length - r, 2.f) * glm::normalize(delta);
				// calculate the viscosity force
                viscosity_force += particle_mass * (p[j].velocity - p[i].velocity) / p[j].pamameters[0] *
					// Laplacian of viscosity kernel
					45.f / (PI_FLOAT * glm::pow(smoothing_length, 6.f)) * (smoothing_length - r);
                
                dCs -= particle_mass * glm::pow(smoothing_length * smoothing_length - r * r, 2.f) / p[j].pamameters[0] *
                    // Poly6 kernel
                	945.f / (32.f * PI_FLOAT * glm::pow(smoothing_length, 9.f)) * delta;
			}
            
            


        }

        /*if (i == 0) {
                std::cout << i << "viscosity_force: " << viscosity_force.x << " " << viscosity_force.y << " " << viscosity_force.z << std::endl;
                std::cout << i << "pressure_force: " << pressure_force.x << " " << pressure_force.y << " " << pressure_force.z << std::endl;
        }*/


        viscosity_force *= particle_viscosity;
        p[i].acceleration = glm::vec3((pressure_force / p[i].pamameters[0] + viscosity_force / p[i].pamameters[0] + gravity_force));

        p[i].deltaCs = glm::vec3(glm::normalize(dCs));

        
    }
    // for each particle, calculate the velocity and new position
    for (int i = 0; i < particle_num; i++) {
        glm::vec3 new_velocity = (p[i].velocity + frameTimeDiff * p[i].acceleration);
		glm::vec3 new_position = p[i].currPos + frameTimeDiff * new_velocity;
        
        //new_velocity.y *= 0.9f;

        // check collision with the bounding box
        if (new_position.y < y_min)
        {
            new_position.y = y_min;
            new_velocity.y *= -1 * wall_damping;
        }
        else if (new_position.y > y_max)
        {
            new_position.y = y_max;
            new_velocity.y *= -1 * wall_damping;
        }
        if (new_position.x < x_min)
        {
            new_position.x = x_min;
            new_velocity.x *= -1 * wall_damping;
        }
        else if (new_position.x > x_max)
        {
            new_position.x = x_max;
            new_velocity.x *= -1 * wall_damping;
        }
        if (new_position.z < z_min)
        {
            new_position.z = z_min;
            new_velocity.z *= -1 * wall_damping;
        }
        else if (new_position.z > z_max)
        {
            new_position.z = z_max;
            new_velocity.z *= -1 * wall_damping;
        }


        p[i].velocity = new_velocity;
        p[i].prevPos = p[i].currPos;
        p[i].currPos = new_position;
      

    }





}







#endif