// MirrorSystem.cpp
#include "MirrorSystem.hpp"
#include <iostream>

static glm::mat4 normalToRotation(
    const glm::vec3& targetNormal,
    const glm::vec3& defaultNormal = glm::vec3(0.0f, 0.0f, 1.0f)
) {
    glm::vec3 n0 = glm::normalize(defaultNormal);
    glm::vec3 n1 = glm::normalize(targetNormal);

    float cosTheta = glm::clamp(glm::dot(n0, n1), -1.0f, 1.0f);

    // Gleiche Richtung → keine Rotation
    if (cosTheta > 0.9999f) {
        return glm::mat4(1.0f);
    }

    // Entgegengesetzte Richtung → 180°
    if (cosTheta < -0.9999f) {
        glm::vec3 axis = glm::cross(n0, glm::vec3(1.0f, 0.0f, 0.0f));
        if (glm::length(axis) < 0.001f) {
            axis = glm::cross(n0, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        axis = glm::normalize(axis);
        return glm::rotate(glm::mat4(1.0f), glm::pi<float>(), axis);
    }

    // Allgemeiner Fall
    glm::vec3 axis = glm::normalize(glm::cross(n0, n1));
    float angle = acos(cosTheta);

    return glm::rotate(glm::mat4(1.0f), angle, axis);
}

glm::mat4 MirrorSystem::calculateReflectionMatrix(const glm::vec3& planePoint, 
                                                   const glm::vec3& planeNormal) {
    // Ebenengleichung: n·x + d = 0
    // d = -n·P (wobei P ein Punkt auf der Ebene ist)
    float d = -glm::dot(planeNormal, planePoint);
    
    glm::mat4 reflectionMatrix = glm::mat4(1.0f);
    
    // Diagonale: 1 - 2*n_i*n_i
    reflectionMatrix[0][0] = 1.0f - 2.0f * planeNormal.x * planeNormal.x;
    reflectionMatrix[1][1] = 1.0f - 2.0f * planeNormal.y * planeNormal.y;
    reflectionMatrix[2][2] = 1.0f - 2.0f * planeNormal.z * planeNormal.z;
    
    // Off-Diagonal: -2*n_i*n_j
    reflectionMatrix[0][1] = -2.0f * planeNormal.x * planeNormal.y;
    reflectionMatrix[0][2] = -2.0f * planeNormal.x * planeNormal.z;
    reflectionMatrix[1][0] = -2.0f * planeNormal.y * planeNormal.x;
    reflectionMatrix[1][2] = -2.0f * planeNormal.y * planeNormal.z;
    reflectionMatrix[2][0] = -2.0f * planeNormal.z * planeNormal.x;
    reflectionMatrix[2][1] = -2.0f * planeNormal.z * planeNormal.y;
    
    // Translation: -2*d*n
    reflectionMatrix[3][0] = -2.0f * d * planeNormal.x;
    reflectionMatrix[3][1] = -2.0f * d * planeNormal.y;
    reflectionMatrix[3][2] = -2.0f * d * planeNormal.z;
    reflectionMatrix[3][3] = 1.0f;
    
    return reflectionMatrix;
}

void MirrorSystem::addMirror(Scene* scene, const MirrorConfig& config) {
    MirrorData mirror;
    mirror.position = config.position;
    mirror.normal = glm::normalize(config.normal);
    
    // Transformationsmatrix für den Spiegel
    mirror.transform = glm::mat4(1.0f);
    mirror.transform = glm::translate(mirror.transform, config.position);
    mirror.transform *= normalToRotation(config.normal);
    mirror.transform = glm::scale(mirror.transform, config.scale);
    
    createMirrorObjects(scene, config, mirror);
    _mirrors.push_back(mirror);
    
    std::cout << "Mirror added at position (" 
              << config.position.x << ", " 
              << config.position.y << ", " 
              << config.position.z << ")" << std::endl;
    std::cout << "Mirror normal: (" 
              << mirror.normal.x << ", " 
              << mirror.normal.y << ", " 
              << mirror.normal.z << ")" << std::endl;
}

void MirrorSystem::createMirrorObjects(Scene* scene, const MirrorConfig& config, 
                                       MirrorData& mirror) {
    // PASS 1: Spiegel-Markierung (schreibt in Stencil)
    RenderObject mirrorMark = _factory->createMirror(
        mirror.transform, _renderPass, PipelineType::MIRROR_MARK);
    scene->setMirrorMarkObject(mirrorMark);
    mirror.markIndex = scene->getMirrorMarkIndex();
    
    // PASS 3: Spiegel mit Transparenz
    RenderObject mirrorBlend = _factory->createMirror(
        mirror.transform, _renderPass, PipelineType::MIRROR_BLEND);
    scene->setMirrorBlendObject(mirrorBlend);
    mirror.blendIndex = scene->getMirrorBlendIndex();
}

void MirrorSystem::addReflectableObject(size_t objectIndex) {
    _reflectableObjects.push_back(objectIndex);
    std::cout << "Object " << objectIndex << " marked as reflectable" << std::endl;
}

void MirrorSystem::createReflections(Scene* scene) {
    // Für jeden Spiegel
    for (const auto& mirror : _mirrors) {
        glm::mat4 reflectionMatrix = calculateReflectionMatrix(
            mirror.position, mirror.normal);
        
        // Für jedes zu spiegelnde Objekt
        for (size_t objIndex : _reflectableObjects) {
            createReflectedObject(scene, objIndex, mirror);
        }
        
        std::cout << "Created " << _reflectableObjects.size() 
                  << " reflections for mirror" << std::endl;
    }
}

void MirrorSystem::createReflectedObject(Scene* scene, size_t objectIndex, 
                                         const MirrorData& mirror) {
    const auto& originalObj = scene->getObject(objectIndex);
    
    // Reflexionsmatrix berechnen
    glm::mat4 reflectionMatrix = calculateReflectionMatrix(
        mirror.position, mirror.normal);
    
    // Gespiegelte Transformationsmatrix
    glm::mat4 reflectedMatrix = reflectionMatrix * originalObj.modelMatrix;
    
    // Neues gespiegeltes RenderObject erstellen
    // Hier müssen wir die Original-Parameter des Objekts kennen
    // Das ist eine Limitierung - in einer vollständigen Implementierung
    // würde man diese Informationen im RenderObject speichern
    
    // Für jetzt: Erstelle eine Kopie und ändere nur die Matrix
    RenderObject reflectedObj = originalObj;
    reflectedObj.modelMatrix = reflectedMatrix;
    
    // Pipeline für gespiegelte Objekte verwenden
    GraphicsPipeline* reflectedPipeline = new GraphicsPipeline(
        originalObj.pipeline->getDevice(),
        originalObj.pipeline->getColorFormat(),
        originalObj.pipeline->getDepthFormat(),
        "shaders/testapp.vert.spv",
        "shaders/testapp.frag.spv",
        _renderPass,
        scene->getDescriptorSetLayout(),
        PipelineType::MIRROR_REFLECT
    );
    
    reflectedObj.pipeline = reflectedPipeline;
    
    scene->addReflectedObject(reflectedObj, objectIndex);
}
