#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <iostream>

#include <vector>
#include <unordered_map>

#include <random>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <glm/gtx/hash.hpp>
#include <shader.h>
#include <camera.h>



// ----------------------------------------------------------------------global part------------------------------------------------------
// I need to use shader in DDA debugging, so I put it here
extern unsigned int  global_cube_VBO[2];
extern unsigned int  global_cube_VAO[2];
extern Shader* global_ourShader;


// ----------------------------------------------------------------------physic part------------------------------------------------------

// particle system part:
// definition of the constants
#define PI_FLOAT 3.1415927410125732421875f
// const int particle_num = 2000;
const float particle_radius = 0.025f;
const float particle_resting_density = 1000.0f;
const float particle_mass = 75.0f; // initial mass
const float particle_maximum_mass = 85.0f; // maximum mass, mass increase when the particle is taking mass from the voxel
const float smoothing_length = 20.0f * particle_radius;
const float particle_viscosity = 190.0f; //175
const glm::vec3 gravity_force = glm::vec3(0.0f, -10.0f, 0.0f);
const float particle_stiffness = 200.0f; // aka K
const float wall_damping = 0.8f;

const float voxel_destroy_density_threshold = 0.01f;
const float voxel_not_destroyable_min_density = 380000.0f;
const float voxel_damage_scale = 1.0f;
const float voxel_density = 300000.0f;
const float voxel_maximum_density = 400000.0f; // if bigger than this, the voxel will separate part of it into its upper voxel neighbour

const float particle_mass_transfer_ratio = 0.0005f; // the ratio of voxel mass vs particle mass ( delta V_mass / delta P_mass)
const float diffusion_rate = 0.00006f; // the rate that controls the diffusion of the mass from one particle to another particle

extern int current_particle_num; // can use this to control the number of particles in the system, actual particle number is min(particle_num,current_particle_num)

extern float particle_render_scale;


// this will inicate the beginning of the voxel field(x=y=z=0) in world space
extern const float voxel_x_origin;
extern const float voxel_y_origin;
extern const float voxel_z_origin;
// this will adjust voxel size, the voxel size will be voxel_size_scale * 1
extern const float voxel_size_scale;

// same as voxel_size_scale, but this will be used in speed up the particle calculation
extern const float neighbour_grid_size;


// definition of the voxel
struct voxel {
    bool exist; // whether the voxel exists, if not, the density and color are meaningless
    bool debug = false;
    bool not_destroyable = false;
    bool is_new = false;
    float density;
    glm::vec4 color;
    void update_color();
};
// definition of the field, contains a 3D array of voxels
class voxel_field {
public:
    std::vector<std::vector<std::vector<voxel>>> field;
    voxel NULL_VOXEL; // used when the voxel is out of bound, it is always not exist
    int x_size, y_size, z_size;
    voxel_field(int x, int y, int z);
    void set_voxel(int x, int y, int z, float density, glm::vec4 color);
    void set_voxel(int x, int y, int z, voxel v);
    voxel& get_voxel(int x, int y, int z);
    void clear_voxel(int x, int y, int z);

    void clear_all();
    void print_field();
};



// Neighborhood Search speed up part:// cell with size = smoothing_length
class neighbourhood_grid {
public:
    std::vector<std::vector<std::vector<std::vector<int>>>> grid;
    int x_size, y_size, z_size;
    neighbourhood_grid(int x, int y, int z);
    void add_particle(int x, int y, int z, int particle_index);
    void clear_grid();
    std::vector<int> world_to_grid(glm::vec3 world_pos);
    std::vector<int> get_neighbourhood(int x, int y, int z, int neighbood_range = 1);
    std::vector<int> get_upper_neighbourhood(int x, int y, int z, int neighbood_range = 1);
    std::vector<int> get_lower_neighbourhood(int x, int y, int z, int neighbood_range = 1);
};


// avoid some cases that the voxel index is out of bound
void check_voxel_index(int& x, int& y, int& z, voxel_field& V);

// used when need to render the voxel in world space
glm::vec3 voxel_to_world(int x, int y, int z);
// used in DDA, need to get the x positive face's world pos of the voxel
std::vector<float> voxel_to_world_6_face(int x, int y, int z)
;
// used when need to get the voxel index from world space(might be a float, which then converted to the int)
std::vector<int> world_to_voxel(glm::vec3 world, voxel_field& V);


// definition of the bounding barrier, contains the max and min coordinates of the box
// particles cannot go beyond the box
class bounding_box {
public:
    // strict physical boundary
    GLfloat x_max, x_min, y_max, y_min, z_max, z_min;
    // mesh for rendering, not the true physical boundary
    std::vector<GLfloat> face_mesh;
    std::vector<GLfloat> face_edge;
    bounding_box(GLfloat _x_max, GLfloat _x_min, GLfloat _y_max, GLfloat _y_min, GLfloat _z_max, GLfloat _z_min);

};

// definition of the bounding box... needed here
extern const GLfloat x_max, x_min, y_max, y_min, z_max, z_min;




