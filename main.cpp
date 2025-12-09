#include "Common.h"
#include "OpenGLCommon.h"
#include "OpenGLMarkerObjects.h"
#include "OpenGLBgEffect.h"
#include "OpenGLMesh.h"
#include "OpenGLViewer.h"
#include "OpenGLWindow.h"
#include "TinyObjLoader.h"
#include "OpenGLSkybox.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>
#include <string>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef __Main_cpp__
#define __Main_cpp__

#ifdef __APPLE__
#define CLOCKS_PER_SEC 100000
#endif

class MyDriver : public OpenGLViewer
{
    std::vector<OpenGLTriangleMesh*> mesh_object_array;
    OpenGLBgEffect* bgEffect = nullptr;
    OpenGLSkybox* skybox = nullptr;
    clock_t startTime;

    // new years stuff
    OpenGLTriangleMesh* ground = nullptr;
    std::vector<OpenGLTriangleMesh*> buildings;
    OpenGLTriangleMesh* pole = nullptr;
    OpenGLTriangleMesh* ball = nullptr;

public:
    virtual void Initialize()
    {
        draw_axes = false;
        startTime = clock();
        OpenGLViewer::Initialize();
    }

    // making procedural buildings
    OpenGLTriangleMesh* Create_Building(float width, float depth, float height, Vector3f position) {
        std::vector<Vector3> vertices;
        std::vector<Vector3i> elements;
        std::vector<Vector2> uvs;

        // bottom face (y=0)
        vertices.push_back(Vector3(-width / 2.0f, 0.0f, -depth / 2.0f));
        vertices.push_back(Vector3(width / 2.0f, 0.0f, -depth / 2.0f));
        vertices.push_back(Vector3(width / 2.0f, 0.0f, depth / 2.0f));
        vertices.push_back(Vector3(-width / 2.0f, 0.0f, depth / 2.0f));

        // top face (y=height)
        vertices.push_back(Vector3(-width / 2.0f, height, -depth / 2.0f));
        vertices.push_back(Vector3(width / 2.0f, height, -depth / 2.0f));
        vertices.push_back(Vector3(width / 2.0f, height, depth / 2.0f));
        vertices.push_back(Vector3(-width / 2.0f, height, depth / 2.0f));

        // front face
        elements.push_back(Vector3i(0, 1, 5));
        elements.push_back(Vector3i(0, 5, 4));
        // right face
        elements.push_back(Vector3i(1, 2, 6));
        elements.push_back(Vector3i(1, 6, 5));
        // back face
        elements.push_back(Vector3i(2, 3, 7));
        elements.push_back(Vector3i(2, 7, 6));
        // left face
        elements.push_back(Vector3i(3, 0, 4));
        elements.push_back(Vector3i(3, 4, 7));
        // top face
        elements.push_back(Vector3i(4, 5, 6));
        elements.push_back(Vector3i(4, 6, 7));
        // bottom face
        elements.push_back(Vector3i(0, 2, 1));
        elements.push_back(Vector3i(0, 3, 2));

        // texture mapping
        for (int i = 0; i < 8; i++) {
            uvs.push_back(Vector2(0.0f, 0.0f));
        }

        auto building = Add_Tri_Mesh_Object(vertices, elements);
        building->mesh.Uvs() = uvs;

        // positioning
        Matrix4f t;
        t << 1.0f, 0.0f, 0.0f, position[0],
            0.0f, 1.0f, 0.0f, position[1],
            0.0f, 0.0f, 1.0f, position[2],
            0.0f, 0.0f, 0.0f, 1.0f;
        building->Set_Model_Matrix(t);

        return building;
    }

    // creating the ground (making it slanted to cover up building bottoms)
    OpenGLTriangleMesh* Create_Ground_Plane(float width, float length, float A, float B, float C, int nx = 80, int nz = 80)
    {
        std::vector<Vector3> vertices;
        std::vector<Vector3i> elements;
        std::vector<Vector2> uvs;

        float halfW = width * 0.5f;
        float dx = width / nx;
        float dz = length / nz;

        for (int iz = 0; iz <= nz; iz++) {
            float z = -iz * dz;
            for (int ix = 0; ix <= nx; ix++) {
                float x = -halfW + ix * dx;
                float y = A * x + B * z + C;
                vertices.push_back(Vector3(x, y, z));
                uvs.push_back(Vector2((float)ix / nx, (float)iz / nz * 5.0f));
            }
        }

        for (int iz = 0; iz < nz; iz++) {
            for (int ix = 0; ix < nx; ix++) {
                int i0 = iz * (nx + 1) + ix;
                int i1 = i0 + 1;
                int i2 = i0 + (nx + 1);
                int i3 = i2 + 1;
                elements.push_back(Vector3i(i0, i1, i2));
                elements.push_back(Vector3i(i1, i3, i2));
            }
        }

        auto mesh = Add_Tri_Mesh_Object(vertices, elements);
        mesh->mesh.Uvs() = uvs;
        return mesh;
    }

