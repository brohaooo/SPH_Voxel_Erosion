#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h>
#include <camera.h>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <chrono>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <physics.h>

const bool _vSync = true; // Enable vsync

// input callback functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 1080;
const unsigned int SCR_HEIGHT = 720;

// camera 
Camera camera(glm::vec3(1.39092f, 1.55529f, 2.59475f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;
float LastTime = 0.0f;

// pre-defined colors
glm::vec4 red = glm::vec4(1.f, 0.f, 0.f, 1.0f);
glm::vec4 green = glm::vec4(0.f, 1.f, 0.f, 1.0f);
glm::vec4 blue = glm::vec4(0.f, 0.f, 1.f, 1.0f);
glm::vec4 black = glm::vec4(0.f, 0.f, 0.f, 1.0f);
glm::vec4 cube_color = glm::vec4(0.4f, 0.4f, 1.f, 1.0f);
glm::vec4 cube_edge_color = glm::vec4(0.8f, 0.8f, 1.f, 1.0f);
glm::vec4 boundary_color = glm::vec4(0.2f, 0.2f, 0.f, 1.0f);
glm::vec4 particle_color = glm::vec4(0.8f, 0.9f, 0.f, 1.0f);



// boundary, see details in physics.h
extern const GLfloat x_max, x_min, y_max, y_min, z_max, z_min;
bounding_box boundary = bounding_box(x_max, x_min, y_max, y_min, z_max, z_min);

// voxel field
int x_num = 3, y_num = 3, z_num = 3;
voxel_field V = voxel_field(x_num, y_num, z_num);

#define PARTICLE_NUM 200

// particle set
std::vector<particle> particles(PARTICLE_NUM);

// particle simulation parameters
bool time_stop = true;
bool regenerate = false;
// tracking space key press
bool isSpaceKeyPressed = false;

// set up voxel field
void set_up_voxel_field(voxel_field& V) {
    voxel v1;
    v1.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    v1.density = 1.0f;
    v1.exist = true;

    V.set_voxel(0, 0, 0, v1);
    V.set_voxel(0, 0, 1, v1);
    V.set_voxel(0, 1, 0, v1);
    V.set_voxel(1, 0, 0, v1);
}
	
// set up particle system
void set_up_SPH_particles(std::vector<particle> & P) {
    particle p1;
	// p1.currPos = generateRandomVec3();
	p1.prevPos = glm::vec3(0.0f, 0.0f, 0.0f);
	p1.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	p1.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
	p1.pamameters = glm::vec3(0.0f, 0.0f, 0.0f);
	p1.deltaCs = glm::vec3(0.0f, 0.0f, 0.0f);
	
    for (int i = 0; i < PARTICLE_NUM; i++) {
        p1.currPos = generateRandomVec3();
		P[i] = p1;
	}

}






// set up coordinate axes, return VBO and VAO reference
void set_up_CoordinateAxes(unsigned int & coordi_VBO, unsigned int & coordi_VAO) {
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // three lines: x, y, z
    GLfloat xyz_axis[] = {
     0.f, 0.f, 0.f,
     1.f, 0.f, 0.f,
     0.f, 0.f, 0.f,
     0.f, 1.f, 0.f,
     0.f, 0.f, 1.f,
    };
    glGenVertexArrays(1, &coordi_VAO);
    glGenBuffers(1, &coordi_VBO);
    glBindVertexArray(coordi_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, coordi_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(xyz_axis), xyz_axis, GL_STATIC_DRAW);

    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void renderCoordinateAxes(Shader & ourShader, unsigned int & coordi_VBO, unsigned int & coordi_VAO) {
    // activate selected shader
    ourShader.use();
    glBindVertexArray(coordi_VAO);

    // get MVP matrix and set it
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.5f, 100.0f);
    ourShader.setMat4("projection", projection);
    glm::mat4 view = camera.GetViewMatrix();
    ourShader.setMat4("view", view);
    glm::mat4 model = glm::mat4(1.0f); 
    ourShader.setMat4("model", model);


    // draw xyz axis
    ourShader.setVec4("color", red);
    glDrawArrays(GL_LINES, 0, 2);
    ourShader.setVec4("color", green);
    glDrawArrays(GL_LINES, 2, 2);
    ourShader.setVec4("color", blue);
    glDrawArrays(GL_LINES, 4, 2);
}



// set up a basic cube with VBO and VAO
void set_up_cube_base_rendering(unsigned int cube_VBO[2], unsigned int cube_VAO[2]) {
    // a cube has six faces, each face has two triangles, each triangle has three vertices
    // so here's 6*2*3=36 vertices
    GLfloat cube_triangle_vertices[] = {
     // Front face
    -0.5f, -0.5f, 0.5f,
     0.5f, -0.5f, 0.5f,
     0.5f,  0.5f, 0.5f,
    -0.5f, -0.5f, 0.5f,
     0.5f,  0.5f, 0.5f,
    -0.5f,  0.5f, 0.5f,

    // Back face
    -0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,

    // Right face
     0.5f, -0.5f, 0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, 0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, 0.5f,

     // Left face
     -0.5f, -0.5f, -0.5f,
     -0.5f, -0.5f, 0.5f,
     -0.5f, 0.5f, -0.5f,
     -0.5f, 0.5f, -0.5f,
     -0.5f, -0.5f, 0.5f,
     -0.5f, 0.5f, 0.5f,

     // Top face
     -0.5f, 0.5f, 0.5f,
      0.5f, 0.5f, 0.5f,
      0.5f, 0.5f, -0.5f,
     -0.5f, 0.5f, 0.5f,
      0.5f, 0.5f, -0.5f,
     -0.5f, 0.5f, -0.5f,

     // Bottom face
     0.5f, -0.5f, 0.5f,
     -0.5f, -0.5f, 0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     -0.5f, -0.5f, 0.5f,
     -0.5f, -0.5f, -0.5f
    };

    GLfloat cube_edge_vertices[] = {
        // Front face
        -0.502f, -0.502f, 0.502f,
         0.502f, -0.502f, 0.502f,

         0.502f, -0.502f, 0.502f,
         0.502f,  0.502f, 0.502f,

         0.502f,  0.502f, 0.502f,
         -0.502f,  0.502f, 0.502f,

         -0.502f,  0.502f, 0.502f,
         -0.502f, -0.502f, 0.502f,
         // Back face
        -0.502f, -0.502f, -0.502f,
         0.502f, -0.502f, -0.502f,

         0.502f, -0.502f, -0.502f,
         0.502f,  0.502f, -0.502f,

         0.502f,  0.502f, -0.502f,
         -0.502f,  0.502f, -0.502f,

         -0.502f,  0.502f, -0.502f,
         -0.502f, -0.502f, -0.502f,
         // Right face
        0.502f, -0.502f, 0.502f,
        0.502f, -0.502f, -0.502f,

        0.502f,  0.502f, -0.502f,
        0.502f,  0.502f, 0.502f,
        // Left face
        -0.502f, -0.502f, 0.502f,
        -0.502f, -0.502f, -0.502f,
        
        -0.502f,  0.502f, -0.502f,
        -0.502f,  0.502f, 0.502f,
        
    };



    glGenVertexArrays(2, cube_VAO);
    glGenBuffers(2, cube_VBO);


    glBindVertexArray(cube_VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, cube_VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_triangle_vertices), cube_triangle_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    glBindVertexArray(cube_VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, cube_VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_edge_vertices), cube_edge_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

}

// render a single cube given transformation matrix 'model'
void render_cube(Shader& ourShader, unsigned int cube_VBO[2], unsigned int cube_VAO[2], glm::mat4 model = glm::mat4(1.0f)) {
    // activate selected shader
    ourShader.use();
    

    // get VP matrix and set it together with Model matrix
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.5f, 100.0f);
    ourShader.setMat4("projection", projection);
    glm::mat4 view = camera.GetViewMatrix();
    ourShader.setMat4("view", view);
    ourShader.setMat4("model", model);
    // draw cube mesh
    glBindVertexArray(cube_VAO[0]);
    ourShader.setVec4("color", cube_color);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // draw cube edge
    glBindVertexArray(cube_VAO[1]);
    ourShader.setVec4("color", cube_edge_color);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_LINES, 0, 24);
    

}


