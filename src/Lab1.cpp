#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "camera.hpp"
#include "model.hpp"

#include <iostream>

//// Window Parameters ////
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
const unsigned int SCR_WIDTH = 1800;
const unsigned int SCR_HEIGHT = 1000;

Camera camera(glm::vec3(0.0f, 0.0f, 4.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // Initialize variables
    float rotationAngle1 = 0.0f;
    float rotationAngle2 = 0.0f;
    float rotationAngle3 = 0.0f;

    // GLFW initialization
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Phong Illumination Lab", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // GLEW initialization
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Shader program
    Shader PhongShader("Shaders/phong.vert", "Shaders/phong.frag");

    // Load model
    Model model1("Models/Teapot/Teapot_Square_White.obj");
    Model model2("Models/Dog/Corgi-2018-obj.obj");

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Clear buffers
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera and projection
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom()), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // Set shader uniforms
        PhongShader.setVec3("viewPos", camera.position());
        PhongShader.setVec3("lightPos", glm::vec3(5.0f, 5.0f, 0.0f)); // light position
        PhongShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f)); // white light

        PhongShader.setMat4("projection", projection);
        PhongShader.setMat4("view", view);

        // Update rotation angle
        rotationAngle1 += 10.0f * deltaTime; // rotate 10 degrees per second
        if (rotationAngle1 >= 360.0f) 
            rotationAngle1 -= 360.0f;

        rotationAngle2 += 12.0f * deltaTime; // rotate 10 degrees per second
        if (rotationAngle1 >= 360.0f)
            rotationAngle1 -= 360.0f;

        rotationAngle3 += 8.0f * deltaTime; // rotate 10 degrees per second
        if (rotationAngle1 >= 360.0f)
            rotationAngle1 -= 360.0f;

        // model1
        PhongShader.use();
        PhongShader.setInt("modelID", 2);
        glm::mat4 modelMatrix1 = glm::mat4(1.0f);
        modelMatrix1 = glm::translate(modelMatrix1, glm::vec3(0.0f, 0.0f, 0.0f));
        modelMatrix1 = glm::rotate(modelMatrix1, glm::radians(rotationAngle1), glm::vec3(0.0f, 1.0f, 0.0f)); // around Y
        modelMatrix1 = glm::scale(modelMatrix1, glm::vec3(2.0f, 2.0f, 2.0f));
        PhongShader.setMat4("model", modelMatrix1);
        model1.Draw(PhongShader);

        // model2
        PhongShader.use();
        PhongShader.setInt("modelID", 1);
        glm::mat4 modelMatrix2 = glm::mat4(1.0f);
        modelMatrix2 = glm::translate(modelMatrix2, glm::vec3(-1.0f, 0.0f, 0.0f)); 
        modelMatrix2 = glm::rotate(modelMatrix2, glm::radians(rotationAngle2), glm::vec3(0.0f, 1.0f, 0.0f)); 
        modelMatrix2 = glm::scale(modelMatrix2, glm::vec3(2.0f, 2.0f, 2.0f));
        PhongShader.setMat4("model", modelMatrix2);
        model1.Draw(PhongShader);

        // model3 - Toon
        PhongShader.use();
        PhongShader.setInt("modelID", 3);
        glm::mat4 modelMatrix3 = glm::mat4(1.0f);
        modelMatrix3 = glm::translate(modelMatrix3, glm::vec3(1.0f, 0.0f, 0.0f));
        modelMatrix3 = glm::rotate(modelMatrix3, glm::radians(rotationAngle3), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix3 = glm::scale(modelMatrix3, glm::vec3(2.0f, 2.0f, 2.0f));
        PhongShader.setMat4("model", modelMatrix3);
        model1.Draw(PhongShader);

        // model4 - Oren-Nayar
        PhongShader.use();
        PhongShader.setInt("modelID", 4);
        glm::mat4 modelMatrix4 = glm::mat4(1.0f);
        modelMatrix4 = glm::translate(modelMatrix4, glm::vec3(-1.0f, -1.0f, 0.0f));
        modelMatrix4 = glm::rotate(modelMatrix4, glm::radians(rotationAngle2), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix4 = glm::scale(modelMatrix4, glm::vec3(2.0f, 2.0f, 2.0f));
        PhongShader.setMat4("model", modelMatrix4);
        model1.Draw(PhongShader);

        // model5 - Cook-Torrance
        PhongShader.use();
        PhongShader.setInt("modelID", 5);
        glm::mat4 modelMatrix5 = glm::mat4(1.0f);
        modelMatrix5 = glm::translate(modelMatrix5, glm::vec3(0.0f, -1.0f, 0.0f));
        modelMatrix5 = glm::rotate(modelMatrix5, glm::radians(rotationAngle1), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix5 = glm::scale(modelMatrix5, glm::vec3(2.0f, 2.0f, 2.0f));
        PhongShader.setMat4("model", modelMatrix5);
        model1.Draw(PhongShader);

        // model6 - Minnaert
        PhongShader.use();
        PhongShader.setInt("modelID", 6);
        glm::mat4 modelMatrix6 = glm::mat4(1.0f);
        modelMatrix6 = glm::translate(modelMatrix6, glm::vec3(1.0f, -1.0f, 0.0f));
        modelMatrix6 = glm::rotate(modelMatrix6, glm::radians(rotationAngle3), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix6 = glm::scale(modelMatrix6, glm::vec3(2.0f, 2.0f, 2.0f));
        PhongShader.setMat4("model", modelMatrix6);
        model1.Draw(PhongShader);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}


void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // WASD - camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(kForward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(kBackward, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(kLeft, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(kRight, deltaTime);

    // O - camera up; K - camera down
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(kUp, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(kDown, deltaTime);

    // Arrow Keys - Camera rotation
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.ProcessSpecialInput(GLFW_KEY_UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.ProcessSpecialInput(GLFW_KEY_DOWN, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.ProcessSpecialInput(GLFW_KEY_LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.ProcessSpecialInput(GLFW_KEY_RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
