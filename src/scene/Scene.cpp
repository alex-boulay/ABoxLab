#include "Scene.hpp"
#include <algorithm>

int Scene::addObject(const std::string& name, PrimitiveType primitiveType) {
  SceneObject obj;
  obj.id = nextObjectId++;
  obj.name = name;
  obj.mesh = Mesh::createPrimitive(primitiveType);
  objects.push_back(std::move(obj));
  return obj.id;
}

int Scene::duplicateObject(int objectId) {
  SceneObject* src = getObject(objectId);
  if (!src) return -1;

  SceneObject copy = *src;
  copy.id = nextObjectId++;
  copy.name = src->name + " (copy)";
  // Offset position slightly so it's visible
  copy.transform.position[0] += 1.0f;
  int id = copy.id;
  objects.push_back(std::move(copy));
  return id;
}

void Scene::removeObject(int objectId) {
  objects.erase(
    std::remove_if(objects.begin(), objects.end(),
      [objectId](const SceneObject& o) { return o.id == objectId; }),
    objects.end());
}

SceneObject* Scene::getObject(int id) {
  for (auto& obj : objects) {
    if (obj.id == id) return &obj;
  }
  return nullptr;
}