// set up particle rendering, instanced rendering
void set_up_particle_rendering(unsigned int& sphereVBO, unsigned int& sphereVAO, unsigned int& sphereEBO, unsigned int& particle_instance_VBO) {
    // generate sphere vertices
    std::vector<GLfloat> sphereVertices;
    float radius = 1.0f; // sphere radius
    int sectors = 16; // sphere resolution 1
    int stacks = 8; // sphere resolution 2

    int num = (stacks * 2) * sectors * 3;

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = glm::pi<float>() / 2.0f - i * glm::pi<float>() / stacks;
        float y = radius * sin(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = 2.0f * glm::pi<float>() * j / sectors;
            float x = radius * cos(stackAngle) * cos(sectorAngle);
            float z = radius * cos(stackAngle) * sin(sectorAngle);

            sphereVertices.push_back(x);
            sphereVertices.push_back(y);
            sphereVertices.push_back(z);
        }
    }

    //  generate sphere indices
    std::vector<GLuint> sphereIndices;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int top = i * (sectors + 1) + j;
            int bottom = top + sectors + 1;

            sphereIndices.push_back(top);
            sphereIndices.push_back(top + 1);
            sphereIndices.push_back(bottom);

            sphereIndices.push_back(bottom);
            sphereIndices.push_back(top + 1);
            sphereIndices.push_back(bottom + 1);
        }
    }


    // create and bind VBO and VAO
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    glGenBuffers(1, &particle_instance_VBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * sphereVertices.size(), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * sphereIndices.size(), sphereIndices.data(), GL_STATIC_DRAW);

    // define vertex attributes pointer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);


    glBindBuffer(GL_ARRAY_BUFFER, particle_instance_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * PARTICLE_NUM, NULL, GL_DYNAMIC_DRAW);// dynamic draw, update every frame
    glVertexAttribDivisor(1, 1);

    // set model matrix attribute pointer
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(1);

    
}

