#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "loadObj.hpp"
#include <iostream>

//Konfiguriert den tinyObjLoader
bool LoadObj::objLoader(const std::string& filename, std::vector<Vertex>& outVertices) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(
        &attrib, &shapes, &materials,
        &warn, &err,
        filename.c_str(),
        "models/mtl/",
        true
    );

    if (!warn.empty()) 
        std::cerr << "[tinyobjloader WARN] " << warn << "\n";

    if (!err.empty()) 
        std::cerr << "[tinyobjloader ERR] " << err << "\n";

    if (!ret) 
        return false;

    //alle Shapes
    for (const auto& shape : shapes) 
    {
        //Jedes face-Dreieck
        for (const auto& index : shape.mesh.indices) 
        {
            Vertex vertex{};

            //Position
            if (index.vertex_index >= 0) 
            {
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };
            }

            //Texture-coordinates
            if (index.texcoord_index >= 0) 
            {
                vertex.tex = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Flip Y
                };
            }
            outVertices.push_back(vertex);
        }
    }
    std::cout << "OBJ geladen: " << filename <<" :)"<< "\n";
    return true;
}
