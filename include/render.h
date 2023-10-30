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

#include <random>
#include <map>
#define M_PI           3.14159265358979323846
#include <FastNoise/FastNoise.h>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/Polyhedron_3.h>


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


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct Triangle {
    Vertex v1, v2, v3;
};

// Random num range
float distrib_range = 16.0;

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_3 Point_3;

std::vector<Vertex> generateVertices(int seed) {
    std::vector<Vertex> vertices;
    std::mt19937 generator(seed); // RNG integer seed
    std::uniform_real_distribution<float> distribution(-distrib_range, distrib_range);

    // Center Point
    glm::vec3 center = glm::vec3(0, 0, 0);
    // Points used in mesh generation
    for (int i = 0; i < 12; i++) {
        glm::vec3 position = glm::vec3(distribution(generator), distribution(generator), distribution(generator));
        vertices.push_back({ position });
    }

    return vertices;
}

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_3 Point_3;
typedef CGAL::Polyhedron_3<K> Polyhedron;

// Generates triangles from a set of vertices using convex hull. Input verts for the tris to be computed, return a vector of tris.
std::vector<glm::ivec3> generateTrianglesFromVertices(const std::vector<Vertex>& vertices) {
    // Convert the input vertices to CGAL Point_3 format
    std::vector<Point_3> points;
    // A map to associate a point to its index in the original vertex list
    std::unordered_map<Point_3, int> pointToIndex;

    // Populate the points vector and pointToIndex map.
    for (int i = 0; i < vertices.size(); ++i) {
        const Vertex& vertex = vertices[i];
        Point_3 point(vertex.position.x, vertex.position.y, vertex.position.z);
        points.push_back(point);
        pointToIndex[point] = i;
    }

    // Create an empty polyhedron to hold the convex hull, then store it
    Polyhedron P;
    CGAL::convex_hull_3(points.begin(), points.end(), P);

    // Vector to store the resulting triangles.
    std::vector<glm::ivec3> triangles;
    // Iterate through the facets of the polyhedron and convert them to triangle indices.
    for (auto facet = P.facets_begin(); facet != P.facets_end(); ++facet) {
        auto halfedge = facet->halfedge();
        // Get the indices of the vertices of the triangle from the pointToIndex map.
        int v0 = pointToIndex[halfedge->vertex()->point()];
        int v1 = pointToIndex[halfedge->next()->vertex()->point()];
        int v2 = pointToIndex[halfedge->next()->next()->vertex()->point()];
        triangles.push_back(glm::ivec3(v0, v1, v2));
    }

    return triangles;
}

// Generates quads from given tris. Finds two tris with common edge and combine them.
std::vector<glm::ivec4> generateQuadsFromTriangles(const std::vector<glm::ivec3>& triangles) {
    // Vector to store resulting quads
    std::vector<glm::ivec4> quads;
    // Track which tris have been processed to form quads
    std::vector<bool> processed(triangles.size(), false);

    // Iterate over each triangle in the list
    for (size_t i = 0; i < triangles.size(); i++) {
        if (processed[i]) continue;

        glm::ivec3 tri1 = triangles[i];

        // Compare the current tri with subsequent triangles in the list
        for (size_t j = i + 1; j < triangles.size(); j++) {
            if (processed[j]) continue;

            glm::ivec3 tri2 = triangles[j];

            // Convert triangle vertices into sets for easy intersection computation
            std::set<int> tri1Vertices{ tri1.x, tri1.y, tri1.z };
            std::set<int> tri2Vertices{ tri2.x, tri2.y, tri2.z };
            std::vector<int> commonVertices;

            // Find the common vertices between the two triangles
            std::set_intersection(tri1Vertices.begin(), tri1Vertices.end(),
                tri2Vertices.begin(), tri2Vertices.end(),
                std::back_inserter(commonVertices));

            // If two common vertices are found, the tris share an edge
            if (commonVertices.size() == 2) {
                // Remove common verts to find the unique verts of the tris
                tri1Vertices.erase(commonVertices[0]);
                tri1Vertices.erase(commonVertices[1]);
                tri2Vertices.erase(commonVertices[0]);
                tri2Vertices.erase(commonVertices[1]);

                // Extract unique verts from two triangles
                int uniqueVertex1 = *tri1Vertices.begin();
                int uniqueVertex2 = *tri2Vertices.begin();

                // Form a quad using the common edge and the unique verts
                quads.push_back(glm::ivec4(commonVertices[0], commonVertices[1], uniqueVertex1, uniqueVertex2));
                // Mark the triangles as processed
                processed[i] = true;
                processed[j] = true;
                break;
            }
        }
    }

    return quads;
}

