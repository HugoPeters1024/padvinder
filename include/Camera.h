#pragma once
#include <precomp.h>

class Camera {
public:
    Camera(GLFWwindow* window);

    void update(float dt);
    glm::vec3 getViewDir() const;
    glm::mat4 getViewMatrix() const;
    inline bool getHasMoved() const { return hasMoved; }

    glm::vec3 eye;
    float theta, phi;
private:
    GLFWwindow* window;
    bool hasMoved = false;
};
