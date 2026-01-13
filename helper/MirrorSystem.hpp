// MirrorSystem.hpp
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "../Scene.hpp"
#include "../ObjectFactory.hpp"
class ObjectFactory;
struct MirrorConfig {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 scale = glm::vec3(1.5f, 2.5f, 0.1f);
};

class MirrorSystem {
public:
    MirrorSystem(VkDevice device,ObjectFactory* factory, VkRenderPass renderPass)
        :  _factory(factory), _renderPass(renderPass), _device(device) {}

    // Fügt einen Spiegel zur Szene hinzu
    void addMirror(Scene* scene, const MirrorConfig& config);

    // Fügt ein Objekt hinzu, das gespiegelt werden soll
    void addReflectableObject(size_t objectIndex);

    // Erstellt alle gespiegelten Objekte für alle Spiegel
    void createReflections(Scene* scene);

    //Resettet Spiegel und erschafft neue Reflexion, nötig für bewegende Objekte
    void updateReflections(Scene* scene, size_t objectIndex);

    // Berechnet die Reflexionsmatrix für eine Ebene
    static glm::mat4 calculateReflectionMatrix(const glm::vec3& planePoint, 
                                               const glm::vec3& planeNormal);

private:
    struct MirrorData {
        glm::vec3 position;
        glm::vec3 normal;
        glm::mat4 transform;
        size_t markIndex;
        size_t blendIndex;
    };

    ObjectFactory* _factory;
    VkRenderPass _renderPass;
    std::vector<MirrorData> _mirrors;
    std::vector<size_t> _reflectableObjects;
    VkDevice _device;

    void createMirrorObjects(Scene* scene,  MirrorData& mirror);
    void createReflectedObject(Scene* scene, size_t objectIndex, const MirrorData& mirror);
};
