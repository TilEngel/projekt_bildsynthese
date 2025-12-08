#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "GraphicsPipeline.hpp"

// Führt das Laden eines OBJ-Files aus
class LoadObj {
public:
    bool objLoader(const std::string& filename, std::vector<Vertex>& outVertices);
};