// Compute and assign vertex normals for verts and quads
void computeVertexNormals(std::vector<Vertex>& vertices, const std::vector<glm::ivec4>& quads) {
    // Initialize all vertex normal to be 0
    for (auto& vertex : vertices) {
        vertex.normal = glm::vec3(0.0f);
    }

    // For each quad, calculate norm, store in quadNormals
    std::vector<glm::vec3> quadNormals;
    for (const auto& quad : quads) {
        glm::vec3 v0 = vertices[quad.x].position;
        glm::vec3 v1 = vertices[quad.y].position;
        glm::vec3 v2 = vertices[quad.z].position;

        // Compute normal vector for current quad using cross product
        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        quadNormals.push_back(normal);
    }

    // Each vertex normal is the sum of the normals of the quads sharing that vertex
    for (size_t i = 0; i < quads.size(); i++) {
        const auto& quad = quads[i];
        vertices[quad.x].normal += quadNormals[i];
        vertices[quad.y].normal += quadNormals[i];
        vertices[quad.z].normal += quadNormals[i];
        vertices[quad.w].normal += quadNormals[i];
    }

    // Get unit vertex normals
    for (auto& vertex : vertices) {
        vertex.normal = glm::normalize(vertex.normal);
    }
}

// 定义哈希函数处理顶点对
struct PairHash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2;
    }
};

void catmullClarkSubdivision(std::vector<Vertex>& vertices, std::vector<glm::ivec4>& quads) {
    std::vector<Vertex> newVertices;
    std::vector<glm::ivec4> newQuads;

    // 1. 对每个四边形计算面中心
    for (const auto& quad : quads) {
        glm::vec3 faceCenter = (vertices[quad.x].position + vertices[quad.y].position + vertices[quad.z].position + vertices[quad.w].position) / 4.0f;
        newVertices.push_back({ faceCenter });
    }

    // 2. 对每个边计算边中心，同时确保每个边只计算一次
    std::unordered_map<std::pair<int, int>, int, PairHash> edgeCenters;
    for (const auto& quad : quads) {
        std::vector<int> indices = { quad.x, quad.y, quad.z, quad.w };
        for (int i = 0; i < 4; i++) {
            int start = std::min(indices[i], indices[(i + 1) % 4]);
            int end = std::max(indices[i], indices[(i + 1) % 4]);
            if (edgeCenters.find({ start, end }) == edgeCenters.end()) {
                glm::vec3 edgeCenter = (vertices[start].position + vertices[end].position) / 2.0f;
                newVertices.push_back({ edgeCenter });
                edgeCenters[{start, end}] = newVertices.size() - 1;
            }
        }
    }

    // 3. 重新计算每个原始顶点的位置
    for (int i = 0; i < vertices.size(); i++) {
        glm::vec3 F(0.0f);
        glm::vec3 R(0.0f);
        int n = 0;

        for (const auto& quad : quads) {
            if (quad.x == i || quad.y == i || quad.z == i || quad.w == i) {
                F += newVertices[&quad - &quads[0]].position;
                n++;
            }
        }
        F /= n;

        std::vector<int> adjacentEdges;
        for (const auto& edge : edgeCenters) {
            if (edge.first.first == i || edge.first.second == i) {
                adjacentEdges.push_back(edge.second);
            }
        }

        for (int edgeIndex : adjacentEdges) {
            R += newVertices[edgeIndex].position;
        }
        R /= adjacentEdges.size();

        vertices[i].position = (F + glm::vec3(2.0f) * R + glm::vec3(n - 3) * vertices[i].position) / float(n);
    }


    // 4. 使用新计算的顶点和中心创建新的四边形
    for (const auto& quad : quads) {
        int faceCenterIndex = &quad - &quads[0];
        int edgeCenterIndices[4] = {
            edgeCenters[{quad.x, quad.y}],
            edgeCenters[{quad.y, quad.z}],
            edgeCenters[{quad.z, quad.w}],
            edgeCenters[{quad.w, quad.x}]
        };

        // 创建新的四个四边形，这些四边形共享相同的面中心，但与不同的边中心相接
        newQuads.push_back({ quad.x, edgeCenterIndices[0], faceCenterIndex, edgeCenterIndices[3] });
        newQuads.push_back({ quad.y, edgeCenterIndices[1], faceCenterIndex, edgeCenterIndices[0] });
        newQuads.push_back({ quad.z, edgeCenterIndices[2], faceCenterIndex, edgeCenterIndices[1] });
        newQuads.push_back({ quad.w, edgeCenterIndices[3], faceCenterIndex, edgeCenterIndices[2] });
    }

    vertices = newVertices;
    quads = newQuads;
}

