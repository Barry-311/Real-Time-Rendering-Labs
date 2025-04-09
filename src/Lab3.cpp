#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

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
unsigned int loadCubemap(const std::vector<std::string>& faces);

// Skybox vertex data
float skyboxVertices[] = {
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

unsigned int skyboxVAO, skyboxVBO;

int main()
{
    // Initialize variables
    float rotationAngle = 0.0f;
    float bumpHeightScale = 0.1f;

    // GLFW initialization
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lab 3 Mapping", nullptr, nullptr);
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
    Shader NormalShader("Shaders/mapping.vert", "Shaders/mapping.frag");
    Shader PhongShader("Shaders/phong.vert", "Shaders/phong.frag");
    Shader BumpShader("Shaders/bump.vert", "Shaders/bump.frag");
    Shader skyboxShader("Shaders/skybox.vert", "Shaders/skybox.frag");

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // Load model
    Model model1("Models/Dog/Corgi-2018-obj.obj");

    // Additional Textures
    unsigned int diffuseTexture = TextureFromFile("Korgi_diffuseOriginal.png", "Models/Dog/");
    unsigned int heightMapTexture = TextureFromFile("Korgi_height.png", "Models/Dog/");
    unsigned int normalTexture = TextureFromFile("Korgi_normal.png", "Models/Dog/");

    std::vector<std::string> faces
    {
        "Models/skybox/right.jpg",
        "Models/skybox/left.jpg",
        "Models/skybox/top.jpg",
        "Models/skybox/bottom.jpg",
        "Models/skybox/front.jpg",
        "Models/skybox/back.jpg"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.5f; 

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        processInput(window);

        // Clear buffers
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera and projection
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom()), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // Update rotation angle
        rotationAngle += 10.0f * deltaTime; // rotate 10 degrees per second
        if (rotationAngle >= 360.0f)
            rotationAngle -= 360.0f;

        // model1 - Phong Illumination
        glBindVertexArray(model1.meshes[0].VAO);
        PhongShader.use();
        PhongShader.setInt("modelID", 2);
        // Set shader uniforms
        PhongShader.setVec3("viewPos", camera.position());
        PhongShader.setVec3("lightPos", glm::vec3(5.0f, 5.0f, 0.0f)); // light position
        PhongShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f)); // white light
        PhongShader.setMat4("projection", projection);
        PhongShader.setMat4("view", view);
        glm::mat4 modelMatrix1 = glm::mat4(1.0f);
        modelMatrix1 = glm::translate(modelMatrix1, glm::vec3(-3.0f, 0.0f, 0.0f));
        modelMatrix1 = glm::rotate(modelMatrix1, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // around Y
        modelMatrix1 = glm::scale(modelMatrix1, glm::vec3(0.01f, 0.01f, 0.01f));
        PhongShader.setMat4("model", modelMatrix1);
        model1.Draw(PhongShader);

        // model2 - Normal Mapping
        glBindVertexArray(model1.meshes[0].VAO); 
        NormalShader.use();
        NormalShader.setMat4("projection", projection);
        NormalShader.setMat4("view", view);
        NormalShader.setVec3("viewPos", camera.position());
        NormalShader.setVec3("lightPos", glm::vec3(5.0f, 5.0f, 0.0f));
        NormalShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
        NormalShader.setInt("texture_diffuse", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalTexture);
        NormalShader.setInt("texture_normal1", 1);

        glm::mat4 modelMatrix2 = glm::mat4(1.0f);
        modelMatrix2 = glm::translate(modelMatrix2, glm::vec3(0.0f, 0.0f, 0.0f));
        modelMatrix2 = glm::rotate(modelMatrix2, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix2 = glm::scale(modelMatrix2, glm::vec3(0.01f, 0.01f, 0.01f));
        NormalShader.setMat4("model", modelMatrix2);
        model1.Draw(NormalShader);

        // model3 - Bump Mapping
        glBindVertexArray(model1.meshes[0].VAO);
        BumpShader.use();
        BumpShader.setMat4("projection", projection);
        BumpShader.setMat4("view", view);
        BumpShader.setVec3("viewPos", camera.position());
        BumpShader.setVec3("lightPos", glm::vec3(5.0f, 5.0f, 0.0f));
        BumpShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));

        BumpShader.setFloat("heightScale", bumpHeightScale);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
        BumpShader.setInt("texture_diffuse", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, heightMapTexture);
        BumpShader.setInt("texture_height", 1);

        glm::mat4 modelMatrix3 = glm::mat4(1.0f);
        modelMatrix3 = glm::translate(modelMatrix3, glm::vec3(3.0f, 0.0f, 0.0f));
        modelMatrix3 = glm::rotate(modelMatrix3, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        modelMatrix3 = glm::scale(modelMatrix3, glm::vec3(0.01f, 0.01f, 0.01f));
        BumpShader.setMat4("model", modelMatrix3);
        model1.Draw(BumpShader);

        // Rendering Skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        // Get 3x3 to remove displacement of the camera to make skybox still
        glm::mat4 viewSkybox = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", viewSkybox);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        // Depth
        glDepthFunc(GL_LESS);

        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Parameter Control");

            ImGui::SliderFloat("Height Scale", &bumpHeightScale, 0.0f, 3.0f, "%.3f");
            ImGui::Text("Height Scale for bump mapping: %.3f", bumpHeightScale);

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);

    glfwTerminate();
    return 0;
}

// Skybox loading
unsigned int loadCubemap(const std::vector<std::string>& faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cerr << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    // Set 6 faces
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
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