// set up sphere rendering, just one sphere, not instanced
void set_up_sphere_rendering(unsigned int & sphereVBO, unsigned int & sphereVAO, unsigned int & sphereEBO) {
    std::vector<GLfloat> sphereVertices;
    float radius = 1.0f;
    int sectors = 16; 
    int stacks = 8; 

    int num = (stacks * 2) * sectors * 3;

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = glm::pi<float>() / 2.0f - i * glm::pi<float>() / stacks;
        float y = radius * sin(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = 2.0f * glm::pi<float>() * j / sectors;
            float x = radius * cos(stackAngle) * cos(sectorAngle);
            float z = radius * cos(stackAngle) * sin(sectorAngle);

            sphereVertices.push_back(x);
            sphereVertices.push_back(y);
            sphereVertices.push_back(z);
        }
    }
    std::vector<GLuint> sphereIndices;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int top = i * (sectors + 1) + j;
            int bottom = top + sectors + 1;

            sphereIndices.push_back(top);
            sphereIndices.push_back(top + 1);
            sphereIndices.push_back(bottom);

            sphereIndices.push_back(bottom);            
            sphereIndices.push_back(top + 1);
            sphereIndices.push_back(bottom + 1);
        }
    }
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * sphereVertices.size(), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * sphereIndices.size(), sphereIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
}

// render a single sphere given transformation matrix 'model', didn't use in this project
void render_sphere(Shader& ourShader, unsigned int& sphere_VBO, unsigned int& sphere_VAO, unsigned int& sphereEBO, glm::mat4 model = glm::mat4(1.0f)) {
    // activate selected shader
    ourShader.use();
    glBindVertexArray(sphere_VAO);

    // get MVP matrix and set it
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.5f, 100.0f);
    ourShader.setMat4("projection", projection);
    glm::mat4 view = camera.GetViewMatrix();
    ourShader.setMat4("view", view);
    ourShader.setMat4("model", model);

    // draw sphere
    ourShader.setVec4("color", particle_color);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawElements(GL_TRIANGLES, 768, GL_UNSIGNED_INT, 0);
    
}

// render a single sphere given transformation matrix 'model', instanced rendering, would be more efficient
void render_sphere_instanced(Shader& ourShader, unsigned int& sphere_VAO, GLsizei intance_num, unsigned int& particle_instance_VBO, GLfloat* particle_vertices) {
    // activate selected shader
    ourShader.use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.5f, 100.0f);
    ourShader.setMat4("projection", projection);
    glm::mat4 view = camera.GetViewMatrix();
    ourShader.setMat4("view", view);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.10));
    ourShader.setMat4("model", model);

    glBindVertexArray(sphere_VAO);

    // update particle position
    glBindBuffer(GL_ARRAY_BUFFER, particle_instance_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * 3 * intance_num, particle_vertices);

    ourShader.setVec4("color", particle_color);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawElementsInstanced(GL_TRIANGLES, 768, GL_UNSIGNED_INT, 0, intance_num);

    glCullFace(GL_FRONT);
    model = glm::scale(model, glm::vec3(1.1));
    ourShader.setMat4("model", model);
    ourShader.setVec4("color", black);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
    glDrawElementsInstanced(GL_TRIANGLES, 768, GL_UNSIGNED_INT, 0, intance_num);
    glCullFace(GL_BACK);
}