// 自定义一个clamp函数，只有C++17及以上才有自带的clamp函数
template <typename T>
T clamp(const T& val, const T& min, const T& max) {
    return std::max(min, std::min(max, val));
}

void displaceVerticesWithNoise(std::vector<Vertex>& vertices, float strength) {
    // 创建FastNoise2生成器
    FastNoise::SmartNode<> fnGenerator = FastNoise::NewFromEncodedNodeTree("EwDD9Sg/DQAEAAAAAAAgQAkAAGZmJj8AAAAAPw==");

    // 定义噪声图的大小
    const int noiseSize = 128;
    std::vector<float> noiseOutput(noiseSize * noiseSize);

    // 生成2D噪声
    fnGenerator->GenUniformGrid2D(noiseOutput.data(), 0, 0, noiseSize, noiseSize, 0.2f, 1337);

    // 对顶点进行displacement
    for (Vertex& vertex : vertices) {
        int x = clamp(std::abs(static_cast<int>(vertex.position.x)), 0, noiseSize - 1);
        int z = clamp(std::abs(static_cast<int>(vertex.position.z)), 0, noiseSize - 1);
        int index = z * noiseSize + x;
        float noiseValue = noiseOutput[index] * strength;
        vertex.position += vertex.normal * noiseValue;
    }
}

std::vector<Triangle> quadsToTriangles(const std::vector<glm::ivec4>& quads, const std::vector<Vertex>& vertices) {
    std::vector<Triangle> triangles;
    triangles.reserve(quads.size() * 2);  // 预分配空间
    for (const auto& quad : quads) {
        triangles.push_back({ vertices[quad.x], vertices[quad.y], vertices[quad.z] });
        triangles.push_back({ vertices[quad.x], vertices[quad.z], vertices[quad.w] });
    }
    return triangles;
}

