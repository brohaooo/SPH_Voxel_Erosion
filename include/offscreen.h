#ifndef __OFFSCREEN_H__
#define __OFFSCREEN_H__

#include "FreeImage.h"
#include <glad/glad.h>

#include <glm/glm.hpp>
#include "data_structures.h"

// Configurations
#ifdef OFFLINE_RENDERING
inline bool g_use_offscreen = true;
#else
inline bool g_use_offscreen = false;
#endif

static constexpr int fps = 60;
static constexpr int simulation_time = 300; // seconds


static GLuint WIDTH, HEIGHT;

void SetOffscreenWidthHeight(GLuint w, GLuint h) {
    WIDTH = w;
    HEIGHT = h;
}

static bool save_to_image(unsigned int w, unsigned int h, char *data, size_t length) {
    static int index = 0;

    char filename[50];
    std::sprintf(filename, "out/%08d.png", index++);

    FIBITMAP *bitmap = FreeImage_Allocate(w, h, 32, 0, 0, 0);

    BYTE *pixels = (BYTE *) FreeImage_GetBits(bitmap);

    memcpy(pixels, data, w * h * 4);
    bool bSuccess = FreeImage_Save(FIF_PNG, bitmap, filename, PNG_DEFAULT);
    FreeImage_Unload(bitmap);
    return bSuccess;
}

void OffscreenSaveRGBA() {
    unsigned char *data = new unsigned char[WIDTH * HEIGHT * 4];
    memset(data, 0, WIDTH * HEIGHT * 4);
    glReadPixels(0, 0, WIDTH, HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, data);
    save_to_image(WIDTH, HEIGHT, (char *) data, WIDTH * HEIGHT * 4);
    delete[] data;
}

static glm::vec3
HermiteCubicInterpolate(const glm::vec3 &p1, const glm::vec3 &p1_, const glm::vec3 &p2, const glm::vec3 &p2_, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    float F0 = 2.f * t3 - 3.f * t2 + 1.f;
    float F1 = -2.f * t3 + 3.f * t2;
    float F2 = t3 - 2.f * t2 + t;
    float F3 = t3 - t2;

    return F0 * p1 + F1 * p2 + F2 * p1_ + F3 * p2_;
}


extern bool time_stop;

static std::vector<glm::vec3> CatmullRomPoints;


//// close higher
//{05.0f,  26.0f, 16.0f}, // right
//{05.0f,  26.0f, 27.0f}, // up-right
//{16.0f,  26.0f, 27.0f}, // up
//{27.0f,  26.0f, 27.0f}, // up-left
//{27.0f,  26.0f, 16.0f}, // left
//{27.0f,  26.0f, 05.0f}, // bottom-left
//{16.0f,  26.0f, 05.0f}, // bottom
//{05.0f,  26.0f, 05.0f}, // bottom-right
//
//// far lower
//{-10.0f, 22.0f, 16.0f}, // right
//{-10.0f, 22.0f, 42.0f}, // up-right
//{16.0f,  22.0f, 42.0f}, // up
//{42.0f,  22.0f, 42.0f}, // up-left
//{42.0f,  22.0f, 16.0f}, // left
//{42.0f,  22.0f, -10.0f}, // bottom-left
//{16.0f,  22.0f, -10.0f}, // bottom
//{-10.0f, 22.0f, -10.0f}, // bottom-right

static glm::vec3 camera_initial_position = glm::vec3(61.9, 35.8, 74.5);
static glm::vec3 camera_initial_lookat = glm::vec3(28.5f, -5.8f, 26.2f);
static std::vector<glm::vec3> g_camera_positions;

static std::vector<glm::vec3> camera_route_positions{
        {11.3, 19.5, 67.9},
        {14.6, 16.3, 65.1},
        {18.5, 14.1, 58.6},
        {21.7, 13.3, 53.0},
        {27.1, 18.1, 47.7},
        {33.1, 20.6, 40.0},
        {33.9, 17.4, 36.5},
        {29.8, 11.1, 36.1},
        {26.8, 10.1, 36.1},
};