void set_up_boundary_rendering(unsigned int bound_VBO[2], unsigned int bound_VAO[2], bounding_box& boundary) {
    glGenVertexArrays(2, bound_VAO);
    glGenBuffers(2, bound_VBO);
    glBindVertexArray(bound_VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, bound_VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*boundary.face_mesh.size(), boundary.face_mesh.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(bound_VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, bound_VAO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * boundary.face_edge.size(), boundary.face_edge.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


}

void render_boundary(Shader& ourShader, unsigned int bound_VBO[2], unsigned int bound_VAO[2]) {
    // activate selected shader
    ourShader.use();
    

    // get MVP matrix and set it
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.5f, 100.0f);
    ourShader.setMat4("projection", projection);
    glm::mat4 view = camera.GetViewMatrix();
    ourShader.setMat4("view", view);
    glm::mat4 model = glm::mat4(1.0f);
    ourShader.setMat4("model", model);

    //glDisable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    // draw boundary mesh
    glBindVertexArray(bound_VAO[0]);
    ourShader.setVec4("color", boundary_color);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    // draw boundary edge
    glBindVertexArray(bound_VAO[1]);
    ourShader.setVec4("color", cube_edge_color);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_LINES, 0, 24);
    glCullFace(GL_BACK);
    //glEnable(GL_CULL_FACE);

}


// render particles
void render_SPH_particles_x(std::vector<particle> & particles, Shader& ourShader, unsigned int & sphere_VBO, unsigned int & sphere_VAO, unsigned int& sphere_EBO) {
    GLfloat * particle_vertices = new GLfloat[particles.size() * 3];
    for(int i = 0; i < particles.size(); i++) {
		const particle p = particles[i];
		glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), p.currPos), glm::vec3(0.18));

        render_sphere(ourShader, sphere_VBO, sphere_VAO, sphere_EBO, model);
	}

}

// render particles
void render_SPH_particles(std::vector<particle>& particles, Shader& ourShader, unsigned int& sphere_VBO, unsigned int& sphere_VAO, unsigned int& sphere_EBO, unsigned int& particle_instance_VBO) {
    GLfloat* particle_vertices = new GLfloat[particles.size() * 3];
    for (int i = 0; i < particles.size(); i++) {
        const particle p = particles[i];
        particle_vertices[i * 3] = p.currPos[0];
        particle_vertices[i * 3 + 1] = p.currPos[1];
        particle_vertices[i * 3 + 2] = p.currPos[2];

    }
    render_sphere_instanced(ourShader, sphere_VAO, particles.size(),particle_instance_VBO,particle_vertices);
}


// render voxel field
void render_voxel_field(voxel_field& V, Shader& ourShader, unsigned int cube_VBO[2], unsigned int cube_VAO[2]) {

    for (int i = 0; i < x_num; i++) {
		for (int j = 0; j < y_num; j++) {
			for (int k = 0; k < z_num; k++) {
				voxel v = V.get_voxel(i, j, k);
				if (v.exist) {
					glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), voxel_to_world(i, j, k)),glm::vec3(voxel_size_scale));
                    render_cube(ourShader, cube_VBO, cube_VAO, model);
				}
			}
		}
	}
}


int main()
{

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE); // Enable double buffering