// 判断射线是否与三角形相交
// rayOrigin: 射线的起点
// rayVector: 射线的方向
// triangle: 要进行检测的三角形
// t: 输出参数，如果射线与三角形相交，则表示相交点距离射线起点的距离
bool rayIntersectsTriangle(const glm::vec3& rayOrigin,
    const glm::vec3& rayVector,
    const Triangle& triangle,
    float& t) {
    // 定义一个极小值，用于浮点数比较
    const float EPSILON = 0.0000001f;
    // 获取三角形的三个顶点
    glm::vec3 vertex0 = triangle.v1.position;
    glm::vec3 vertex1 = triangle.v2.position;
    glm::vec3 vertex2 = triangle.v3.position;
    glm::vec3 h, s, q;
    float a, f, u, v;
    // 计算射线与三角形的交叉点
    h = glm::cross(rayVector, vertex2 - vertex0);
    a = glm::dot(vertex1 - vertex0, h);
    // 如果a接近0，表示射线与三角形平行，不相交
    if (a > -EPSILON && a < EPSILON)
        return false;
    f = 1.0f / a;
    s = rayOrigin - vertex0;
    u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;
    q = glm::cross(s, vertex1 - vertex0);
    v = f * glm::dot(rayVector, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;
    t = f * glm::dot(vertex2 - vertex0, q);
    if (t > EPSILON)
        return true;
    else
        return false;
}

// 使用重心坐标系判断点是否在三角形内部
bool isPointInTriangle(const glm::vec3& point, const Triangle& triangle) {
    // 计算三角形的两个向量
    glm::vec3 v0 = triangle.v3.position - triangle.v1.position;
    glm::vec3 v1 = triangle.v2.position - triangle.v1.position;
    glm::vec3 v2 = point - triangle.v1.position;

    // 使用点积计算重心坐标系
    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;

    // 计算重心坐标
    float alpha = (d11 * d20 - d01 * d21) / denom;
    float beta = (d00 * d21 - d01 * d20) / denom;
    float gamma = 1.0f - alpha - beta;

    // 如果重心坐标的所有值都大于0，则点在三角形内部
    return alpha >= 0 && beta >= 0 && gamma >= 0;
}

// 检查点是否在三维网格内，通过发送射线并计算与三角形的交点来实现
bool isPointInsideMesh(const glm::vec3& point, const std::vector<Triangle>& triangles) {
    glm::vec3 rayVector(0.0f, 1.0f, 0.0f);  // 射线方向
    int intersections = 0;
    float t;
    // 对每个三角形进行射线交叉检测
    for (const auto& triangle : triangles) {
        if (rayIntersectsTriangle(point, rayVector, triangle, t)) {
            intersections++;
        }
    }
    // 如果与奇数个三角形相交，点在网格内，否则在外部
    return (intersections % 2) == 1;
}

// 检查体素的角点是否与三维网格相交
bool isVoxelIntersectingMesh(const glm::vec3 voxelCorners[8], const std::vector<Triangle>& finalTriangles) {
    // 遍历体素的所有角点，检查它们是否在三维网格内。如果至少有一个角点在，则返回true，表示体素与三维网格相交
    for (int i = 0; i < 8; i++) {
        if (isPointInsideMesh(voxelCorners[i], finalTriangles)) {
            return true;
        }
    }
    return false;
}

// set up voxel field
void set_up_voxel_field(voxel_field& V) {
    voxel v1;
    v1.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    v1.density = 1.0f;
    v1.exist = true;

    std::random_device rd;
    int randomSeed = rd();

    // 1. 生成顶点和四边形
    std::vector<Vertex> vertices = generateVertices(randomSeed);
    std::vector<glm::ivec3> triangles = generateTrianglesFromVertices(vertices);
    std::vector<glm::ivec4> quads = generateQuadsFromTriangles(triangles);

    // 2. 应用Catmull-Clark细分
    catmullClarkSubdivision(vertices, quads);

    // 3. 应用位移
    displaceVerticesWithNoise(vertices, 0.5);

    // 4. 转换为三角形
    std::vector<Triangle> finalTriangles = quadsToTriangles(quads, vertices);

    for (int x = 0; x < V.x_size; x++)
    {
        for (int y = 0; y < V.y_size; y++)
        {
            for (int z = 0; z < V.z_size; z++)
            {
                glm::vec3 samplePoint(x, y, z);
                if (isPointInsideMesh(samplePoint, finalTriangles))
                {
                    V.set_voxel(x, y, z, v1);
                }
            }
        }
    }

    /*V.set_voxel(0, 0, 0, v1);
    V.set_voxel(0, 0, 1, v1);
    V.set_voxel(0, 1, 0, v1);
    V.set_voxel(1, 0, 0, v1);*/
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