    // procedurally generate sphere mesh
    OpenGLTriangleMesh* Create_Sphere(float radius, int slices, int stacks, Vector3f center)
    {
        std::vector<Vector3>  vertices;
        std::vector<Vector3i> elements;
        std::vector<Vector2>  uvs;

        // generate vertices
        for (int i = 0; i <= stacks; ++i) {
            float v = (float)i / (float)stacks;
            float phi = v * M_PI;

            for (int j = 0; j <= slices; ++j) {
                float u = (float)j / (float)slices;
                float theta = u * 2.0f * M_PI;

                float x = radius * sinf(phi) * cosf(theta);
                float y = radius * cosf(phi);
                float z = radius * sinf(phi) * sinf(theta);

                vertices.push_back(Vector3(x, y, z));
                uvs.push_back(Vector2(u, v));
            }
        }

        int vertsPerRow = slices + 1;
        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                int i0 = i * vertsPerRow + j;
                int i1 = i0 + 1;
                int i2 = i0 + vertsPerRow;
                int i3 = i2 + 1;

                elements.push_back(Vector3i(i0, i2, i1));
                elements.push_back(Vector3i(i1, i2, i3));
            }
        }

        auto sphere = Add_Tri_Mesh_Object(vertices, elements);
        sphere->mesh.Uvs() = uvs;

        // put it in world
        Matrix4f t;
        t << 1, 0, 0, center[0],
            0, 1, 0, center[1],
            0, 0, 1, center[2],
            0, 0, 0, 1;
        sphere->Set_Model_Matrix(t);

        return sphere;
    }

    OpenGLTriangleMesh* Create_Cylinder(
        float radius,
        float height,
        int slices,
        Vector3f baseCenter)
    {
        std::vector<Vector3> vertices;
        std::vector<Vector3i> elements;
        std::vector<Vector2> uvs;

        // generate top and bottom vertices
        for (int i = 0; i <= slices; i++) {
            float t = float(i) / float(slices);
            float angle = t * 2.0f * M_PI;
            float x = radius * cos(angle);
            float z = radius * sin(angle);

            // bottom vertex
            vertices.push_back(Vector3(x, 0.0f, z));
            uvs.push_back(Vector2(t, 0.0f));

            // top vertex
            vertices.push_back(Vector3(x, height, z));
            uvs.push_back(Vector2(t, 1.0f));
        }

        // make triangles
        for (int i = 0; i < slices; i++) {
            int i0 = 2 * i;
            int i1 = i0 + 1;
            int i2 = i0 + 2;
            int i3 = i0 + 3;

            // side quad split into triangles
            elements.push_back(Vector3i(i0, i2, i1));
            elements.push_back(Vector3i(i1, i2, i3));
        }

        // create mesh object
        auto cyl = Add_Tri_Mesh_Object(vertices, elements);
        cyl->mesh.Uvs() = uvs;

        // put in world
        Matrix4f T;
        T << 1, 0, 0, baseCenter[0],
            0, 1, 0, baseCenter[1],
            0, 0, 1, baseCenter[2],
            0, 0, 0, 1;
        cyl->Set_Model_Matrix(T);

        return cyl;
    }



    virtual void Initialize_Data()
    {
        //// Load shaders
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/basic.vert", "shaders/basic.frag", "basic");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/building.vert", "shaders/building.frag", "building");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/street.vert", "shaders/street.frag", "street");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/basic.vert", "shaders/ball.frag", "ball");
        OpenGLShaderLibrary::Instance()->Add_Shader_From_File("shaders/basic.vert", "shaders/pole.frag", "pole");

        //// Load textures
        OpenGLTextureLibrary::Instance()->Add_Texture_From_File("tex/star.png", "star_color");

        // lots of lights for ball drop
        opengl_window->Add_Light(Vector3f(0.0f, 8.0f, 0.0f), Vector3f(0.1f, 0.1f, 0.1f), Vector3f(2.0f, 2.0f, 2.0f), Vector3f(1.0f, 1.0f, 1.0f));
        opengl_window->Add_Light(Vector3f(0.0f, 5.0f, -3.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.8f, 0.0f, 0.8f), Vector3f(0.8f, 0.5f, 0.8f));


        // background - dark gradient
        {
            auto bg = Add_Interactive_Object<OpenGLBackground>();
            bg->Set_Color(OpenGLColor(0.01f, 0.01f, 0.05f, 1.0f), OpenGLColor(0.05f, 0.05f, 0.15f, 1.0f));
            bg->Initialize();
        }

        // making ground
        {
            //ground = Create_Ground_Plane(30.0f);  // 30 units 
            ground = Create_Ground_Plane(15.0f, 60.0f, 0.0f, -0.067f, 0.0f, 120, 120);

            Matrix4f t;
            t << 1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, -2.0f,
                0.0f, 0.0f, 1.0f, 10.0f,
                0.0f, 0.0f, 0.0f, 1.0f;
            ground->Set_Model_Matrix(t);

            // colorings (still working on this)
            ground->Set_Ka(Vector3f(0.05f, 0.05f, 0.05f));
            ground->Set_Kd(Vector3f(0.25f, 0.25f, 0.25f));
            ground->Set_Ks(Vector3f(0.05f, 0.05f, 0.05f));
            ground->Set_Shininess(8.0f);

            ground->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("street"));
        }

        //making the buildings
        {

            // left front
            auto b1 = Create_Building(2.0f, 3.0f, 8.0f, Vector3f(-4.0f, -3.0f, 0.0f));
            b1->Set_Ka(Vector3f(0.1f, 0.1f, 0.1f));
            b1->Set_Kd(Vector3f(0.3f, 0.35f, 0.35f));
            b1->Set_Ks(Vector3f(0.2f, 0.2f, 0.2f));
            b1->Set_Shininess(32.0f);
            b1->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("building"));
            buildings.push_back(b1);

            // right front
            auto b2 = Create_Building(2.0f, 3.0f, 8.5f, Vector3f(4.0f, -3.0f, 0.0f));
            b2->Set_Ka(Vector3f(0.1f, 0.1f, 0.1f));
            b2->Set_Kd(Vector3f(0.3f, 0.3f, 0.35f));
            b2->Set_Ks(Vector3f(0.2f, 0.2f, 0.2f));
            b2->Set_Shininess(32.0f);
            b2->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("building"));
            buildings.push_back(b2);

            // left mid
            auto b3 = Create_Building(1.5f, 2.5f, 9.5f, Vector3f(-3.5f, -2.0f, -5.0f));
            b3->Set_Ka(Vector3f(0.1f, 0.1f, 0.1f));
            b3->Set_Kd(Vector3f(0.25f, 0.25f, 0.3f));
            b3->Set_Ks(Vector3f(0.2f, 0.2f, 0.2f));
            b3->Set_Shininess(32.0f);
            b3->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("building"));
            buildings.push_back(b3);

            // right mid
            auto b4 = Create_Building(1.5f, 2.5f, 10.0f, Vector3f(3.5f, -2.0f, -5.0f));
            b4->Set_Ka(Vector3f(0.1f, 0.1f, 0.1f));
            b4->Set_Kd(Vector3f(0.25f, 0.3f, 0.3f));
            b4->Set_Ks(Vector3f(0.2f, 0.2f, 0.2f));
            b4->Set_Shininess(32.0f);
            b4->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("building"));
            buildings.push_back(b4);

            // left back
            auto b5 = Create_Building(1.0f, 2.0f, 11.0f, Vector3f(-3.0f, -1.0f, -10.0f));
            b5->Set_Ka(Vector3f(0.1f, 0.1f, 0.1f));
            b5->Set_Kd(Vector3f(0.2f, 0.2f, 0.25f));
            b5->Set_Ks(Vector3f(0.2f, 0.2f, 0.2f));
            b5->Set_Shininess(32.0f);
            b5->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("building"));
            buildings.push_back(b5);

            // right back
            auto b6 = Create_Building(1.0f, 2.0f, 12.0f, Vector3f(3.0f, -1.0f, -10.0f));
            b6->Set_Ka(Vector3f(0.1f, 0.1f, 0.1f));
            b6->Set_Kd(Vector3f(0.2f, 0.25f, 0.25f));
            b6->Set_Ks(Vector3f(0.2f, 0.2f, 0.2f));
            b6->Set_Shininess(32.0f);
            b6->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("building"));
            buildings.push_back(b6);
        }

        // pole
        {
            float poleRadius = 0.15f;
            float poleHeight = 4.0f;

            pole = Create_Cylinder(poleRadius, poleHeight, 40, Vector3f(0.0f, -1.0f, -10.0f));

            pole->Set_Ka(Vector3f(0.08f, 0.08f, 0.08f));
            pole->Set_Kd(Vector3f(0.7f, 0.7f, 0.75f));
            pole->Set_Ks(Vector3f(1.0f, 1.0f, 1.0f));
            pole->Set_Shininess(128.0f);

            pole->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("pole"));
        }


        // ball
        {
            float ballRadius = 1.2f;

            ball = Create_Sphere(ballRadius, 40, 40, Vector3f(0.0f, 3.9f, -10.0f));

            ball->Set_Ka(Vector3f(0.5f, 0.5f, 0.5f));
            ball->Set_Kd(Vector3f(0.9f, 0.9f, 0.9f));
            ball->Set_Ks(Vector3f(1.0f, 1.0f, 1.0f));
            ball->Set_Shininess(128.0f);

            ball->Add_Shader_Program(OpenGLShaderLibrary::Get_Shader("ball"));
        }

        //// This for-loop updates the rendering model for each object on the list
        for (auto& mesh_obj : mesh_object_array) {
            Set_Polygon_Mode(mesh_obj, PolygonMode::Fill);
            Set_Shading_Mode(mesh_obj, ShadingMode::TexAlpha);
            mesh_obj->Set_Data_Refreshed();
            mesh_obj->Initialize();
        }
        Toggle_Play();
    }

    //// add mesh object by reading an .obj file
    OpenGLTriangleMesh* Add_Obj_Mesh_Object(std::string obj_file_name)
    {
        auto mesh_obj = Add_Interactive_Object<OpenGLTriangleMesh>();
        Array<std::shared_ptr<TriangleMesh<3>>> meshes;
        Obj::Read_From_Obj_File_Discrete_Triangles(obj_file_name, meshes);

        mesh_obj->mesh = *meshes[0];
        std::cout << "load tri_mesh from obj file, #vtx: " << mesh_obj->mesh.Vertices().size() << ", #ele: " << mesh_obj->mesh.Elements().size() << std::endl;

        mesh_object_array.push_back(mesh_obj);
        return mesh_obj;
    }

    //// add mesh object by reading an array of vertices and an array of elements
    OpenGLTriangleMesh* Add_Tri_Mesh_Object(const std::vector<Vector3>& vertices,
        const std::vector<Vector3i>& elements)
    {
        auto obj = Add_Interactive_Object<OpenGLTriangleMesh>();
        mesh_object_array.push_back(obj);

        obj->mesh.Vertices() = vertices;
        obj->mesh.Elements() = elements;

        return obj;
    }

    //// Go to next frame
    virtual void Toggle_Next_Frame()
    {
        float time = GLfloat(clock() - startTime) / CLOCKS_PER_SEC;

        // ball drop
        if (ball) {
            float dropTime = 5.0f;
            float cycle = std::fmod(time, dropTime + 2.0f);

            float y_pos;
            if (cycle < dropTime) {
                // drop
                float normalizedTime = cycle / dropTime;
                float easeOut = 1.0f - pow(1.0f - normalizedTime, 3.0f);
                y_pos = 3.9f - (easeOut * 3.4f);
            }
            else {
                // reset to top
                y_pos = 3.9f;
            }

            Matrix4f t;
            t << 1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, y_pos,
                0.0f, 0.0f, 1.0f, -10.0f,
                0.0f, 0.0f, 0.0f, 1.0f;
            ball->Set_Model_Matrix(t);
        }


        // Update time uniform for all animated objects
        for (auto& mesh_obj : mesh_object_array) {
            mesh_obj->setTime(time);
        }

        if (bgEffect) {
            bgEffect->setResolution((float)Win_Width(), (float)Win_Height());
            bgEffect->setTime(time);
            bgEffect->setFrame(frame++);
        }

        if (skybox) {
            skybox->setTime(time);
        }

        OpenGLViewer::Toggle_Next_Frame();
    }

    virtual void Run()
    {
        OpenGLViewer::Run();
    }
};

int main(int argc, char* argv[])
{
    MyDriver driver;
    driver.Initialize();
    driver.Run();
}

#endif
