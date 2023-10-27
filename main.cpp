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
glm::vec4 cube_color = glm::vec4(0.4f, 0.4f, 1.f, 1.0f);
glm::vec4 cube_edge_color = glm::vec4(0.8f, 0.8f, 1.f, 1.0f);
glm::vec4 boundary_color = glm::vec4(0.2f, 0.2f, 0.f, 1.0f);


// boundary
const GLfloat x_max = 3.0f, x_min = -3.0f, y_max = 3.0f, y_min = -3.0f, z_max = 3.0f, z_min = -3.0f;
bounding_box boundary = bounding_box(x_max, x_min, y_max, y_min, z_max, z_min);

// voxel field
int x_num = 3, y_num = 3, z_num = 3;
voxel_field V = voxel_field(x_num, y_num, z_num);


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
void set_up_cube_base(unsigned int cube_VBO[2], unsigned int cube_VAO[2]) {
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

void set_up_boundary(unsigned int bound_VBO[2], unsigned int bound_VAO[2], bounding_box& boundary) {
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
    glPointSize(4.0);
    glLineWidth(2.0);
    // modify camera infos before render loop starts
    camera.MovementSpeed = 1.0f;
    camera.Front = glm::vec3(-0.373257, -0.393942, -0.826684);





    // build and compile our shader program
    // ------------------------------------
    Shader ourShader("../../../shader/shader.vs", "../../../shader/shader.fs");
    

    

    // scene building----------------------

    // set up coordinate axes to render
    unsigned int coordi_VBO, coordi_VAO;
    set_up_CoordinateAxes(coordi_VBO, coordi_VAO);

    // set up basic cube
    unsigned int cube_VBO[2];
    unsigned int cube_VAO[2];
    set_up_cube_base(cube_VBO, cube_VAO);


    // set up boundary
    unsigned int bound_VBO[2], bound_VAO[2];
    set_up_boundary(bound_VBO, bound_VAO, boundary);


    // set up voxel field
    set_up_voxel_field(V);



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
    io.Fonts->AddFontFromFileTTF("../../../resource/fonts/Cousine-Regular.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("../../../resource/fonts/DroidSans.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("../../../resource/fonts/Karla-Regular.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("../../../resource/fonts/ProggyClean.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    io.Fonts->AddFontFromFileTTF("../../../resource/fonts/Roboto-Medium.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesDefault());
    // --------------------------------
    
    // render loop
    while (!glfwWindowShouldClose(window))
    {
        
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        LastTime+=deltaTime;

        // input
        // -----
        processInput(window);

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




        // --------------------------------

        // print camera info
        std::cout<<"camera pos:"<<camera.Position[0]<<" "<<camera.Position[1]<<" "<<camera.Position[2]<<std::endl;
        std::cout<<"camera front:"<<camera.Front[0]<<" "<<camera.Front[1]<<" "<<camera.Front[2]<<std::endl;
    

    



        // imgui---------------------------
        // 1. ImGui render
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 2. UI准备（这里添加了一个ImGui默认提供的界面，可选）
        bool bShowDemoWindow = true;// 控制UI是否渲染
        // 显示ImGui自带的demo window
        ImGui::ShowDemoWindow(&bShowDemoWindow);

        // 3. 渲染
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
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
        // nothing yet, maybe add some functions later for space key
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