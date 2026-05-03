#pragma once

#include "Mesh.hpp"
#include <vector>
#include <string>

struct Transform {
  float position[3] = { 0, 0, 0 };
  float rotation[3] = { 0, 0, 0 }; // euler angles in degrees
  float scale[3]    = { 1, 1, 1 };
};

struct SceneObject {
  int id;
  std::string name;
  Mesh mesh;
  Transform transform;
  std::string vertShaderPath;
  std::string fragShaderPath;
  bool visible = true;
};

struct Camera {
  float position[3] = { 0, 0, 3 };
  float target[3]   = { 0, 0, 0 };
  float up[3]       = { 0, 1, 0 };
  float fov = 60.0f;   // degrees
  float nearPlane = 0.1f;
  float farPlane = 100.0f;
};

class Scene {
public:
  Scene() = default;

  int addObject(const std::string& name, PrimitiveType primitiveType);
  int duplicateObject(int objectId);
  void removeObject(int objectId);

  SceneObject* getObject(int id);
  std::vector<SceneObject>& getObjects() { return objects; }
  const std::vector<SceneObject>& getObjects() const { return objects; }

  Camera& getCamera() { return camera; }
  const Camera& getCamera() const { return camera; }

private:
  std::vector<SceneObject> objects;
  Camera camera;
  int nextObjectId = 0;
};
