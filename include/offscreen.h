#ifndef __OFFSCREEN_H__
#define __OFFSCREEN_H__

#include "FreeImage.h"
#include <glad/glad.h>

#include <glm/glm.hpp>

#ifndef USE_OFFSCREEN
#define USE_OFFSCREEN 1
#endif

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

static const glm::vec3 camera_initial_position(16.f, 16.f, 16.f);
static const glm::vec3 camera_initial_lookat(16.f, 3.f, 16.f);

std::vector<glm::vec3> g_camera_positions{

        // close higher
        {05.0f,  26.0f, 16.0f}, // right
        {05.0f,  26.0f, 27.0f}, // up-right
        {16.0f,  26.0f, 27.0f}, // up
        {27.0f,  26.0f, 27.0f}, // up-left
        {27.0f,  26.0f, 16.0f}, // left
        {27.0f,  26.0f, 05.0f}, // bottom-left
        {16.0f,  26.0f, 05.0f}, // bottom
        {05.0f,  26.0f, 05.0f}, // bottom-right

        // far lower
        {-10.0f, 22.0f, 16.0f}, // right
        {-10.0f, 22.0f, 42.0f}, // up-right
        {16.0f,  22.0f, 42.0f}, // up
        {42.0f,  22.0f, 42.0f}, // up-left
        {42.0f,  22.0f, 16.0f}, // left
        {42.0f,  22.0f, -10.0f}, // bottom-left
        {16.0f,  22.0f, -10.0f}, // bottom
        {-10.0f, 22.0f, -10.0f}, // bottom-right

};

extern bool time_stop;

static std::vector<glm::vec3> CatmullRomPoints;

void OffscreenProcessCamera(Camera *camera) {
    if (camera == nullptr) {
        return;
    }

    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        camera->Position = camera_initial_position;
        camera->Front = glm::normalize(camera_initial_lookat - camera_initial_position);

        CatmullRomPoints.push_back(2.f * camera_initial_position - g_camera_positions[0]);
        CatmullRomPoints.push_back(camera_initial_position);
        CatmullRomPoints.push_back(g_camera_positions[0]);
        CatmullRomPoints.push_back(g_camera_positions[1]);
    }

    static bool first_route = false;
    static int total_frame = 3200; // 30 frames per second

    // update camera location
    static float t = 0.0f;
    constexpr float speed = 0.005f;
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
        //exit(0);

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

    std::cout << "frame: " << 3200 - total_frame << std::endl;
    if (total_frame-- <= 0) {
        exit(0); // quit
    }

}

#endif // !__OFFSCREEN_H__