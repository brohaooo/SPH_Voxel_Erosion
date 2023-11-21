#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <unordered_map>

#include <random>
#include <FastNoise/FastNoise.h>

#include <data_structures.h>


// this will inicate the beginning of the voxel field(x=y=z=0) in world space
extern const float voxel_x_origin;
extern const float voxel_y_origin;
extern const float voxel_z_origin;
// this will adjust voxel size, the voxel size will be voxel_size_scale * 1
extern const float voxel_size_scale;

std::vector<std::vector<std::vector<float>>> CreateGround(int xStart, int yStart) {
    // 创建FastNoise2生成器
    FastNoise::SmartNode<> fnGenerator = FastNoise::NewFromEncodedNodeTree("EwCamZk+DQAMAAAAw/VoQAkAAKRwvT4AAAAAPw==");

    // 定义噪声图的大小
    const int noiseSizeX = 32;
    const int noiseSizeY = 32;
    std::vector<float> noiseOutput(noiseSizeX * noiseSizeY);

    // 生成2D噪声
    fnGenerator->GenUniformGrid2D(noiseOutput.data(), xStart, yStart, noiseSizeX, noiseSizeY, 0.15f, 1337);

    // 噪声值的最大高度
    const int maxHeight = 12;

    // 初始化三维数组
    auto noise3DArray = std::vector<std::vector<std::vector<float>>>(noiseSizeX, std::vector<std::vector<float>>(noiseSizeY, std::vector<float>(maxHeight, 0)));

    // 填充三维数组
    for (int x = 0; x < noiseSizeX; ++x) {
        for (int y = 0; y < noiseSizeY; ++y) {
            // 将噪声值从[-1, 1]映射到[0, 1]
            float scaledHeight = (noiseOutput[x * noiseSizeY + y] + 1.0f) / 2.0f;

            // 将映射后的噪声值转换为体素高度
            int voxelHeight = static_cast<int>(scaledHeight * maxHeight);

            // 设置该点对应的高度值
            for (int z = 0; z < voxelHeight; ++z) {
                noise3DArray[x][y][z] = 1.0f; // 在高度 z 处设置体素存在
            }
        }
    }

    return noise3DArray;
}

void set_up_voxel_field(voxel_field& V, float voxel_density) {
    // common destroyable voxel
    voxel v1;
    v1.density = voxel_density;
    v1.exist = true;
    v1.is_new = false;
    v1.not_destroyable = false;
    v1.update_color();

    // 生成地形数据
    int xStart = 0; // 可以根据需要更改这些值
    int yStart = 0;
    auto terrainData = CreateGround(xStart, yStart);


    // 根据地形数据设置体素
    for (int i = 0; i < terrainData.size(); i++) {
        for (int k = 0; k < terrainData[i].size(); k++) {
            for (int j = 0; j < terrainData[i][k].size(); j++) {
                if (terrainData[i][k][j] > 0 && i < V.x_size && j <V .y_size && k < V.z_size) {
                    V.set_voxel(i, j, k, v1);
                }
            }
        }
    }
}




void refresh_debug(voxel_field& V) {
    for (int i = 0; i < V.x_size; i++) {
        for (int j = 0; j < V.y_size; j++) {
            for (int k = 0; k < V.z_size; k++) {
                V.get_voxel(i, j, k).debug = false;
            }
        }
    }
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
    p1.mass = particle_mass;
    p1.stuck_count = 0;

    for (int i = 0; i < P.size(); i++) {
        p1.currPos = generateRandomVec3();
        p1.currPos.y /= 6;
        p1.currPos.y += (y_max - y_min) * 5 / 6;
        p1.currPos.x /= 6;
        //p1.currPos.x += (x_max - x_min) * 3 / 4;
        P[i] = p1;
    }

    //P[0].currPos = glm::vec3(2.4f, 3.0f, 2.5f);
    //P[1].currPos = glm::vec3(0.115f, 2.111f, 0.125f);

}


void voxel::update_color() {
    float density_ratio = density / voxel_maximum_density;
    if (!this->is_new) {
        glm::vec4 mix_color = glm::mix(dark_red, soil_color, density_ratio);
        color = mix_color;
    }
    else {
        glm::vec4 mix_color = glm::mix(dark_red, green, density_ratio);
        color = glm::vec4(0.1f, 0.9f, density_ratio, 1.0f);
    }
}


// definition of the field, contains a 3D array of voxels

