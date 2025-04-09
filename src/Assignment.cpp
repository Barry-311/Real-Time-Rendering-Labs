#define _CRT_SECURE_NO_WARNINGS
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stb_image/stb_image.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "camera.hpp"
#include "grid.h"
#include "shader.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cmath>

// window
static const unsigned int SCR_WIDTH = 1980;
static const unsigned int SCR_HEIGHT = 1080;

// must match ifft_xxx.comp
static const unsigned int FFT_SIZE = 512;

Camera camera(glm::vec3(0.0f, 5.0f, 15.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;
int totalDispatches = 0;

// freq for u/v/h
static GLuint spectrumU0 = 0, spectrumU = 0, spectrumUTemp = 0;
static GLuint spectrumV0 = 0, spectrumV = 0, spectrumVTemp = 0;
static GLuint spectrumH0 = 0, spectrumH = 0, spectrumHTemp = 0;

// partial freq
static GLuint spectrumU_x = 0, spectrumU_y = 0;
static GLuint spectrumV_x = 0, spectrumV_y = 0;

// partial space
static GLuint partialU_x = 0, partialU_y = 0;
static GLuint partialV_x = 0, partialV_y = 0;

// space domain
static GLuint uTex = 0, vTex = 0, waveTex = 0, jacobianTex = 0;

// compute programs
static GLuint initSpectrumProg = 0;    // same .comp => use uMode=0,1,2
static GLuint updatePhaseProg = 0;     // same .comp => use uMode=0,1,2
static GLuint ifftRowProg = 0, ifftColProg = 0;
static GLuint derivUProg = 0, derivVProg = 0;
static GLuint whitecapJacobianProg = 0;

// for ImGui
static float windDirX = 1.0f;
static float windDirY = 1.0f;
static float foamStart = 1.001f;
static float foamEnd = 0.995f;
static float waveAmplify = 50.0;
static float chopAmplify = 10.0;
float domainSize = 200.0f;
float phillipsA = 0.1f;

// skybox
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

static GLuint skyboxVAO = 0, skyboxVBO = 0;

void framebuffer_size_callback(GLFWwindow*, int, int);
unsigned int loadCubemap(const std::vector<std::string>& faces);
GLuint LoadComputeShader(const char* csPath);
void RunComputeShader(GLuint prog, GLuint gx, GLuint gy);

static GLuint createRG32F(int w, int h)
{
    GLuint tex; glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, w, h, 0, GL_RG, GL_FLOAT, nullptr);

    /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);*/

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}
static GLuint createR32F(int w, int h)
{
    GLuint tex; glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, w, h, 0, GL_RED, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

// init GPU
static void initGPUResources()
{
    // freq
    spectrumU0 = createRG32F(FFT_SIZE, FFT_SIZE);
    spectrumU = createRG32F(FFT_SIZE, FFT_SIZE);
    spectrumUTemp = createRG32F(FFT_SIZE, FFT_SIZE);

    spectrumV0 = createRG32F(FFT_SIZE, FFT_SIZE);
    spectrumV = createRG32F(FFT_SIZE, FFT_SIZE);
    spectrumVTemp = createRG32F(FFT_SIZE, FFT_SIZE);

    spectrumH0 = createRG32F(FFT_SIZE, FFT_SIZE);
    spectrumH = createRG32F(FFT_SIZE, FFT_SIZE);
    spectrumHTemp = createRG32F(FFT_SIZE, FFT_SIZE);

    spectrumU_x = createRG32F(FFT_SIZE, FFT_SIZE);
    spectrumU_y = createRG32F(FFT_SIZE, FFT_SIZE);
    spectrumV_x = createRG32F(FFT_SIZE, FFT_SIZE);
    spectrumV_y = createRG32F(FFT_SIZE, FFT_SIZE);

    partialU_x = createR32F(FFT_SIZE, FFT_SIZE);
    partialU_y = createR32F(FFT_SIZE, FFT_SIZE);
    partialV_x = createR32F(FFT_SIZE, FFT_SIZE);
    partialV_y = createR32F(FFT_SIZE, FFT_SIZE);

    uTex = createR32F(FFT_SIZE, FFT_SIZE);
    vTex = createR32F(FFT_SIZE, FFT_SIZE);
    waveTex = createR32F(FFT_SIZE, FFT_SIZE);
    jacobianTex = createR32F(FFT_SIZE, FFT_SIZE);
}

static void initComputePrograms()
{
    // init_spectrum.comp => single file => use uMode=0 =>u,1=>v,2=>h
    initSpectrumProg = LoadComputeShader("ShadersCompute/init_spectrum.comp");
    // update_phase.comp => single file => use uMode=0 =>u,1=>v,2=>h
    updatePhaseProg = LoadComputeShader("ShadersCompute/update_phase.comp");

    ifftRowProg = LoadComputeShader("ShadersCompute/ifft_row.comp");
    ifftColProg = LoadComputeShader("ShadersCompute/ifft_col.comp");

    derivUProg = LoadComputeShader("ShadersCompute/derivative_pre_u.comp");
    derivVProg = LoadComputeShader("ShadersCompute/derivative_pre_v.comp");

    whitecapJacobianProg = LoadComputeShader("ShadersCompute/whitecap_jacobian_space.comp");
}

static void createInitialSpectrum()
{
    // 1) init (u)
    glUseProgram(initSpectrumProg);

    glUniform1f(glGetUniformLocation(initSpectrumProg, "uPhillipsA"), phillipsA);
    glUniform1f(glGetUniformLocation(initSpectrumProg, "uDomainSize"), domainSize);
    glUniform2f(glGetUniformLocation(initSpectrumProg, "uWindDir"), windDirX, windDirY);
    glUniform1i(glGetUniformLocation(initSpectrumProg, "uMode"), 0); // => (u)
    glBindImageTexture(0, spectrumU0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(initSpectrumProg, (FFT_SIZE + 15) / 16, (FFT_SIZE + 15) / 16);

    // 2) init (v)
    glUseProgram(initSpectrumProg);
    glUniform1i(glGetUniformLocation(initSpectrumProg, "uMode"), 1); // => (v)
    glBindImageTexture(0, spectrumV0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(initSpectrumProg, (FFT_SIZE + 15) / 16, (FFT_SIZE + 15) / 16);

    // 3) init (h)
    glUseProgram(initSpectrumProg);
    glUniform1i(glGetUniformLocation(initSpectrumProg, "uMode"), 2); // => (h)
    glBindImageTexture(0, spectrumH0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(initSpectrumProg, (FFT_SIZE + 15) / 16, (FFT_SIZE + 15) / 16);

    // for debug
    std::vector<glm::vec2> debugSpec(FFT_SIZE * FFT_SIZE);
    glBindTexture(GL_TEXTURE_2D, spectrumH0);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, debugSpec.data());

    float avgH = 0.0f;
    for (int i = 0; i < FFT_SIZE * FFT_SIZE; ++i) {
        avgH += glm::length(debugSpec[i]);
    }
    avgH /= (FFT_SIZE * FFT_SIZE);
    std::cout << "[debug] spectrumH0 avg = " << avgH << std::endl;
}


static void updateWaveAndJacobian(float time)
{
    // A) update_phase => (u), (v), (h)
    //   1) (u)
    glUseProgram(updatePhaseProg);
    glUniform1f(glGetUniformLocation(updatePhaseProg, "uTime"), time);
    glUniform1f(glGetUniformLocation(updatePhaseProg, "uGravity"), 9.81f);
    glUniform1f(glGetUniformLocation(updatePhaseProg, "uDomainSize"), domainSize);

    glUniform1i(glGetUniformLocation(updatePhaseProg, "uMode"), 0); // => (u)
    glBindImageTexture(0, spectrumU0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumU, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(updatePhaseProg, (FFT_SIZE + 15) / 16, (FFT_SIZE + 15) / 16);

    //   2) (v)
    glUseProgram(updatePhaseProg);
    glUniform1f(glGetUniformLocation(updatePhaseProg, "uTime"), time);
    glUniform1f(glGetUniformLocation(updatePhaseProg, "uGravity"), 9.81f);
    glUniform1f(glGetUniformLocation(updatePhaseProg, "uDomainSize"), domainSize);

    glUniform1i(glGetUniformLocation(updatePhaseProg, "uMode"), 1);
    glBindImageTexture(0, spectrumV0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumV, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(updatePhaseProg, (FFT_SIZE + 15) / 16, (FFT_SIZE + 15) / 16);

    //   3) (h)
    glUseProgram(updatePhaseProg);
    glUniform1i(glGetUniformLocation(updatePhaseProg, "uMode"), 2);
    glBindImageTexture(0, spectrumH0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumH, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(updatePhaseProg, (FFT_SIZE + 15) / 16, (FFT_SIZE + 15) / 16);

    // B) derivative (spectrumU_x, spectrumU_y)
    glUseProgram(derivUProg);
    glUniform1f(glGetUniformLocation(derivUProg, "domainSize"), domainSize);
    glBindImageTexture(0, spectrumU, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumU_x, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    glBindImageTexture(2, spectrumU_y, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(derivUProg, (FFT_SIZE + 15) / 16, (FFT_SIZE + 15) / 16);

    // derivative (spectrumV_x, spectrumV_y)
    glUseProgram(derivVProg);
    glUniform1f(glGetUniformLocation(derivVProg, "domainSize"), domainSize);
    glBindImageTexture(0, spectrumV, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumV_x, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    glBindImageTexture(2, spectrumV_y, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(derivVProg, (FFT_SIZE + 15) / 16, (FFT_SIZE + 15) / 16);

    // C) iFFT => (uTex)
    glUseProgram(ifftRowProg);
    glBindImageTexture(0, spectrumU, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumUTemp, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    glUniform1ui(glGetUniformLocation(ifftRowProg, "uInverse"), 1u);
    RunComputeShader(ifftRowProg, 1, FFT_SIZE);

    glUseProgram(ifftColProg);
    glBindImageTexture(0, spectrumUTemp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, uTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
    glUniform1ui(glGetUniformLocation(ifftColProg, "uInverse"), 1u);
    RunComputeShader(ifftColProg, FFT_SIZE, 1);

    // iFFT => (vTex)
    glUseProgram(ifftRowProg);
    glBindImageTexture(0, spectrumV, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumVTemp, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    glUniform1ui(glGetUniformLocation(ifftRowProg, "uInverse"), 1u);
    RunComputeShader(ifftRowProg, 1, FFT_SIZE);

    glUseProgram(ifftColProg);
    glBindImageTexture(0, spectrumVTemp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, vTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
    glUniform1ui(glGetUniformLocation(ifftColProg, "uInverse"), 1u);
    RunComputeShader(ifftColProg, FFT_SIZE, 1);

    // D) iFFT => waveTex (h)
    glUseProgram(ifftRowProg);
    glBindImageTexture(0, spectrumH, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumHTemp, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(ifftRowProg, 1, FFT_SIZE);

    glUseProgram(ifftColProg);
    glBindImageTexture(0, spectrumHTemp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, waveTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
    RunComputeShader(ifftColProg, FFT_SIZE, 1);

    // E) iFFT => partialU_x, partialU_y
    //    对 spectrumU_x / spectrumU_y -> iFFT =>  dU/dx, dU/dy
    //    1) spectrumU_x -> row -> spectrumUTemp -> col -> partialU_x
    glUseProgram(ifftRowProg);
    glBindImageTexture(0, spectrumU_x, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumUTemp, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    glUniform1ui(glGetUniformLocation(ifftRowProg, "uInverse"), 1u);
    RunComputeShader(ifftRowProg, 1, FFT_SIZE);

    glUseProgram(ifftColProg);
    glBindImageTexture(0, spectrumUTemp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, partialU_x, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
    glUniform1ui(glGetUniformLocation(ifftColProg, "uInverse"), 1u);
    RunComputeShader(ifftColProg, FFT_SIZE, 1);

    //    2) spectrumU_y -> row -> spectrumUTemp -> col -> partialU_y
    glUseProgram(ifftRowProg);
    glBindImageTexture(0, spectrumU_y, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumUTemp, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(ifftRowProg, 1, FFT_SIZE);

    glUseProgram(ifftColProg);
    glBindImageTexture(0, spectrumUTemp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, partialU_y, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
    RunComputeShader(ifftColProg, FFT_SIZE, 1);

    // F) iFFT => partialV_x, partialV_y
    //    1) spectrumV_x -> row -> spectrumVTemp -> col -> partialV_x
    glUseProgram(ifftRowProg);
    glBindImageTexture(0, spectrumV_x, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumVTemp, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(ifftRowProg, 1, FFT_SIZE);

    glUseProgram(ifftColProg);
    glBindImageTexture(0, spectrumVTemp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, partialV_x, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
    RunComputeShader(ifftColProg, FFT_SIZE, 1);

    //    2) spectrumV_y -> row -> spectrumVTemp -> col -> partialV_y
    glUseProgram(ifftRowProg);
    glBindImageTexture(0, spectrumV_y, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, spectrumVTemp, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);
    RunComputeShader(ifftRowProg, 1, FFT_SIZE);

    glUseProgram(ifftColProg);
    glBindImageTexture(0, spectrumVTemp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG32F);
    glBindImageTexture(1, partialV_y, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
    RunComputeShader(ifftColProg, FFT_SIZE, 1);

    // G) whitecap_jacobian_space => combine partialU_x, partialU_y, partialV_x, partialV_y => jacobianTex
    glUseProgram(whitecapJacobianProg);
    glBindImageTexture(0, partialU_x, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    glBindImageTexture(1, partialU_y, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    glBindImageTexture(2, partialV_x, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    glBindImageTexture(3, partialV_y, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
    glBindImageTexture(4, jacobianTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

    RunComputeShader(whitecapJacobianProg, (FFT_SIZE + 15) / 16, (FFT_SIZE + 15) / 16);

    // DEBUG OUTPUT SECTION
    std::vector<glm::vec2> debugTemp(FFT_SIZE * FFT_SIZE);
    glBindTexture(GL_TEXTURE_2D, spectrumHTemp);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, debugTemp.data());

    float avgTemp = 0.0f;
    for (int i = 0; i < FFT_SIZE * FFT_SIZE; ++i)
        avgTemp += glm::length(debugTemp[i]);
    avgTemp /= (FFT_SIZE * FFT_SIZE);
    std::cout << "[debug] spectrumHTemp avg = " << avgTemp << std::endl;

    std::vector<glm::vec2> debugU(FFT_SIZE * FFT_SIZE);
    glBindTexture(GL_TEXTURE_2D, spectrumU);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, debugU.data());

    float sumU = 0.0f;
    for (int i = 0; i < FFT_SIZE * FFT_SIZE; ++i)
        sumU += glm::length(debugU[i]);

    std::cout << "[debug] spectrumU avg = " << sumU / (FFT_SIZE * FFT_SIZE) << std::endl;

    std::vector<glm::vec2> debugUTemp(FFT_SIZE * FFT_SIZE);
    glBindTexture(GL_TEXTURE_2D, spectrumUTemp);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, debugUTemp.data());

    float sumUTemp = 0.0f;
    for (int i = 0; i < FFT_SIZE * FFT_SIZE; ++i)
        sumUTemp += glm::length(debugUTemp[i]);

    std::cout << "[debug] spectrumUTemp avg = " << sumUTemp / (FFT_SIZE * FFT_SIZE) << std::endl;

    std::vector<glm::vec2> debugUTex(FFT_SIZE * FFT_SIZE);
    glBindTexture(GL_TEXTURE_2D, uTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, debugUTex.data());

    float sumUTex = 0.0f;
    for (int i = 0; i < FFT_SIZE * FFT_SIZE; ++i)
        sumUTex += glm::length(debugUTex[i]);

    std::cout << "[debug] uTex avg = " << sumUTex / (FFT_SIZE * FFT_SIZE) << std::endl;

    std::vector<glm::vec2> debugDeriv(FFT_SIZE * FFT_SIZE);
    glBindTexture(GL_TEXTURE_2D, spectrumU_x);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, debugDeriv.data());

    float sumDeriv = 0.0f;
    for (int i = 0; i < FFT_SIZE * FFT_SIZE; ++i)
        sumDeriv += glm::length(debugDeriv[i]);

    std::cout << "[debug] spectrumU_x avg = " << sumDeriv / (FFT_SIZE * FFT_SIZE) << std::endl;

    std::vector<float> debugJacobian(FFT_SIZE * FFT_SIZE);
    glBindTexture(GL_TEXTURE_2D, jacobianTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, debugJacobian.data());

    float jMin = 1e9, jMax = -1e9;
    for (float val : debugJacobian) {
        if (val < jMin) jMin = val;
        if (val > jMax) jMax = val;
    }
    std::cout << "[debug] Jacobian min: " << jMin << ", max: " << jMax << std::endl;

    std::vector<glm::vec2> debugUResult(FFT_SIZE * FFT_SIZE);
    glBindTexture(GL_TEXTURE_2D, spectrumUTemp);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, debugUResult.data());

    float avgU = 0.0f;
    for (auto& v : debugUResult) avgU += glm::length(v);
    std::cout << "[debug] spectrumUTemp(iFFT u) avg = " << avgU / (FFT_SIZE * FFT_SIZE) << std::endl;

    std::vector<float> debugWave(FFT_SIZE * FFT_SIZE);
    glBindTexture(GL_TEXTURE_2D, waveTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, debugWave.data());

    float minH = 1e9f, maxH = -1e9f;
    for (float h : debugWave) {
        if (h < minH) minH = h;
        if (h > maxH) maxH = h;
    }
    std::cout << "[debug] waveTex height: min = " << minH << ", max = " << maxH << std::endl;

    std::vector<float> debugUField(FFT_SIZE * FFT_SIZE);
    glBindTexture(GL_TEXTURE_2D, uTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, debugUField.data());

    float minU = 1e9f, maxU = -1e9f;
    for (float val : debugUField) {
        if (val < minU) minU = val;
        if (val > maxU) maxU = val;
    }
    std::cout << "[debug] uTex displacement: min = " << minU << ", max = " << maxU << std::endl;

}

unsigned int loadCubemap(const std::vector<std::string>& faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int w, h, nc;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &w, &h, &nc, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cerr << "Failed load cubemap at " << faces[i] << "\n";
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

int main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Choppy Whitecap Ocean", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW!\n";
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glEnable(GL_DEPTH_TEST);

    // init compute
    initGPUResources();
    initComputePrograms();
    createInitialSpectrum();

    // projected grid
    ProjectedGrid oceanGrid(512);

    // ocean shader
    Shader oceanShader("ShadersAssignment/grid.vert", "ShadersAssignment/grid.frag");

    // skybox
    Shader skyboxShader("Shaders/skybox.vert", "Shaders/skybox.frag");
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    std::vector<std::string> faces{
        "Models/skybox/cloud_LF.tga",
        "Models/skybox/cloud_RT.tga",
        "Models/skybox/cloud_UP.tga",
        "Models/skybox/cloud_DN.tga",
        "Models/skybox/cloud_FR.tga",
        "Models/skybox/cloud_BK.tga"
    };
    unsigned int cubemapTex = loadCubemap(faces);

    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 1.2f;

    float fps = 1.0f / deltaTime;

    // main loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        static double lastComputeTime = 0;
        double start = glfwGetTime();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // compute => updateWaveAndJacobian
        updateWaveAndJacobian(currentFrame);

        double end = glfwGetTime();
        double elapsed = (end - start) * 1000.0; // in ms

        glClearColor(0.2f, 0.2f, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 10.0f, 30.0f),
            glm::vec3(0.0f, 5.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f,
            500.0f);

        glm::mat4 invVP = glm::inverse(projection * view);

        // ocean
        oceanShader.use();
        oceanShader.setMat4("view", view);
        oceanShader.setMat4("projection", projection);
        oceanShader.setMat4("inverseVP", invVP);
        oceanShader.setVec3("cameraPos", camera.position());

        oceanShader.setFloat("foamStart", foamStart);
        oceanShader.setFloat("foamEnd", foamEnd);
        oceanShader.setFloat("chopAmplify", chopAmplify);
        oceanShader.setFloat("waveAmplify", waveAmplify);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, waveTex);
        oceanShader.setInt("uWaveTex", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, uTex);
        oceanShader.setInt("uTex", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, vTex);
        oceanShader.setInt("vTex", 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, jacobianTex);
        oceanShader.setInt("uJacobianTex", 3);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
        oceanShader.setInt("uSkybox", 4);

        oceanGrid.Draw();

        // skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        glm::mat4 viewSkybox = glm::mat4(glm::mat3(view));
        skyboxShader.setMat4("view", viewSkybox);
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        // ImGui
        {
            ImGui::SetNextWindowSize(ImVec2(800, 400));
            ImGui::Begin("Parameters Control");

            ImGui::Text("=== Simulation ===");
            ImGui::SliderFloat("Wind Dir X", &windDirX, -1.0f, 1.0f);
            ImGui::SliderFloat("Wind Dir Y", &windDirY, -1.0f, 1.0f);

            ImGui::SliderFloat("Chop Amplify", &chopAmplify, 0.0f, 20.0f);
            ImGui::SliderFloat("Wave Amplify", &waveAmplify, 0.0f, 300.0f);

            ImGui::SliderFloat("Phillips A", &phillipsA, 0.01f, 1.0f);
            ImGui::SliderFloat("Domain Size", &domainSize, 50.0f, 1000.0f);

            ImGui::Text("=== Whitecap ===");
            ImGui::SliderFloat("Foam Start", &foamStart, 0.999f, 1.01f);
            ImGui::SliderFloat("Foam End", &foamEnd, 0.99f, 0.998f);

            ImGui::Text("=== Performance ===");
            ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
            ImGui::Text("FFT Size: %d x %d", FFT_SIZE, FFT_SIZE);
            ImGui::Text("Frame Time: %.2f ms", 1000.0f * deltaTime);
            ImGui::Text("Compute Dispatches: %d", totalDispatches);

            if (ImGui::Button("Re-generate Initial Spectrum")) {
                createInitialSpectrum();
            }

            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* w, int width, int height)
{
    glViewport(0, 0, width, height);
}

GLuint LoadComputeShader(const char* path)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        std::cerr << "Cannot open " << path << "\n";
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::vector<char> buf(sz + 1);
    fread(buf.data(), 1, sz, fp);
    buf[sz] = '\0';
    fclose(fp);

    std::string shaderCode = std::string(buf.data());

    // 找到 #version 行
    size_t versionPos = shaderCode.find("#version");
    if (versionPos == std::string::npos) {
        std::cerr << "Shader missing #version directive: " << path << "\n";
        return 0;
    }

    size_t insertPos = shaderCode.find('\n', versionPos);
    std::string fullShaderCode = shaderCode.substr(0, insertPos + 1) +
        "#define N " + std::to_string(FFT_SIZE) + "\n" +
        shaderCode.substr(insertPos + 1);  // 插入 define 在 version 后

    const char* src = fullShaderCode.c_str();
    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs, 1, &src, nullptr);
    glCompileShader(cs);

    GLint success;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetShaderInfoLog(cs, 1024, nullptr, info);
        std::cerr << "Compute Shader compile error:\n" << info << "\n";
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, cs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        char info[1024];
        glGetProgramInfoLog(prog, 1024, nullptr, info);
        std::cerr << "Program link error:\n" << info << "\n";
        return 0;
    }
    glDetachShader(prog, cs);
    glDeleteShader(cs);
    return prog;
}


void RunComputeShader(GLuint prog, GLuint gx, GLuint gy)
{
    glUseProgram(prog);
    glDispatchCompute(gx, gy, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    totalDispatches++;
}