void OffscreenProcessCamera(Camera *camera) {
    if (camera == nullptr) {
        return;
    }

    static double start_time = glfwGetTime();


    static bool initialized = false;
    if (!initialized) {
        initialized = true;

        particle_render_scale = 0.005;

        auto lerp = [](float min, float max, float t) {
            return min + (max - min) * t;
        };

        float x_mid = (x_min + x_max) * 0.5f;
        float y_mid = (y_min + y_max) * 0.5f;
        float z_mid = (z_min + z_max) * 0.5f;

        // start from right above the center of the scene
        camera_initial_position = glm::vec3(lerp(x_min, x_max, 0.6f),
                                            lerp(y_min, y_max, 1.1f),
                                            lerp(z_min, z_max, 0.5f));

        // look at the center of the scene
        camera_initial_lookat = glm::vec3(lerp(x_min, x_max, 0.5f),
                                          lerp(y_min, y_max, 0.5f),
                                          lerp(z_min, z_max, 0.5f));

        static float camera_close_h = lerp(y_min, y_max, 2.3f);
        static float camera_far_h = lerp(y_min, y_max, 1.6f);

        static float camera_close_padding = -(x_max + z_max - x_min - z_min) * 0.5f * 0.35f;
        static float camera_far_padding = -(x_max + z_max - x_min - z_min) * 0.5f * 0.2f;

        g_camera_positions = std::vector<glm::vec3>{

                // close higher
                {x_max + camera_close_padding, camera_close_h, z_mid}, // right
                {x_max + camera_close_padding, camera_close_h, z_min - camera_close_padding}, // up-right
                {x_mid,                        camera_close_h, z_min - camera_close_padding}, // up
                {x_min - camera_close_padding, camera_close_h, z_min - camera_close_padding}, // up-left
                {x_min - camera_close_padding, camera_close_h, z_mid}, // left
                {x_min - camera_close_padding, camera_close_h, z_max + camera_close_padding}, // bottom-left
                {x_mid,                        camera_close_h, z_max + camera_close_padding}, // bottom
                {x_max + camera_close_padding, camera_close_h, z_max + camera_close_padding}, // bottom-right

                // far lower
                {x_max + camera_far_padding,   camera_far_h,   z_mid}, // right
                {x_max + camera_far_padding,   camera_far_h,   z_min - camera_far_padding}, // up-right
                {x_mid,                        camera_far_h,   z_min - camera_far_padding}, // up
                {x_min - camera_far_padding,   camera_far_h,   z_min - camera_far_padding}, // up-left
                {x_min - camera_far_padding,   camera_far_h,   z_mid}, // left
                {x_min - camera_far_padding,   camera_far_h,   z_max + camera_far_padding}, // bottom-left
                {x_mid,                        camera_far_h,   z_max + camera_far_padding}, // bottom
                {x_max + camera_far_padding,   camera_far_h,   z_max + camera_far_padding}, // bottom-right

        };

        camera->Position = camera_initial_position;
        camera->Front = glm::normalize(camera_initial_lookat - camera_initial_position);

        CatmullRomPoints.push_back(2.f * camera_initial_position - g_camera_positions[0]);
        CatmullRomPoints.push_back(camera_initial_position);
        CatmullRomPoints.push_back(g_camera_positions[0]);
        CatmullRomPoints.push_back(g_camera_positions[1]);
    }

    static constexpr int total_frame = fps * simulation_time; // 60 frames per second
    static int frame_remains = total_frame; // 60 frames per second

    // update camera location
    static float t = 0.0f;
    static constexpr float speed = 0.001f;
    static int point_index = 2;

    static bool first_route = false;

    glm::vec3 new_position = HermiteCubicInterpolate(
            CatmullRomPoints[1],
            (CatmullRomPoints[2] - CatmullRomPoints[0]) * 0.5f,
            CatmullRomPoints[2],
            (CatmullRomPoints[3] - CatmullRomPoints[1]) * 0.5f,
            t);
    camera->Position = new_position;
    camera->Front = glm::normalize(camera_initial_lookat - new_position);

    t += speed;

    if (t >= 1.0f) {
        // end of curve
        t = 0.0f;

        if (!first_route) {
            first_route = true;
            time_stop = false;
        }

        CatmullRomPoints.erase(CatmullRomPoints.begin());
        CatmullRomPoints.push_back(g_camera_positions[point_index]);
        point_index += 1;
        if (point_index >= g_camera_positions.size()) {
            point_index = 0;
        }
    }

    std::cout << "frame: " << total_frame - frame_remains << std::endl;
    if (frame_remains-- <= 0) {
        std::cout << "/////////////////////////////////////" << std::endl;
        std::cout << "total time cost: " << glfwGetTime() - start_time << std::endl;

        time_stop = true;
//        exit(0); // quit
    }

}