voxel_field::voxel_field(int x, int y, int z) {
    x_size = x;
    y_size = y;
    z_size = z;
    NULL_VOXEL.exist = false;
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
void voxel_field::set_voxel(int x, int y, int z, float density, glm::vec4 color) {
    field[x][y][z].exist = true;
    field[x][y][z].density = density;
    field[x][y][z].color = color;
}
void voxel_field::set_voxel(int x, int y, int z, voxel v) {
    field[x][y][z] = v;
}
voxel& voxel_field::get_voxel(int x, int y, int z) {
    if (x < 0 || x >= x_size || y < 0 || y >= y_size || z < 0 || z >= z_size) {
        return NULL_VOXEL;
    }
    // if (x < 0) {
    // 	x = 0;
    // }
    // if (x >= x_size) {
    // 	x = x_size - 1;
    // }
    // if (y < 0) {
    // 	y = 0;
    // }
    // if (y >= y_size) {
    // 	y = y_size - 1;
    // }
    // if (z < 0) {
    // 	z = 0;
    // }
    // if (z >= z_size) {
    // 	z = z_size - 1;
    // }


    return field[x][y][z];
}
void voxel_field::clear_voxel(int x, int y, int z) {
    field[x][y][z].exist = false;
    field[x][y][z].density = 0.0f;
    field[x][y][z].color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
}
void voxel_field::clear_all() {
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
void voxel_field::print_field() {
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




// avoid some cases that the voxel index is out of bound
void check_voxel_index(int& x, int& y, int& z, voxel_field& V) {
    if (x < 0) {
        x = 0;
    }
    if (x >= V.x_size) {
        x = V.x_size - 1;
    }
    if (y < 0) {
        y = 0;
    }
    if (y >= V.y_size) {
        y = V.y_size - 1;
    }
    if (z < 0) {
        z = 0;
    }
    if (z >= V.z_size) {
        z = V.z_size - 1;
    }
}







// used when need to render the voxel in world space
glm::vec3 voxel_to_world(int x, int y, int z) {
    return glm::vec3(x * voxel_size_scale + voxel_x_origin, y * voxel_size_scale + voxel_y_origin, z * voxel_size_scale + voxel_z_origin);
};
// used in DDA, need to get the x positive face's world pos of the voxel
std::vector<float> voxel_to_world_6_face(int x, int y, int z) {
    std::vector<float> res;// {x_pos,x_neg,y_pos,y_neg,z_pos,z_neg}
    res.push_back(x * voxel_size_scale + voxel_x_origin + voxel_size_scale / 2);
    res.push_back(x * voxel_size_scale + voxel_x_origin - voxel_size_scale / 2);
    res.push_back(y * voxel_size_scale + voxel_y_origin + voxel_size_scale / 2);
    res.push_back(y * voxel_size_scale + voxel_y_origin - voxel_size_scale / 2);
    res.push_back(z * voxel_size_scale + voxel_z_origin + voxel_size_scale / 2);
    res.push_back(z * voxel_size_scale + voxel_z_origin - voxel_size_scale / 2);
    return res;
};

// trick version of voxel_to_world_6_face(), assmune the voxel is a little bit bigger than the real voxel
std::vector<float> voxel_to_world_6_face_extend(int x, int y, int z) {
    std::vector<float> res;// {x_pos,x_neg,y_pos,y_neg,z_pos,z_neg}
    res.push_back(x * voxel_size_scale + voxel_x_origin + (voxel_size_scale * 1.001f) / 2);
    res.push_back(x * voxel_size_scale + voxel_x_origin - (voxel_size_scale * 1.001f) / 2);
    res.push_back(y * voxel_size_scale + voxel_y_origin + (voxel_size_scale * 1.001f) / 2);
    res.push_back(y * voxel_size_scale + voxel_y_origin - (voxel_size_scale * 1.001f) / 2);
    res.push_back(z * voxel_size_scale + voxel_z_origin + (voxel_size_scale * 1.001f) / 2);
    res.push_back(z * voxel_size_scale + voxel_z_origin - (voxel_size_scale * 1.001f) / 2);
    return res;
};
// used when need to get the voxel index from world space(might be a float, which then converted to the int)
std::vector<int> world_to_voxel(glm::vec3 world, voxel_field& V) {
    //std::cout<<"world" << world.x << " " << world.y << " " << world.z << std::endl;
    //glm::vec3 float_val = glm::vec3((world.x - voxel_x_origin) / voxel_size_scale, (world.y - voxel_y_origin) / voxel_size_scale, (world.z - voxel_z_origin) / voxel_size_scale);
    glm::vec3 float_val = glm::vec3((world.x) / voxel_size_scale, (world.y) / voxel_size_scale, (world.z) / voxel_size_scale);
    //std::cout<<"voxel" << float_val.x << " " << float_val.y << " " << float_val.z << std::endl;
    int x = static_cast<int>(std::floor(float_val.x));
    int y = static_cast<int>(std::floor(float_val.y));
    int z = static_cast<int>(std::floor(float_val.z));
    check_voxel_index(x, y, z, V);
    return { x,y,z };
};



// definition of the neighbourhood grid, contains a 3D array of vectors of particle indices
// Neighborhood Search speed up part:// cell with size = smoothing_length
neighbourhood_grid::neighbourhood_grid(int x, int y, int z) {
    x_size = x;
    y_size = y;
    z_size = z;
    grid.resize(x);
    for (int i = 0; i < x_size; i++) {
        grid[i].resize(y_size);
        for (int j = 0; j < y_size; j++) {
            grid[i][j].resize(z_size);
            for (int k = 0; k < z_size; k++) {
                grid[i][j][k].resize(0);
            }
        }
    }
};
void neighbourhood_grid::add_particle(int x, int y, int z, int particle_index) {
    grid[x][y][z].push_back(particle_index);
};
void neighbourhood_grid::clear_grid() {
    for (int i = 0; i < x_size; i++) {
        grid[i].resize(y_size);
        for (int j = 0; j < y_size; j++) {
            grid[i][j].resize(z_size);
            for (int k = 0; k < z_size; k++) {
                grid[i][j][k].clear();
            }
        }
    }
};
// here we use the same world_to_object function as the voxel field
std::vector<int> neighbourhood_grid::world_to_grid(glm::vec3 world_pos) {
    glm::vec3 float_val = glm::vec3((world_pos.x) / neighbour_grid_size, (world_pos.y) / neighbour_grid_size, (world_pos.z) / neighbour_grid_size);
    int x = static_cast<int>(std::floor(float_val.x));
    int y = static_cast<int>(std::floor(float_val.y));
    int z = static_cast<int>(std::floor(float_val.z));
    if (x < 0) {
        x = 0;
    }
    if (x >= x_size) {
        x = x_size - 1;
    }
    if (y < 0) {
        y = 0;
    }
    if (y >= y_size) {
        y = y_size - 1;
    }
    if (z < 0) {
        z = 0;
    }
    if (z >= z_size) {
        z = z_size - 1;
    }
    return { x,y,z };
};
std::vector<int> neighbourhood_grid::get_neighbourhood(int x, int y, int z, int neighbood_range) {
    std::vector<int> res;
    for (int i = x - neighbood_range; i <= x + neighbood_range; i++) {
        if (i < 0 || i >= x_size) {
            continue;
        }
        for (int j = y - neighbood_range; j <= y + neighbood_range; j++) {
            if (j < 0 || j >= y_size) {
                continue;
            }
            for (int k = z - neighbood_range; k <= z + neighbood_range; k++) {
                if (k < 0 || k >= z_size) {
                    continue;
                }
                for (int l = 0; l < grid[i][j][k].size(); l++) {
                    res.push_back(grid[i][j][k][l]);
                }
            }
        }
    }
    return res;
};
// this one is used to get the upper neighbourhood(with particle_y >= input_y), which is used in the deposition detection
std::vector<int> neighbourhood_grid::get_upper_neighbourhood(int x, int y, int z, int neighbood_range) {
    std::vector<int> res;
    for (int i = x - neighbood_range; i <= x + neighbood_range; i++) {
        if (i < 0 || i >= x_size) {
            continue;
        }
        for (int j = y; j <= y + neighbood_range; j++) {
            if (j < 0 || j >= y_size) {
                continue;
            }
            for (int k = z - neighbood_range; k <= z + neighbood_range; k++) {
                if (k < 0 || k >= z_size) {
                    continue;
                }
                for (int l = 0; l < grid[i][j][k].size(); l++) {
                    res.push_back(grid[i][j][k][l]);
                }
            }
        }
    }
    return res;
};
// this one is used to get the lower neighbourhood(with particle_y <= input_y), which is used in the diffusion detection
std::vector<int> neighbourhood_grid::get_lower_neighbourhood(int x, int y, int z, int neighbood_range) {
    std::vector<int> res;
    for (int i = x - neighbood_range; i <= x + neighbood_range; i++) {
        if (i < 0 || i >= x_size) {
            continue;
        }
        for (int j = y; j >= y - neighbood_range; j--) {
            if (j < 0 || j >= y_size) {
                continue;
            }
            for (int k = z - neighbood_range; k <= z + neighbood_range; k++) {
                if (k < 0 || k >= z_size) {
                    continue;
                }
                for (int l = 0; l < grid[i][j][k].size(); l++) {
                    res.push_back(grid[i][j][k][l]);
                }
            }
        }
    }
    return res;
};












// definition of the bounding barrier, contains the max and min coordinates of the box
// particles cannot go beyond the box
bounding_box::bounding_box(GLfloat _x_max, GLfloat _x_min, GLfloat _y_max, GLfloat _y_min, GLfloat _z_max, GLfloat _z_min) {
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
    render_x_max += 0.15f;
    render_x_min -= 0.15f;
    render_y_max += 0.15f;
    render_y_min -= 0.15f;
    render_z_max += 0.15f;
    render_z_min -= 0.15f;

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

// generate a random vec3 in the min and max range
glm::vec3 generateRandomVec3(float _x_max, float _x_min, float _y_max, float _y_min, float _z_max, float _z_min) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> distributionX(_x_min, _x_max);
    std::uniform_real_distribution<float> distributionY(_y_min, _y_max);
    std::uniform_real_distribution<float> distributionZ(_z_min, _z_max);

    float randomX = distributionX(rng);
    float randomY = distributionY(rng);
    float randomZ = distributionZ(rng);

    return glm::vec3(randomX * 0.9f, randomY * 0.9f, randomZ * 0.9f);
}






void calculate_SPH_movement(std::vector<particle>& p, float frameTimeDiff, voxel_field& V, neighbourhood_grid& G, std::vector<int>& recycle_list) {
    int particle_num = std::min(current_particle_num, (int)p.size());
    //std::cout << "particle_num: " << particle_num << std::endl;
    // int particle_num = p.size();
    //refresh_debug(V);
    // first, re-genereate the neighbourhood grid
    G.clear_grid();
    // looks like we cannot use parallel here, shit (even use thread with mutex lock or reduction, it is slower than default)
    for (int i = 0; i < particle_num; i++) {

        std::vector<int> grid_index = G.world_to_grid(p[i].currPos);
        G.add_particle(grid_index[0], grid_index[1], grid_index[2], i);
    }


    // for each particle, calculate the density and pressure
#pragma omp parallel for
    for (int i = 0; i < particle_num; i++) {
        std::vector<int> current_grid = G.world_to_grid(p[i].currPos);
        std::vector<int> neighbour_particles = G.get_neighbourhood(current_grid[0], current_grid[1], current_grid[2]);


        int cnt = 0;
        float density_sum = 0.f;

        //for (int j = 0; j < particle_num; j++) {
        for (int j : neighbour_particles) {
            glm::vec3 delta = (p[i].currPos - p[j].currPos);
            float r = length(delta);
            if (r < smoothing_length)
            {
                cnt++;
                density_sum += p[j].mass * /* poly6 kernel */ 315.f * glm::pow(smoothing_length * smoothing_length - r * r, 3.f) / (64.f * PI_FLOAT * glm::pow(smoothing_length, 9));
                //density_sum += particle_mass * /* poly6 kernel */ 315.f * glm::pow(smoothing_length * smoothing_length - r * r, 3.f) / (64.f * PI_FLOAT * glm::pow(smoothing_length, 9));
            }
        }
        p[i].pamameters[0] = density_sum;
        p[i].pamameters[1] = glm::max(particle_stiffness * (density_sum - particle_resting_density), 0.f);
        p[i].pamameters[2] = float(cnt);
    }
    // for each particle, calculate the force and acceleration
#pragma omp parallel for
    for (int i = 0; i < particle_num; i++) {
        std::vector<int> current_grid = G.world_to_grid(p[i].currPos);
        std::vector<int> neighbour_particles = G.get_neighbourhood(current_grid[0], current_grid[1], current_grid[2]);


        glm::vec3 pressure_force = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 viscosity_force = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 dCs = glm::vec3(0.0f, 0.0f, 0.0f);
        //for (int j = 0; j < particle_num; j++) {
        for (int j : neighbour_particles) {
            if (i == j) {
                continue;
            }
            glm::vec3 delta = (p[i].currPos - p[j].currPos);
            float r = length(delta);
            if (r < smoothing_length) {
                if (r == 0.0f) {
                    // if the two particles are at the same position, add a small random delta to avoid NaN
                    delta = generateRandomVec3(0.0001f, -0.0001f, 0.0001f, -0.0001f, 0.0001f, -0.0001f);
                }
                // calculate the pressure force
                // pressure_force -= particle_mass * (p[i].pamameters[1] + p[j].pamameters[1]) / (2.f * p[j].pamameters[0]) *
                pressure_force -= p[i].mass * (p[i].pamameters[1] + p[j].pamameters[1]) / (2.f * p[j].pamameters[0]) *
                    // gradient of spiky kernel
                    -45.f / (PI_FLOAT * glm::pow(smoothing_length, 6.f)) * glm::pow(smoothing_length - r, 2.f) * glm::normalize(delta);
                // calculate the viscosity force
                // viscosity_force += particle_mass * (p[j].velocity - p[i].velocity) / p[j].pamameters[0] *
                viscosity_force += p[j].mass * (p[j].velocity - p[i].velocity) / p[j].pamameters[0] *
                    // Laplacian of viscosity kernel
                    45.f / (PI_FLOAT * glm::pow(smoothing_length, 6.f)) * (smoothing_length - r);

                // dCs -= particle_mass * glm::pow(smoothing_length * smoothing_length - r * r, 2.f) / p[j].pamameters[0] *
                dCs -= p[j].mass * glm::pow(smoothing_length * smoothing_length - r * r, 2.f) / p[j].pamameters[0] *
                    // Poly6 kernel
                    945.f / (32.f * PI_FLOAT * glm::pow(smoothing_length, 9.f)) * delta;
            }




        }


        viscosity_force *= particle_viscosity;
        p[i].acceleration = glm::vec3((pressure_force / p[i].pamameters[0] + viscosity_force / p[i].pamameters[0] + gravity_force));
        p[i].deltaCs = glm::vec3(glm::normalize(dCs));

    }




    // for each particle, calculate the velocity and new position
#pragma omp parallel for
    for (int i = 0; i < particle_num; i++) {
        glm::vec3 new_velocity = (p[i].velocity + frameTimeDiff * p[i].acceleration);
        glm::vec3 old_velocity = p[i].velocity;
        glm::vec3 old_position = p[i].currPos;
        glm::vec3 new_position = p[i].currPos + frameTimeDiff * new_velocity;





        p[i].velocity = new_velocity;
        p[i].prevPos = p[i].currPos;
        p[i].currPos = new_position;

        //std::cout<<"old position: "<<old_position.x<<" "<<old_position.y<<" "<<old_position.z<<std::endl;   
        //std::cout<<"new position: "<<new_position.x<<" "<<new_position.y<<" "<<new_position.z<<std::endl;


        // -----------------------particle - voxel collision detection-----------------------

        // ------3D-DDA collision------
        //std::cout << "begin DDA" << std::endl;
        // 1. get the initial voxel index & the final voxel index
        std::vector<int> begin_voxel_index = world_to_voxel(old_position, V);//check_voxel_index
        std::vector<int> end_voxel_index = world_to_voxel(new_position, V);


        voxel* end_v = &V.get_voxel(end_voxel_index[0], end_voxel_index[1], end_voxel_index[2]);
        voxel* current_v = &V.get_voxel(begin_voxel_index[0], begin_voxel_index[1], begin_voxel_index[2]);


        std::vector<int> current_voxel_index = begin_voxel_index;
        // 2. get the ray direction
        glm::vec3 ray_direction = glm::normalize(new_position - old_position);
        //std::cout << "ray_direction: " << ray_direction.x << " " << ray_direction.y << " " << ray_direction.z << std::endl;
        // 3. get the initial t and final t
        float t_current = 0.f; // begin at 0, start from the beginning of the ray AKA---> the previous position
        float t_end = glm::length(new_position - old_position); // end at the length of the ray
        // 4. get the delta t in each direction X/Y/Z
        float delta_t_x = voxel_size_scale / glm::abs(ray_direction.x);
        float delta_t_y = voxel_size_scale / glm::abs(ray_direction.y);
        float delta_t_z = voxel_size_scale / glm::abs(ray_direction.z);


        int sign_x = ray_direction.x > 0 ? 1 : -1;
        int sign_y = ray_direction.y > 0 ? 1 : -1;
        int sign_z = ray_direction.z > 0 ? 1 : -1;
        // 5. initialize t_next_x, t_next_y, t_next_z
        std::vector<float> voxel_6_face = voxel_to_world_6_face(current_voxel_index[0], current_voxel_index[1], current_voxel_index[2]);
        float t_next_x;
        float t_next_y;
        float t_next_z;
        // initialize t_next_x, t_next_y, t_next_z, it is the distance from the current position to the next X/Y/Z direction voxel's face
        if (sign_x == 1) {
            t_next_x = glm::abs((voxel_6_face[0] - old_position.x) / ray_direction.x);
        }
        else {
            t_next_x = glm::abs((voxel_6_face[1] - old_position.x) / ray_direction.x);
        }
        if (sign_y == 1) {
            t_next_y = glm::abs((voxel_6_face[2] - old_position.y) / ray_direction.y);
        }
        else {
            t_next_y = glm::abs((voxel_6_face[3] - old_position.y) / ray_direction.y);
        }
        if (sign_z == 1) {
            t_next_z = glm::abs((voxel_6_face[4] - old_position.z) / ray_direction.z);
        }
        else {
            t_next_z = glm::abs((voxel_6_face[5] - old_position.z) / ray_direction.z);
        }

        // exclude NAN
        if (std::isnan(t_next_x)) {
            t_next_x = INFINITY;
        }
        if (std::isnan(t_next_y)) {
            t_next_y = INFINITY;
        }
        if (std::isnan(t_next_z)) {
            t_next_z = INFINITY;
        }

        // if it is already inside a voxel, then try push it out (I cannot fix this bug by avoiding all the stucking inside possibilities, so just push it out)
        if (current_v->exist) {
            new_velocity = glm::vec3(0);
            //new_velocity *= -1.0f;
            glm::vec3 push_direction = glm::normalize(new_position - voxel_to_world(begin_voxel_index[0], begin_voxel_index[1], begin_voxel_index[2]));
            float push_distance = voxel_size_scale * 0.005f;
            new_position = old_position + push_distance * push_direction;
            p[i].currPos = new_position;
            p[i].velocity = new_velocity;

        }


        // 6. recursively check the voxel in the ray direction, if the voxel is occupied, 
        // then it collides, reverse the velocity and stop the particle at(/before) the collision point
        while ((t_current <= t_end) && !current_v->exist) {
            float t_min_next = glm::min(t_next_x, glm::min(t_next_y, t_next_z));
            int x_or_y_or_z = -1; // 0 for x, 1 for y, 2 for z, indicating which direction is the next voxel
            t_current += t_min_next;
            if (t_current > t_end) {
                break;
            }
            if (t_min_next == t_next_x)
            {
                x_or_y_or_z = 0;
                t_next_x += delta_t_x;
                current_voxel_index[0] += sign_x;
                if (current_voxel_index[0] < 0 || current_voxel_index[0] >= V.x_size)
                    break;
            }
            else if (t_min_next == t_next_y)
            {
                x_or_y_or_z = 1;
                t_next_y += delta_t_y;
                current_voxel_index[1] += sign_y;
                if (current_voxel_index[1] < 0 || current_voxel_index[1] >= V.y_size)
                    break;
            }
            else if (t_min_next == t_next_z)
            {
                x_or_y_or_z = 2;
                t_next_z += delta_t_z;
                current_voxel_index[2] += sign_z;
                if (current_voxel_index[2] < 0 || current_voxel_index[2] >= V.z_size)
                    break;
            }
            else
            {
                std::cout << "error in 3D-DDA ray delta" << t_min_next << t_next_x << "," << t_next_y << "," << t_next_z << std::endl;
                break;
            }

            current_v = &V.get_voxel(current_voxel_index[0], current_voxel_index[1], current_voxel_index[2]);
            if (current_v->exist) {
                // TRICK: I let the particle stop a little bit earlier than the true computed collision point to avoid the case that the particle is stucking inside the voxel
                // caused by floating point error
                // get the collision point
                glm::vec3 collision_point = old_position + (t_current)*ray_direction * 0.999f;// some trick
                //new_position = collision_point;
                //std::vector<float> collide_voxel_6_face = voxel_to_world_6_face(current_voxel_index[0], current_voxel_index[1], current_voxel_index[2]);
                std::vector<float> collide_voxel_6_face = voxel_to_world_6_face_extend(current_voxel_index[0], current_voxel_index[1], current_voxel_index[2]);//trick version
                // reverse the velocity  component that is in the direction of the collision face
                if (x_or_y_or_z == 0) {
                    new_position.x = collide_voxel_6_face[(1 + sign_x) / 2];
                    new_velocity.x = -old_velocity.x;
                    collision_point.x = new_position.x;
                }
                else if (x_or_y_or_z == 1) {
                    new_position.y = collide_voxel_6_face[(5 + sign_y) / 2];
                    new_velocity.y = -old_velocity.y;
                    collision_point.y = new_position.y;
                }
                else if (x_or_y_or_z == 2) {
                    new_position.z = collide_voxel_6_face[(9 + sign_z) / 2];
                    new_velocity.z = -old_velocity.z;
                    collision_point.z = new_position.z;
                }
                new_velocity *= 0.8f;

                // quickly detect if the particle's new bounced position is still inside the voxel (just fast approximation, not physical based)
                current_voxel_index = world_to_voxel(old_position, V);
                current_v = &V.get_voxel(current_voxel_index[0], current_voxel_index[1], current_voxel_index[2]);
                if (current_v->exist) {
                    new_position = collision_point;
                }


                break;
            }

        }

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
            recycle_list.push_back(i);
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
        p[i].currPos = new_position;

        // esitmation of the velocity
        p[i].estimated_velocity = (p[i].estimated_velocity) * 0.5f + (p[i].currPos - p[i].prevPos) / frameTimeDiff * 0.5f;

        // // ------simplest collision detection, just reverse the velocity if this pos has a voxel------
        // std::vector<int> voxel_index = world_to_voxel(new_position,V);
        // int x = voxel_index[0];
        // int y = voxel_index[1];
        // int z = voxel_index[2];
        // // avoid some cases that the voxel index is out of bound
        // /*if (x < 0) {
        // 	x = 0;
        // }*/
        // 
        // //std::cout << x << " " << y << " " << z << std::endl;
        // voxel & v = V.get_voxel(x, y, z);
        // if (v.exist) {
        //     
        //     std::cout << "collision" << std::endl;
        //     v.color = glm::vec4(1.f, 0.f, 0.f, 1.0f);
        //     new_velocity.x = -new_velocity.x;
        //     new_velocity.y = -new_velocity.y;
        //     new_velocity.z = -new_velocity.z;
        //     p[i].velocity = new_velocity;
        // 
        // }

    }

    // std::cout << "velocity: " << p[0].velocity.x <<" " << p[0].velocity.y << " " << p[0].velocity.z << std::endl;
    // std::cout << "true velocity: " << (p[0].currPos.x - p[0].prevPos.x)/frameTimeDiff << " " << (p[0].currPos.y - p[0].prevPos.y) / frameTimeDiff << " " << (p[0].currPos.z - p[0].prevPos.z) / frameTimeDiff << std::endl;
    // std::cout << "estimated velocity: " << p[0].estimated_velocity.x << " " << p[0].estimated_velocity.y << " " << p[0].estimated_velocity.z << std::endl;



    // didn't use this one, because it still looks right without updating the neighbourhood grid
    // if it looks correct, then don't change it
    // // update the neighbourhood grid, because the position has changed and we still need the neighbourhood grid for diffusion and erosion and deposition
    // G.clear_grid();
    // for (int i = 0; i < particle_num; i++) {
    // 
    //     std::vector<int> grid_index = G.world_to_grid(p[i].currPos);
    //     G.add_particle(grid_index[0], grid_index[1], grid_index[2], i);
    // }

    // diffusion and stuck check
    //#pragma omp parallel for
    for (int i = 0; i < particle_num; i++) {
        std::vector<int> current_grid = G.world_to_grid(p[i].currPos);
        std::vector<int> neighbour_particles = G.get_lower_neighbourhood(current_grid[0], current_grid[1], current_grid[2]);
        for (int& j : neighbour_particles) {
            if (i == j) {
                continue;
            }
            if (p[i].mass > particle_mass && p[j].mass < particle_maximum_mass) {
                glm::vec3 delta = (p[i].currPos - p[j].currPos);
                float r = length(delta);
                if (r < smoothing_length && p[i].mass > p[j].mass && p[i].currPos.y - p[j].currPos.y >= -smoothing_length * 0.01f) {
                    // particles lose mass
                    float weight = length(p[i].mass * (p[i].pamameters[1]) / (p[i].pamameters[0]) * -45.f / (PI_FLOAT * glm::pow(smoothing_length, 6.f)) * glm::pow(smoothing_length - r, 2.f) * glm::normalize(delta));
                    p[i].mass -= frameTimeDiff * diffusion_rate * weight;
                    // particles gain mass
                    p[j].mass += frameTimeDiff * diffusion_rate * weight;
                }
            }
        }
        // stuck check
        // voxel * current_V = &V.get_voxel(current_grid[0], current_grid[1], current_grid[2]);
        // if (current_V->exist) {
        // 	p[i].stuck_count++;
        // }
        // else
        // {
        //     p[i].stuck_count = 0;
        // }
        //  if it stucks for too long, then treat it as deposited into the voxel
        //  unleash its mass and recycle it
        // if (p[i].stuck_count > 40)
        // {
        //     current_V->density += (p[i].mass - particle_mass) * voxel_damage_scale / particle_mass_transfer_ratio;
        //     current_V->update_color();
        //     // clear particles mass
        //     p[i].mass = particle_mass;
        // 	recycle_list.push_back(i);
        // }

    }

}

void calculate_voxel_erosion(std::vector<particle>& p, float frameTimeDiff, voxel_field& V, neighbourhood_grid& G, std::vector<int>& recycle_list) {
    float voxel_pressure_range = smoothing_length * 2.0;
    float voxel_deposition_range = smoothing_length * 2.0;
    //#pragma omp parallel for collapse(3)  // unfortunately, simple parallelization does not work here when deposition is calculated
    for (int i = 0; i < V.x_size; i++) {
        int G_x = i;
        for (int j = 0; j < V.y_size; j++) {
            int G_y = j;
            for (int k = 0; k < V.z_size; k++) {
                int G_z = k;
                voxel* v = &V.get_voxel(i, j, k);
                if (v->exist) {
                    // voxel's i j k is the same as neighbour_particles's i j k
                    std::vector<int> neighbour_particles = G.get_neighbourhood(G_x, G_y, G_z, 2);
                    std::vector<int> upper_neighbour_particles = G.get_upper_neighbourhood(G_x, G_y, G_z, 2);
                    // erosion part
                    if (!v->not_destroyable) {
                        for (int& n : neighbour_particles) {
                            glm::vec3 delta = (p[n].currPos - voxel_to_world(i, j, k));
                            float r = length(delta);
                            if (r < voxel_pressure_range && p[n].mass < particle_maximum_mass) {
                                // voxels lose mass because of the particles (erosion)
                                float weight = length(p[n].mass * (p[n].pamameters[1]) / (p[n].pamameters[0]) * -45.f / (PI_FLOAT * glm::pow(voxel_pressure_range, 6.f)) * glm::pow(voxel_pressure_range - r, 2.f) * glm::normalize(delta));
                                v->density -= frameTimeDiff * voxel_damage_scale * weight;
                                v->update_color();

                                // particles gain mass from voxels (carry the mass)
                                p[n].mass += frameTimeDiff * particle_mass_transfer_ratio * weight;



                            }
                            if (v->density < voxel_destroy_density_threshold) {
                                v->exist = false;
                                v->color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                            }

                        }
                    }
                    else {
                        for (int& n : neighbour_particles) {
                            glm::vec3 delta = (p[n].currPos - voxel_to_world(i, j, k));
                            float r = length(delta);
                            if (r < voxel_pressure_range && p[n].mass < particle_maximum_mass && v->density>voxel_not_destroyable_min_density) {
                                // voxels lose mass because of the particles (erosion)
                                float weight = length(p[n].mass * (p[n].pamameters[1]) / (p[n].pamameters[0]) * -45.f / (PI_FLOAT * glm::pow(voxel_pressure_range, 6.f)) * glm::pow(voxel_pressure_range - r, 2.f) * glm::normalize(delta));
                                v->density -= frameTimeDiff * voxel_damage_scale * weight;
                                v->update_color();

                                // particles gain mass from voxels (carry the mass)
                                p[n].mass += frameTimeDiff * particle_mass_transfer_ratio * weight;



                            }
                            if (v->density < voxel_destroy_density_threshold) {
                                std::cout << "error: non-destroyable voxel destroyed" << std::endl;
                                v->color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                            }

                        }

                    }
                    // deposition part
                    bool new_voxel_created = false;
                    for (int& n : upper_neighbour_particles) {
                        glm::vec3 delta = (p[n].currPos - voxel_to_world(i, j, k));
                        float r = length(delta);
                        if (r < voxel_deposition_range && p[n].mass > particle_mass) {
                            // voxels get mass 
                            float weight = length(p[n].mass * (p[n].pamameters[1]) / (p[n].pamameters[0]) * -45.f / (PI_FLOAT * glm::pow(voxel_deposition_range, 6.f)) * glm::pow(voxel_deposition_range - r, 2.f) * glm::normalize(delta));


                            // adjust this part to control the deposition-erosion speed, very important here
                            weight *= 1.0f / (length(p[n].estimated_velocity) * 0.55f + 0.75f);// speed penalty, if the particle is moving fast, then it will deposit less mass under the same time interval

                            if (v->density < voxel_maximum_density) {// if the voxel is not full, then it can gain mass
                                v->density += frameTimeDiff * voxel_damage_scale * weight;
                                v->update_color();
                                // particles lose mass
                                p[n].mass -= frameTimeDiff * particle_mass_transfer_ratio * weight;


                            }
                            else {// otherwise, it will try to create a new solid voxel right on its upper face
                                int upper_voxel_x = i;
                                int upper_voxel_y = j + 1;
                                int upper_voxel_z = k;
                                if (upper_voxel_y < V.y_size) {
                                    voxel* upper_v = &V.get_voxel(upper_voxel_x, upper_voxel_y, upper_voxel_z);
                                    if (!upper_v->exist) {
                                        // generate a new voxel
                                        upper_v->exist = true;
                                        upper_v->density = v->density - voxel_density;
                                        upper_v->update_color();
                                        // the current voxel loses a part of the mass and share it to the new voxel
                                        v->density -= upper_v->density;
                                        v->update_color();
                                        // then, we need to destroy and re-create the particles that are inside the new voxel
                                        // let's first break this for loop and find out which particles are inside the new voxel,
                                        new_voxel_created = true;
                                        break;
                                    }
                                }
                            }

                        }
                    }
                    // generation of new voxel part
                    if (new_voxel_created) {// re-create the particles that are inside the new voxel
                        std::vector<int> stucked_particles = G.get_neighbourhood(i, j + 1, k, 1);
                        voxel* new_V = &V.get_voxel(i, j + 1, k);
                        if (!new_V->exist) {
                            std::cout << "error in voxel deposition" << std::endl;
                        }
                        for (int& n : stucked_particles) {
                            glm::vec3 delta = (p[n].currPos - voxel_to_world(i, j + 1, k));
                            float r = length(delta);
                            // still trick, just to avoid the case that the particle is still stucking inside the voxel close to current one
                            // if the particle is still inside the voxel at this frame and we didn't remove it, 
                            // then it will be very likely to stuck in the voxel again in the next frame
                            // so now we remove all particles that are square_root(3) * voxel_size_scale away from the new voxel center
                            if (r < voxel_size_scale * 1.05) {
                                new_V->density += (p[n].mass - particle_mass) * voxel_damage_scale / particle_mass_transfer_ratio;
                                new_V->is_new = true;
                                new_V->update_color();
                                // clear particles mass
                                p[n].mass = particle_mass;
                                recycle_list.push_back(n);
                            }

                        }


                    }



                }

            }
        }
    }


}

void recycle_particle(std::vector<particle>& p, std::vector<int>& recycle_list) {
    particle p1;
    p1.prevPos = glm::vec3(0.0f, 0.0f, 0.0f);
    p1.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    p1.acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    p1.pamameters = glm::vec3(0.0f, 0.0f, 0.0f);
    p1.deltaCs = glm::vec3(0.0f, 0.0f, 0.0f);
    p1.mass = particle_mass;
    p1.stuck_count = 0;

    for (int n : recycle_list) {
        p1.currPos = generateRandomVec3();
        p1.currPos.y /= 4;
        p1.currPos.y += (y_max - y_min) * 3 / 4;
        p1.currPos.x /= 4;
        //p1.currPos.x += (x_max - x_min) * 3 / 4;
        p[n] = p1;
    }
    recycle_list.clear();
}