#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SPH_Voxel_Erosion", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    // set window position and size
    glfwMakeContextCurrent(window);

    glfwSwapInterval(_vSync ? 1 : 0); // Enable vsync

    // register callback functions
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glPointSize(8.0);
    glLineWidth(4.0);
    // modify camera infos before render loop starts
    camera.MovementSpeed = 1.0f;
    camera.Front = glm::vec3(-0.373257, -0.393942, -0.826684);





    // build and compile our shader program
    // ------------------------------------
    Shader ourShader("shader/shader.vs", "shader/shader.fs");
    Shader instance_shader("shader/shader_instance.vs", "shader/shader_instance.fs");
    

    

    // scene building----------------------

    // set up coordinate axes to render
    unsigned int coordi_VBO, coordi_VAO;
    set_up_CoordinateAxes(coordi_VBO, coordi_VAO);

    // set up basic cube
    unsigned int cube_VBO[2];
    unsigned int cube_VAO[2];
    set_up_cube_base_rendering(cube_VBO, cube_VAO);


    // set up boundary
    unsigned int bound_VBO[2], bound_VAO[2];
    set_up_boundary_rendering(bound_VBO, bound_VAO, boundary);


    // set up voxel field
    set_up_voxel_field(V);

    // set up particles
    set_up_SPH_particles(particles);

    unsigned int sphere_VBO, sphere_VAO, sphere_EBO, particle_instance_VBO;
    // set_up_sphere_rendering(sphere_VBO, sphere_VAO, sphere_EBO); 
    set_up_particle_rendering(sphere_VBO, sphere_VAO, sphere_EBO, particle_instance_VBO);

    // --------------------------------
    
    


    //imgui config----------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // set color theme
    ImGui::StyleColorsDark();
    // embed imgui into glfw and opengl3
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // OpenGL version 3.3
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    // font setting
    io.Fonts->AddFontFromFileTTF("resource/fonts/Cousine-Regular.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("resource/fonts/DroidSans.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("resource/fonts/Karla-Regular.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("resource/fonts/ProggyClean.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("resource/fonts/Roboto-Medium.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    // --------------------------------
    
    // render loop
    while (!glfwWindowShouldClose(window))
    {
        if (regenerate) {
			regenerate = false;
			set_up_SPH_particles(particles);
		}


        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float fps = 1.0f / deltaTime;

        LastTime+=deltaTime;

        

        // input
        // -----
        processInput(window);


        // do the physics calculation here, this will be the bottleneck of the program
        if (!time_stop) {
            calculate_SPH_movement(particles, deltaTime);
        }




        // render part is here
        // ------
        // clear screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 


        // call each render function here
        // ------------------------------
        renderCoordinateAxes(ourShader, coordi_VBO, coordi_VAO);

        glm::mat4 cube_position = glm::mat4(1.0f);

        // render_cube(ourShader, cube_VBO, cube_VAO, cube_position);
        // render_cube(ourShader, cube_VBO, cube_VAO, glm::translate(cube_position, glm::vec3(1.0f, 0.0f, 0.0f)));

        render_voxel_field(V, ourShader, cube_VBO, cube_VAO);

        render_boundary(ourShader, bound_VBO, bound_VAO);

        // render_SPH_particles(particles, ourShader, sphere_VBO, sphere_VAO, sphere_EBO);
        render_SPH_particles(particles, instance_shader, sphere_VBO, sphere_VAO, sphere_EBO, particle_instance_VBO);


        int debug_particle_index = 0;
        int d = debug_particle_index;


        // std::cout <<"pos"<< particles[d].currPos[0]<<" "<<          particles[d].currPos[1]<<" "<<          particles[d].currPos[2]<<std::endl;
        // std::cout <<"spd"<< particles[d].velocity[0] << " " <<      particles[d].velocity[1] << " " <<      particles[d].velocity[2] << std::endl;
        // std::cout <<"acc"<< particles[d].acceleration[0] << " " <<  particles[d].acceleration[1] << " " <<  particles[d].acceleration[2] << std::endl;

        // --------------------------------

        // print camera info
        // std::cout<<"camera pos:"<<camera.Position[0]<<" "<<camera.Position[1]<<" "<<camera.Position[2]<<std::endl;
        // std::cout<<"camera front:"<<camera.Front[0]<<" "<<camera.Front[1]<<" "<<camera.Front[2]<<std::endl;
        // std::cout<<"fps:"<<fps<<std::endl;

    



        // imgui---------------------------
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 250, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(240, 100), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("LOG", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("CAM POS: %.3f %.3f %.3f", camera.Position[0], camera.Position[1], camera.Position[2]);
            ImGui::Text("CAM DIR: %.3f %.3f %.3f", camera.Front[0], camera.Front[1], camera.Front[2]);
            ImGui::Text("CAM FOV: %.3f", camera.Zoom);
        }
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // --------------------------------
    




        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &coordi_VAO);
    glDeleteBuffers(1, &coordi_VBO);

    glDeleteVertexArrays(2, cube_VAO);
    glDeleteBuffers(2, cube_VBO);
    

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        // std::cout<<"W"<<std::endl;
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!isSpaceKeyPressed) {
            time_stop = !time_stop;
        }
        isSpaceKeyPressed = true;
    }
    else {
        isSpaceKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        regenerate = true;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
