#ifndef RENDER_H
#define RENDER_H


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
#include <physics.h>

#include <shader.h>
#include <camera.h>


// defined in main.cpp
extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;
extern Camera camera;
extern int voxel_x_num, voxel_y_num, voxel_z_num;
extern const int particle_num;

// defined in physics.h
extern const GLfloat x_max, x_min, y_max, y_min, z_max, z_min;
extern const float voxel_size_scale;

// pre-defined colors
glm::vec4 red = glm::vec4(1.f, 0.f, 0.f, 1.0f);
glm::vec4 green = glm::vec4(0.f, 1.f, 0.f, 1.0f);
glm::vec4 blue = glm::vec4(0.f, 0.f, 1.f, 1.0f);
glm::vec4 black = glm::vec4(0.f, 0.f, 0.f, 1.0f);
glm::vec4 cube_color = glm::vec4(0.4f, 0.4f, 1.f, 1.0f);
glm::vec4 cube_edge_color = glm::vec4(0.8f, 0.8f, 1.f, 1.0f);
glm::vec4 boundary_color = glm::vec4(0.2f, 0.2f, 0.f, 1.0f);
glm::vec4 particle_color = glm::vec4(0.8f, 0.9f, 0.f, 1.0f);

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
void set_up_SPH_particles(std::vector<particle>& P) {
    particle p1;
    // p1.currPos = generateRandomVec3();
    p1.prevPos = glm::vec3(0.0f, 0.0f, 0.0f);
    p1.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    p1.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    p1.pamameters = glm::vec3(0.0f, 0.0f, 0.0f);
    p1.deltaCs = glm::vec3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < P.size(); i++) {
        p1.currPos = generateRandomVec3();
        P[i] = p1;
    }

}






// set up coordinate axes, return VBO and VAO reference
void set_up_CoordinateAxes(unsigned int& coordi_VBO, unsigned int& coordi_VAO) {
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

void renderCoordinateAxes(Shader& ourShader, unsigned int& coordi_VBO, unsigned int& coordi_VAO) {
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * particle_num, NULL, GL_DYNAMIC_DRAW);// dynamic draw, update every frame
    glVertexAttribDivisor(1, 1);

    // set model matrix attribute pointer
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(1);


}

// set up sphere rendering, just one sphere, not instanced
void set_up_sphere_rendering(unsigned int& sphereVBO, unsigned int& sphereVAO, unsigned int& sphereEBO) {
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * boundary.face_mesh.size(), boundary.face_mesh.data(), GL_STATIC_DRAW);
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


// render particles, abandoned, because it's not efficient
void render_SPH_particles_x(std::vector<particle>& particles, Shader& ourShader, unsigned int& sphere_VBO, unsigned int& sphere_VAO, unsigned int& sphere_EBO) {
    GLfloat* particle_vertices = new GLfloat[particles.size() * 3];
    for (int i = 0; i < particles.size(); i++) {
        const particle p = particles[i];
        glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), p.currPos), glm::vec3(0.18));

        render_sphere(ourShader, sphere_VBO, sphere_VAO, sphere_EBO, model);
    }

}

// render particles, use instanced rendering
void render_SPH_particles(std::vector<particle>& particles, Shader& ourShader, unsigned int& sphere_VBO, unsigned int& sphere_VAO, unsigned int& sphere_EBO, unsigned int& particle_instance_VBO) {
    GLfloat* particle_vertices = new GLfloat[particles.size() * 3];
    for (int i = 0; i < particles.size(); i++) {
        const particle p = particles[i];
        particle_vertices[i * 3] = p.currPos[0];
        particle_vertices[i * 3 + 1] = p.currPos[1];
        particle_vertices[i * 3 + 2] = p.currPos[2];

    }
    render_sphere_instanced(ourShader, sphere_VAO, particles.size(), particle_instance_VBO, particle_vertices);
}


// render voxel field
void render_voxel_field(voxel_field& V, Shader& ourShader, unsigned int cube_VBO[2], unsigned int cube_VAO[2]) {

    for (int i = 0; i < voxel_x_num; i++) {
        for (int j = 0; j < voxel_y_num; j++) {
            for (int k = 0; k < voxel_z_num; k++) {
                voxel v = V.get_voxel(i, j, k);
                if (v.exist) {
                    glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), voxel_to_world(i, j, k)), glm::vec3(voxel_size_scale));
                    render_cube(ourShader, cube_VBO, cube_VAO, model);
                }
            }
        }
    }
}


#endif