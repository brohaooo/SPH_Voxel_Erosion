#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <physics.h>
#include <render.h>

const bool _vSync = true; // Enable vsync

// input callback functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
extern const unsigned int SCR_WIDTH = 1080;
extern const unsigned int SCR_HEIGHT = 720;

// camera
extern Camera camera(glm::vec3(5.5f, 4.5f, 5.5f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;
float LastTime = 0.0f;

// boundary, see details in physics.h
extern const GLfloat x_max, x_min, y_max, y_min, z_max, z_min;
bounding_box boundary = bounding_box(x_max, x_min, y_max, y_min, z_max, z_min);

// voxel field
extern int voxel_x_num = 32, voxel_y_num = 32, voxel_z_num = 32;
int x_num = voxel_x_num, y_num = voxel_y_num, z_num = voxel_z_num;
voxel_field V = voxel_field(voxel_x_num, voxel_y_num, voxel_z_num);

extern const int particle_num = 200;

// particle set
std::vector<particle> particles(particle_num);

// particle simulation parameters
bool time_stop = true;
bool regenerate = false;
// tracking space key press
bool isSpaceKeyPressed = false;

int main()
{

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE); // Enable double buffering

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SPH_Voxel_Erosion", NULL, NULL);
    if (window == NULL) {
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
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
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

    // imgui config----------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // set color theme
    ImGui::StyleColorsDark();
    // embed imgui into glfw and opengl3
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // OpenGL version 3.3
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    // font setting
    io.Fonts->AddFontFromFileTTF("resource/fonts/Cousine-Regular.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("resource/fonts/DroidSans.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("resource/fonts/Karla-Regular.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("resource/fonts/ProggyClean.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("resource/fonts/Roboto-Medium.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    // --------------------------------

    // render loop
    while (!glfwWindowShouldClose(window)) {
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

        LastTime += deltaTime;

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
void processInput(GLFWwindow* window)
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
    } else {
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

    if (firstMouse) {
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