void OffscreenProcessCameraNew(Camera *pCamera) {
    if (pCamera == nullptr) {
        return;
    }

    static double start_time = glfwGetTime();


    static bool initialized = false;
    if (!initialized) {
        initialized = true;

        auto lerp = [](float min, float max, float t) {
            return min + (max - min) * t;
        };

        float x_mid = (x_min + x_max) * 0.5f;
        float y_mid = (y_min + y_max) * 0.5f;
        float z_mid = (z_min + z_max) * 0.5f;

        pCamera->Position = camera_initial_position;
        pCamera->Front = glm::normalize(camera_initial_lookat - camera_initial_position);

        CatmullRomPoints.push_back(2.f * camera_initial_position - camera_route_positions[0]);
        CatmullRomPoints.push_back(camera_initial_position);
        CatmullRomPoints.push_back(camera_route_positions[0]);
        CatmullRomPoints.push_back(camera_route_positions[1]);

        particle_render_scale = particle_render_scale_maximum;
    }

    static constexpr int total_frame = fps * simulation_time; // 60 frames per second
    static int frame_remains = total_frame; // 60 frames per second

    static constexpr int particle_size_toggle_frame = fps * 10; // 10 seconds
    static int particle_size_toggle_frame_remains = particle_size_toggle_frame;


    if (frame_remains-- <= 0) {
        // starts to navigate the scene
        time_stop = true;

        particle_render_scale = particle_render_scale_minimum;
    } else {
        // stop the camera
        time_stop = false;

        // toggle particle size every 10 seconds
        if (particle_size_toggle_frame_remains-- <= 0) {
            particle_size_toggle_frame_remains = particle_size_toggle_frame;

            particle_render_scale = particle_render_scale == particle_render_scale_minimum
                                    ? particle_render_scale_maximum
                                    : particle_render_scale_minimum;
        }

        std::cout << "frame: [" << total_frame - frame_remains << "/" << total_frame << "]" << std::endl;
        return;
    }

    // navigate the scene by hand

    /*
    // update camera location
    static float t = 0.0f;
    static constexpr float speed = 0.01f;
    static int point_index = 2;

    glm::vec3 new_position = HermiteCubicInterpolate(
            CatmullRomPoints[1],
            (CatmullRomPoints[2] - CatmullRomPoints[0]) * 0.5f,
            CatmullRomPoints[2],
            (CatmullRomPoints[3] - CatmullRomPoints[1]) * 0.5f,
            t);
    camera->Position = new_position;
    camera->Front = glm::normalize(camera_initial_lookat - new_position);

    t += speed;

    if (t >= 1.0f) {
        // end of curve
        t = 0.0f;

        CatmullRomPoints.erase(CatmullRomPoints.begin());
        CatmullRomPoints.push_back(camera_route_positions[point_index]);
        point_index += 1;
        if (point_index >= camera_route_positions.size()) {
            std::cout << "/////////////////////////////////////" << std::endl;
            std::cout << "total time cost: " << glfwGetTime() - start_time << std::endl;

            exit(0);
        }
    }
     */

}

#endif // !__OFFSCREEN_H__