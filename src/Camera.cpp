#include <Camera.h>

Camera::Camera(GLFWwindow* window) : window(window) {
    eye = glm::vec3(0.0f);
    phi = 0;
    theta = glm::pi<float>() / 2.0f;
}

void Camera::update(float dt) {
    hasMoved = false;

    const float moveSpeed = 3.0f * dt;
    const float rotSpeed = 2.0f * dt;
    const glm::vec3 viewDir = getViewDir();
    const glm::vec3 sideDir = cross(glm::vec3(0,1,0), viewDir);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { hasMoved = true, eye += moveSpeed * viewDir; }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { hasMoved = true, eye -= moveSpeed * viewDir; }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { hasMoved = true, eye += moveSpeed * sideDir; }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { hasMoved = true, eye -= moveSpeed * sideDir; }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) { hasMoved = true, theta -= rotSpeed; }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) { hasMoved = true, theta += rotSpeed; }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) { hasMoved = true, phi -= rotSpeed; }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) { hasMoved = true, phi += rotSpeed; }

}

glm::vec3 Camera::getViewDir() const {
    return glm::vec3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(eye, eye + getViewDir(), glm::vec3(0,1,0));
}