// generate a random vec3 in the min and max range
glm::vec3 generateRandomVec3(float _x_max = x_max, float _x_min = x_min, float _y_max = y_max, float _y_min = y_min, float _z_max = z_max, float _z_min = z_min);

// definition of the particle
struct particle {
    glm::vec3  currPos;
    glm::vec3  prevPos;
    glm::vec3  acceleration;
    glm::vec3  velocity;
    glm::vec3  pamameters;// density, pressure, neighbor number
    glm::vec3  deltaCs;
    float      mass;
    glm::vec3  estimated_velocity = glm::vec3(0, 0, 0); // this is a more accurate velocity estimation, to check whether the particle is stopped 


    // use this to check whether the particle has been stucked for a while, if stuck, then it will be recycled
    // however, I didn't use this in this project till now, because I avoided stucking via other methods
    // but this might be useful in the future, I might activate stuck_count if some other stucking cases happen
    int		   stuck_count = 0;

};

void calculate_SPH_movement(std::vector<particle>& p, float frameTimeDiff, voxel_field& V, neighbourhood_grid& G, std::vector<int>& recycle_list);

void calculate_voxel_erosion(std::vector<particle>& p, float frameTimeDiff, voxel_field& V, neighbourhood_grid& G, std::vector<int>& recycle_list);



void recycle_particle(std::vector<particle>& p, std::vector<int>& recycle_list);


// ----------------------------------------------------------------------render part------------------------------------------------------
// defined in main.cpp
extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;
extern Camera camera;
extern int voxel_x_num, voxel_y_num, voxel_z_num;
extern const int particle_num;

// pre-defined colors
extern glm::vec4 red;
extern glm::vec4 dark_red;
extern glm::vec4 yellow;
extern glm::vec4 soil_color;
extern glm::vec4 green;
extern glm::vec4 blue;
extern glm::vec4 black;
extern glm::vec4 cube_color;
extern glm::vec4 cube_edge_color;
extern glm::vec4 boundary_color;
extern glm::vec4 particle_color;

// set up voxel field
void set_up_voxel_field(voxel_field& V, float voxel_density);
// set up particle system
void set_up_SPH_particles(std::vector<particle>& P);


// set up coordinate axes, return VBO and VAO reference
void set_up_CoordinateAxes(unsigned int& coordi_VBO, unsigned int& coordi_VAO);

void renderCoordinateAxes(Shader& ourShader, unsigned int& coordi_VBO, unsigned int& coordi_VAO);


// set up particle rendering, instanced rendering
void set_up_cube_base_instance_rendering(unsigned int cube_VBO[2], unsigned int cube_VAO[2], unsigned int& voxel_instance_VBO);
// set up a basic cube with VBO and VAO
void set_up_cube_base_rendering(unsigned int cube_VBO[2], unsigned int cube_VAO[2]);

// render a single cube given transformation matrix 'model'
void render_cube(Shader& ourShader, unsigned int cube_VBO[2], unsigned int cube_VAO[2], glm::mat4 model = glm::mat4(1.0f), glm::vec4 color = cube_color);


// set up particle rendering, instanced rendering
void set_up_particle_rendering(unsigned int& sphereVBO, unsigned int& sphereVAO, unsigned int& sphereEBO, unsigned int& particle_instance_VBO);

// set up sphere rendering, just one sphere, not instanced
void set_up_sphere_rendering(unsigned int& sphereVBO, unsigned int& sphereVAO, unsigned int& sphereEBO);

// render a single sphere given transformation matrix 'model', didn't use in this project
void render_sphere(Shader& ourShader, unsigned int& sphere_VBO, unsigned int& sphere_VAO, unsigned int& sphereEBO, glm::mat4 model = glm::mat4(1.0f));
// render a single sphere given transformation matrix 'model', instanced rendering, would be more efficient
void render_sphere_instanced(Shader& ourShader, unsigned int& sphere_VAO, GLsizei intance_num, unsigned int& particle_instance_VBO, GLfloat* particle_vertices);


void set_up_boundary_rendering(unsigned int bound_VBO[2], unsigned int bound_VAO[2], bounding_box& boundary);

void render_boundary(Shader& ourShader, unsigned int bound_VBO[2], unsigned int bound_VAO[2]);


// render particles, abandoned, because it's not efficient
void render_SPH_particles_x(std::vector<particle>& particles, Shader& ourShader, unsigned int& sphere_VBO, unsigned int& sphere_VAO, unsigned int& sphere_EBO);

// render particles, use instanced rendering
void render_SPH_particles(std::vector<particle>& particles, Shader& ourShader, unsigned int& sphere_VBO, unsigned int& sphere_VAO, unsigned int& sphere_EBO, unsigned int& particle_instance_VBO);


// render voxel field, not instanced rendering
void render_voxel_field_x(voxel_field& V, Shader& ourShader, unsigned int cube_VBO[2], unsigned int cube_VAO[2]);
// render voxel field, instanced rendering
void render_voxel_field(voxel_field& V, Shader& ourShader, unsigned int cube_VBO[2], unsigned int cube_VAO[2], unsigned int& voxel_instance_VBO);





#endif