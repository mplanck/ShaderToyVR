//
//  ShaderToyVR - simple wrapper around the ShaderToy concept but with the 
//  ability to interface with the Oculus Rift.  ShaderToy must implement
//  iCameraTransform.
//
//  Created by Maxwell Planck on 9/3/14.
//  Copyright (c) 2012 Maxwell Planck. All rights reserved.
//

#define NOMINMAX
#define _USE_MATH_DEFINES 1

#define GLEW_BUILD GLEW_STATIC
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL


#ifdef _DEBUG
#pragma comment(lib, "libovrd.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "opengl32.lib")
#else
#pragma comment(lib, "libovr.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "opengl32.lib")
#endif

#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "winmm.lib")

// SYSTEM DEPENDENCIES

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

// INTERNAL DEPENDENCIES

#include "HBGLStats.h"
#include "HBGLShaders.h"
#include "STVRShaders.h"
#include "HBGLUtils.h"
#include "HBGLResourceWrappers.h"

// OUTSIDE DEPENDENCIES

#include "OVR.h"
#include "OVR_CAPI_GL.h"
#include "SOIL.h"

#include "glm/glm.hpp"
#include "glm/gtc/noise.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/string_cast.hpp"

using namespace OVR;
using namespace HBGLUtils;

// ========================================================================
// GLOBAL DECLARES AND CONSTANTS
// ========================================================================

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

const bool c_DebugMode = true;
const bool c_DebugModeRelink = false;

const bool c_DebugWindowed = false;

const int c_SphGridNumLatSpans = 32;
const int c_SphGridNumLongSpans = 32;

const int c_DefaultWindowWidth = 1920;
const int c_DefaultWindowHeight = 1080;

const GLuint c_ChannelTextures[4] = { GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3 };

// ========================================================================
// GLOBAL STATIC <- not good in large systems but fine in isolation
// ========================================================================

static GLFWwindow* g_GLFWWindow;

static uint                           g_FrameNumber = 0;
static float                          g_TimebaseInSecs = 0.0f;
static float                          g_FramesPerSecond = 0.0f;

static float                          g_PlaybackTimeInSecs = 0.0f;
static float                          g_PlaybackResetInSecs = 0.0f;
static bool                           g_Playing = true;

static bool                           g_DisplaySphereGrid = false;
static bool                           g_DisplayOverlay = false;
static bool                           g_DisplayPositionalCam = false;

static HBGLBufferResourcePtr          g_SphereGridVertexVBOID;
static HBGLBufferResourcePtr          g_SphereGridIndexIBOID;
static uint                           g_NumSphereGridIndices;
static HBGLBufferResourcePtr          g_ScreenQuadVertexVBOID;
static HBGLShaderProgramPtr           g_SphereGridShaderProgram;
static HBGLShaderProgramPtr           g_ScreenQuadShaderProgram;

static HBGLOverlayStatsPtr            g_OverlayStats;

static HBGLTextureResourcePtr         g_ChannelTextures[4];
static GLfloat                        g_ChannelTimes[4] = { 0.f, 0.f, 0.f, 0.f };
static GLfloat                        g_ChannelResolutions[4][3] = { { 0.f, 0.f, 0.f },
                                                                     { 0.f, 0.f, 0.f },
                                                                     { 0.f, 0.f, 0.f },
                                                                     { 0.f, 0.f, 0.f } };
static glm::vec4                      g_Date;

static ovrHmd		                  g_HMD;
static ovrGLTexture                   g_EyeTextures[2];
static float                          g_ScreenPercentage = 1.f;
static float                          g_FocalLengthScalar = 1.f;

// OVR g_ state
static HBGLFrameBufferResourcePtr	  g_OVRFrameBuffer[2];
static HBGLTextureResourcePtr         g_OVRColorTexture[2];
static HBGLRenderBufferResourcePtr    g_OVRDepthTexture[2];
static GLsizei                        g_OVRTextureSize[2][2];
static glm::mat4                      g_OVRCamPerspective[2];
static glm::vec3                      g_OVRCamOffset[2];
static bool                           g_OVRStereoView;
static glm::mat4                      g_OVRCameraTransform[2];
static glm::mat4                      g_OVRPositionalCamXform(1);
static float                          g_OVRPositionalCamTanHalfFov[2];
static glm::mat4                      g_WalkPosition;

// ========================================================================
// FORWARD DECLARES
// ========================================================================

void ShaderToyVRResetOVRPosition();
void ShaderToyVRErrorAndQuit();

// ========================================================================
// MATH UTILITIES
// ========================================================================

inline glm::mat4
FromOvrMatToMat(const ovrMatrix4f& omat4)
{
    // ovrMatrix is row major while glm is column major - bleck!
    return glm::transpose(glm::make_mat4(&omat4.M[0][0]));
}

inline glm::vec3
FromOvrVecToVec(const ovrVector3f& ovec3)
{
    return glm::make_vec3(&ovec3.x);
}

inline glm::quat
FromOvrQuatToQuat(const ovrQuatf& oquat) 
{
    return glm::make_quat(&oquat.x);
}

inline glm::mat4
FromOvrVecToMat(const ovrVector3f & ovec3) 
{
    return glm::translate(glm::mat4(), FromOvrVecToVec(ovec3));
}

inline glm::mat4 
FromOvrPoseToMat(const ovrPosef & opose) 
{
    glm::mat4 orientation = glm::mat4_cast(FromOvrQuatToQuat(opose.Orientation));
    glm::mat4 translation = FromOvrVecToMat(opose.Position);

    return translation * orientation; // * P
}

namespace std
{
    inline float 
        clamp(float v, float bottom, float top) {
            return std::max(std::min(v, top), bottom);
        }
}

// ========================================================================
// GL INIT
// ========================================================================

void
ShaderToyVRInitShaderSystem()
{
    // -------------------------------------------------

    {

        HBGLShaderProgram* shprog = new HBGLShaderProgram("ShaderToyVR Sphere Grid Shader Program");
        g_SphereGridShaderProgram = HBGLShaderProgramPtr(shprog); // should flush any existing reference in the construction

        STVRDebugGridVertexShader* vshader = new STVRDebugGridVertexShader();
        HBGLShaderPtr stvrDebugGridVertShader = HBGLShaderPtr(vshader);

        STVRDebugGridFragmentShader* fshader = new STVRDebugGridFragmentShader();
        HBGLShaderPtr stvrDebugGridFragShader = HBGLShaderPtr(fshader);

        g_SphereGridShaderProgram->LoadAndCompileShaders(stvrDebugGridVertShader, stvrDebugGridFragShader);

        GLint reservedIndex;
        g_SphereGridShaderProgram->ReserveAttribLocation("position", &reservedIndex);
        g_SphereGridShaderProgram->ReserveAttribLocation("texcoord", &reservedIndex);
        g_SphereGridShaderProgram->LinkShaders();
    }

    // -------------------------------------------------

    {

        // TODO: allow for reloading of the shader

        HBGLShaderProgram* shprog = new HBGLShaderProgram("ShaderToyVR Screen Quad Shader Program");
        g_ScreenQuadShaderProgram = HBGLShaderProgramPtr(shprog); // should flush any existing reference in the construction

        STVRVertexShader* vshader = new STVRVertexShader();
        HBGLShaderPtr stvrVertShader = HBGLShaderPtr(vshader);

        // TODO: make file searching better!
        STVRFragmentShader* fshader = new STVRFragmentShader("../glshaders/shadertoy.fs");
        HBGLShaderPtr stvrFragShader = HBGLShaderPtr(fshader);

        if (!g_ScreenQuadShaderProgram->LoadAndCompileShaders(stvrVertShader, stvrFragShader))
        {
            std::cerr << "Aborting since there was no valid shadertoy shader." << std::endl;
            ShaderToyVRErrorAndQuit();
        }

        GLint reservedIndex;
        g_ScreenQuadShaderProgram->ReserveAttribLocation("position", &reservedIndex);
        g_ScreenQuadShaderProgram->ReserveAttribLocation("texcoord", &reservedIndex);
        g_ScreenQuadShaderProgram->LinkShaders();
    }   

    // -------------------------------------------------

}

void
ShaderToyVRGenScreenQuadBuffers()
{
    HBGLBufferResource* vbobr = new HBGLBufferResource();
    g_ScreenQuadVertexVBOID = HBGLBufferResourcePtr(vbobr);
    g_ScreenQuadVertexVBOID->Generate();
    glBindBuffer(GL_ARRAY_BUFFER, g_ScreenQuadVertexVBOID->GetIndex());

    GLfloat  quadVertices[6][4] =
    {
        { -1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, -1.0f, 1.0f, 0.0f },

        { 1.0f, -1.0f, 1.0f, 0.0f },
        { -1.0f, -1.0f, 0.0f, 0.0f },
        { -1.0f, 1.0f, 0.0f, 1.0f }
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* 24,
        &quadVertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

}

void
ShaderToyVRGenSphereGridBuffers()
{
    GLfloat grid_vertices[c_SphGridNumLatSpans * (c_SphGridNumLongSpans + 1)][5];

    float sphere_radius = .5f;
    glm::vec3 sphere_pos(0.f, 0.f, 0.f);

    float pole_offset = .01f;

    float lat_delta = ((float)M_PI - 2.f * pole_offset) / (float)(c_SphGridNumLatSpans - 1);
    float long_delta = 2.f * (float)M_PI / (float)(c_SphGridNumLongSpans);

    float lat_angle = -(float)M_PI * .5f + pole_offset;

    for (uint lat_idx = 0; lat_idx < c_SphGridNumLatSpans; lat_idx++)
    {
        float long_angle = 0.;

        for (uint long_idx = 0; long_idx < c_SphGridNumLongSpans; long_idx++)
        {
            // grid points
            grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + long_idx)][0] = sphere_radius * cos(lat_angle) * cos(long_angle) + sphere_pos.x;
            grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + long_idx)][1] = sphere_radius * sin(lat_angle) + sphere_pos.y;
            grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + long_idx)][2] = sphere_radius * cos(lat_angle) * sin(long_angle) + sphere_pos.z;

            // grid uvs
            grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + long_idx)][3] = (float(lat_idx) / (c_SphGridNumLatSpans - 1));
            grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + long_idx)][4] = (float(long_idx) / (c_SphGridNumLongSpans));

            long_angle += long_delta;
        }

        // Repeat the first longitudinal coordinate at the end of this span to close the isoparm
        grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + c_SphGridNumLongSpans)][0] = grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + 0)][0];
        grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + c_SphGridNumLongSpans)][1] = grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + 0)][1];
        grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + c_SphGridNumLongSpans)][2] = grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + 0)][2];
        grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + c_SphGridNumLongSpans)][3] = grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + 0)][3];
        grid_vertices[((c_SphGridNumLongSpans + 1)*lat_idx + c_SphGridNumLongSpans)][4] = 1.;

        lat_angle += lat_delta;
    }

    // create the index buffer for a bunch of overlapping quads
    uint point_indices[c_SphGridNumLongSpans * (c_SphGridNumLatSpans - 1) * 4];
    g_NumSphereGridIndices = c_SphGridNumLongSpans * (c_SphGridNumLatSpans - 1) * 4;

    uint offset = 0;
    for (uint lat_idx = 0; lat_idx < c_SphGridNumLatSpans - 1; lat_idx++)
    {
        for (uint long_idx = 0; long_idx < c_SphGridNumLongSpans; long_idx++)
        {
            point_indices[offset++] = lat_idx * (c_SphGridNumLongSpans + 1) + long_idx;
            point_indices[offset++] = (lat_idx + 1) * (c_SphGridNumLongSpans + 1) + long_idx;
            point_indices[offset++] = (lat_idx + 1) * (c_SphGridNumLongSpans + 1) + long_idx + 1;
            point_indices[offset++] = lat_idx * (c_SphGridNumLongSpans + 1) + long_idx + 1;
        }
    }

    HBGLBufferResource* vbobr = new HBGLBufferResource();
    g_SphereGridVertexVBOID = HBGLBufferResourcePtr(vbobr);
    g_SphereGridVertexVBOID->Generate();

    HBGLBufferResource* ibobr = new HBGLBufferResource();
    g_SphereGridIndexIBOID = HBGLBufferResourcePtr(ibobr);
    g_SphereGridIndexIBOID->Generate();

    glBindBuffer(GL_ARRAY_BUFFER, g_SphereGridVertexVBOID->GetIndex());
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* 5 * c_SphGridNumLatSpans * (c_SphGridNumLongSpans + 1),
        &grid_vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_SphereGridIndexIBOID->GetIndex());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint)* g_NumSphereGridIndices,
        point_indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

// ========================================================================
// RESOURCE MANAGEMENT
// ========================================================================

bool
ShaderToyVRGenImageTexture(ShaderToyVRChannelType textureType, HBGLTextureResourcePtr& textureResource)
{

    textureResource->Generate();
    GLuint texID = textureResource->GetIndex();

    // TODO: Make better file finding logic
    if (textureType == SHADERTOYVR_RGB_NOISE_256x256_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex16.png", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_R_NOISE_256x256_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex12.png", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_RGB_NOISE_64x64_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex11.png", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_R_NOISE_64x64_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex10.png", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if(textureType == SHADERTOYVR_R_NOISE_8x8_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex15.png", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_STONE_TILES_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex00.jpg", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_OLD_BIRCH_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex01.jpg", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_RUSTED_METAL_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex02.jpg", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_DEEPSKY_PATTERN_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex03.jpg", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_LONDON_STREET_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex04.jpg", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_FINISHED_WOOD_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex05.jpg", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_BARK_AND_LICHEN_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex06.jpg", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_COLORED_ROCKS_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex07.jpg", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_CLOTH_WEAVE_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex08.jpg", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_ANIMAL_PRINT_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex09.jpg", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_NYAN_CAT_TEX)
    {
        texID = SOIL_load_OGL_texture("../resources/tex14.png", 3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else {
        std::cerr << "ShaderToyVR ERROR: unknown image texture type [ " << textureType << " ] " << std::endl;
        texID = 0;
    }

    if (texID == 0)
    {

        std::cerr << "ShaderToyVR ERROR: failed to read in image texture type [ " << textureType << " ] " << std::endl;
        std::cerr << "Possible texture read error: " << SOIL_last_result() << std::endl;
        return false;
    }

    std::cout << SOIL_last_result() << std::endl;

    HB_CHECK_GL_ERROR();
    return true;
}

bool
ShaderToyVRGenCubeMapTexture(ShaderToyVRChannelType textureType, HBGLTextureResourcePtr& textureResource)
{
    textureResource->Generate();
    GLuint texID = textureResource->GetIndex();

    // TODO: Make better file finding logic
    if (textureType == SHADERTOYVR_UFFIZI_GALLERY_512_CUBEMAP)
    {
        texID = SOIL_load_OGL_cubemap(
            "../resources/cube00_0.jpg",
            "../resources/cube00_1.jpg",
            "../resources/cube00_2.jpg",
            "../resources/cube00_3.jpg",
            "../resources/cube00_4.jpg",
            "../resources/cube00_5.jpg",
            3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_UFFIZI_GALLERY_64_CUBEMAP)
    {
        texID = SOIL_load_OGL_cubemap(
            "../resources/cube01_0.png",
            "../resources/cube01_1.png", 
            "../resources/cube01_2.png",
            "../resources/cube01_3.png",
            "../resources/cube01_4.png",
            "../resources/cube01_5.png",
            3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_ST_PETERS_256_CUBEMAP)
    {
        texID = SOIL_load_OGL_cubemap(
            "../resources/cube02_0.jpg",
            "../resources/cube02_1.jpg", 
            "../resources/cube02_2.jpg",
            "../resources/cube02_3.jpg",
            "../resources/cube02_4.jpg",
            "../resources/cube02_5.jpg",
            3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_ST_PETERS_64_CUBEMAP)
    {
        texID = SOIL_load_OGL_cubemap(
            "../resources/cube03_0.png",
            "../resources/cube03_1.png", 
            "../resources/cube03_2.png",
            "../resources/cube03_3.png",
            "../resources/cube03_4.png",
            "../resources/cube03_5.png",
            3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_GROVE_512_CUBEMAP)
    {
        texID = SOIL_load_OGL_cubemap(
            "../resources/cube04_0.png",
            "../resources/cube04_1.png", 
            "../resources/cube04_2.png",
            "../resources/cube04_3.png",
            "../resources/cube04_4.png",
            "../resources/cube04_5.png",
            3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }

    else if (textureType == SHADERTOYVR_GROVE_64_CUBEMAP)
    {
        texID = SOIL_load_OGL_cubemap(
            "../resources/cube05_0.png",
            "../resources/cube05_1.png",
            "../resources/cube05_2.png",
            "../resources/cube05_3.png",
            "../resources/cube05_4.png",
            "../resources/cube05_5.png",
            3, texID, SOIL_FLAG_POWER_OF_TWO | SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
    }


    else {
        std::cerr << "ShaderToyVR ERROR: unknown cubemap texture type [ " << textureType << " ] " << std::endl;
        texID = 0;
    }

    if (texID == 0)
    {
        std::cerr << "ShaderToyVR ERROR: failed to read in cubemap texture type [ " << textureType << " ] " << std::endl;
        std::cerr << "Possible texture read error: " << SOIL_last_result() << std::endl;
        return false;
    }

    std::cout << SOIL_last_result() << std::endl;

    HB_CHECK_GL_ERROR();

    return true;
}

void
ShaderToyVRLoadResources()
{
    // TODO: Allow for reloading resources when the shader is reloaded on the fly

    // FRAGILE: consider re-implementing GetFragmentShader as a virtual function that
    // returns an already cast STVRFragmentShaderPtr.
    HBGLShaderPtr fragShaderPtr = g_ScreenQuadShaderProgram->GetFragmentShader();
    const STVRFragmentShader* stvrFragShader = static_cast<STVRFragmentShader*>(&*fragShaderPtr);

    for (uint inputChannel = uint(SHADERTOYVR_CHANNEL_0); inputChannel < SHADERTOYVR_NUMCHANNELS; inputChannel++)
    {
        ShaderToyVRChannelType inputType = stvrFragShader->GetInputType(static_cast<ShaderToyVRInputChannel>(inputChannel));

        if (inputType == SHADERTOYVR_UNKNOWN_TYPE)
        {
            continue;
        }

        else if (inputType == SHADERTOYVR_RGB_NOISE_256x256_TEX ||
            inputType == SHADERTOYVR_R_NOISE_256x256_TEX ||
            inputType == SHADERTOYVR_RGB_NOISE_64x64_TEX ||
            inputType == SHADERTOYVR_R_NOISE_64x64_TEX ||
            inputType == SHADERTOYVR_R_NOISE_8x8_TEX ||
            inputType == SHADERTOYVR_STONE_TILES_TEX ||
            inputType == SHADERTOYVR_OLD_BIRCH_TEX ||
            inputType == SHADERTOYVR_RUSTED_METAL_TEX ||
            inputType == SHADERTOYVR_DEEPSKY_PATTERN_TEX ||
            inputType == SHADERTOYVR_LONDON_STREET_TEX ||
            inputType == SHADERTOYVR_FINISHED_WOOD_TEX ||
            inputType == SHADERTOYVR_BARK_AND_LICHEN_TEX ||
            inputType == SHADERTOYVR_COLORED_ROCKS_TEX ||
            inputType == SHADERTOYVR_CLOTH_WEAVE_TEX ||
            inputType == SHADERTOYVR_ANIMAL_PRINT_TEX ||
            inputType == SHADERTOYVR_NYAN_CAT_TEX)
        {
            ShaderToyVRGenImageTexture(inputType, g_ChannelTextures[inputChannel]);
        }

        else if (inputType == SHADERTOYVR_UFFIZI_GALLERY_512_CUBEMAP ||
            inputType == SHADERTOYVR_UFFIZI_GALLERY_64_CUBEMAP ||
            inputType == SHADERTOYVR_ST_PETERS_256_CUBEMAP ||
            inputType == SHADERTOYVR_ST_PETERS_64_CUBEMAP ||
            inputType == SHADERTOYVR_GROVE_512_CUBEMAP ||
            inputType == SHADERTOYVR_GROVE_64_CUBEMAP)
        {
            ShaderToyVRGenCubeMapTexture(inputType, g_ChannelTextures[inputChannel]);
        }

        if (g_ChannelTextures[inputChannel])
        {
            GLint width = 0, height = 0;
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
            HB_CHECK_GL_ERROR();
            g_ChannelResolutions[inputChannel][0] = (float)width;
            g_ChannelResolutions[inputChannel][1] = (float)height;
        }
    }
}

// ========================================================================
// OVR MANAGEMENT
// ========================================================================

void
ShaderToyVRInitOVR()
{
    ovr_Initialize();

    g_HMD = ovrHmd_Create(0);
    if (g_HMD != nullptr)
    {
        g_OVRPositionalCamTanHalfFov[0] = tan(g_HMD->CameraFrustumHFovInRadians * .5f);
        g_OVRPositionalCamTanHalfFov[1] = tan(g_HMD->CameraFrustumVFovInRadians * .5f);
    }
    else
    {
        g_HMD = ovrHmd_CreateDebug(ovrHmd_DK2);
        std::cerr << "ShaderToyVR ERROR: Could not find an HMD device.  Creating debug version." << std::endl;

        g_OVRPositionalCamTanHalfFov[0] = tan(1.9 * .5f);
        g_OVRPositionalCamTanHalfFov[1] = tan(1.9 * .5f);
    }

    ovrHmd_SetEnabledCaps(g_HMD, 
        ovrHmdCap_LowPersistence | 
        ovrHmdCap_DynamicPrediction);

    ovrHmd_ConfigureTracking(g_HMD, 
        ovrTrackingCap_Orientation | 
        ovrTrackingCap_MagYawCorrection | 
        ovrTrackingCap_Position, 
        0);

    g_OVRStereoView = true;
}

void
ShaderToyVRCloseOVR()
{
    ovrHmd_Destroy(g_HMD);
    ovr_Shutdown();
}

void
ShaderToyVRGenOVRTextures(const ovrEyeType& eye)
{

    ovrTextureHeader& eyeTextureHeader = g_EyeTextures[eye].OGL.Header;
    eyeTextureHeader.TextureSize = ovrHmd_GetFovTextureSize(g_HMD, eye, g_HMD->DefaultEyeFov[eye], g_ScreenPercentage);
    eyeTextureHeader.RenderViewport.Size = eyeTextureHeader.TextureSize;
    eyeTextureHeader.RenderViewport.Pos.x = 0;
    eyeTextureHeader.RenderViewport.Pos.y = 0;
    eyeTextureHeader.API = ovrRenderAPI_OpenGL;

    glBindTexture(GL_TEXTURE_2D, g_OVRColorTexture[eye]->GetIndex());

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        eyeTextureHeader.TextureSize.w,
        eyeTextureHeader.TextureSize.h, 0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        NULL);

    HB_CHECK_GL_ERROR();

    glBindTexture(GL_TEXTURE_2D, 0);

    g_OVRTextureSize[eye][0] = eyeTextureHeader.TextureSize.w;
    g_OVRTextureSize[eye][1] = eyeTextureHeader.TextureSize.h;

    glBindRenderbuffer(GL_RENDERBUFFER, g_OVRDepthTexture[eye]->GetIndex());

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
        eyeTextureHeader.TextureSize.w,
        eyeTextureHeader.TextureSize.h);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void
ShaderToyVRInitOVRGLSystem()
{
    ovrFovPort eyeFovPorts[2];

    // FRAGILE: consider re-implementing GetFragmentShader as a virtual function that
    // returns an already cast STVRFragmentShaderPtr.
    HBGLShaderPtr fragShaderPtr = g_ScreenQuadShaderProgram->GetFragmentShader();
    const STVRFragmentShader* stvrFragShader = static_cast<STVRFragmentShader*>(&*fragShaderPtr);
    g_ScreenPercentage = stvrFragShader->GetScreenPercentageResolution();

    // Initialize all channel textures.  These may not get generated, but let's
    // allocate them to make everything nice and consistent
    HBGLTextureResource* t1r = new HBGLTextureResource();
    g_ChannelTextures[0] = HBGLTextureResourcePtr(t1r);
    HBGLTextureResource* t2r = new HBGLTextureResource();
    g_ChannelTextures[1] = HBGLTextureResourcePtr(t2r);
    HBGLTextureResource* t3r = new HBGLTextureResource();
    g_ChannelTextures[2] = HBGLTextureResourcePtr(t3r);
    HBGLTextureResource* t4r = new HBGLTextureResource();
    g_ChannelTextures[3] = HBGLTextureResourcePtr(t4r);

    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
        eye < ovrEyeType::ovrEye_Count;
        eye = static_cast<ovrEyeType>(eye + 1)) 
    {

        eyeFovPorts[eye] = g_HMD->DefaultEyeFov[eye];

        HBGLFrameBufferResource* fbr = new HBGLFrameBufferResource();
        g_OVRFrameBuffer[eye] = HBGLFrameBufferResourcePtr(fbr);
        g_OVRFrameBuffer[eye]->Generate();

        HBGLTextureResource* tr = new HBGLTextureResource();
        g_OVRColorTexture[eye] = HBGLTextureResourcePtr(tr);
        g_OVRColorTexture[eye]->Generate();

        HBGLRenderBufferResource* rbr = new HBGLRenderBufferResource();
        g_OVRDepthTexture[eye] = HBGLRenderBufferResourcePtr(rbr);
        g_OVRDepthTexture[eye]->Generate();

        glBindTexture(GL_TEXTURE_2D, g_OVRColorTexture[eye]->GetIndex());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        HB_CHECK_GL_ERROR();

        glBindTexture(GL_TEXTURE_2D, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, g_OVRFrameBuffer[eye]->GetIndex());

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_OVRColorTexture[eye]->GetIndex(), 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, g_OVRDepthTexture[eye]->GetIndex());

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_OVRDepthTexture[eye]->GetIndex());

        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        g_EyeTextures[eye].OGL.TexId = g_OVRFrameBuffer[eye]->GetIndex();

        ShaderToyVRGenOVRTextures(eye);
    }

    ovrGLConfig cfg;
    memset(&cfg, 0, sizeof(ovrGLConfig));
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.BackBufferSize = g_HMD->Resolution;
    cfg.OGL.Header.Multisample = 1; // <-- does this do anything??
    cfg.OGL.Window = glfwGetWin32Window(g_GLFWWindow);
    cfg.OGL.DC = wglGetCurrentDC();

    int distortionCaps = ovrDistortionCap_TimeWarp |
        ovrDistortionCap_Chromatic |
        ovrDistortionCap_Vignette;

    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(g_HMD, 
        &cfg.Config, 
        distortionCaps, 
        eyeFovPorts, 
        eyeRenderDescs);

    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
        eye < ovrEyeType::ovrEye_Count;
        eye = static_cast<ovrEyeType>(eye + 1)) {
        
        g_OVRCamPerspective[eye] = FromOvrMatToMat(ovrMatrix4f_Projection(eyeFovPorts[eye], 0.1f, 4000.f, true));
        g_OVRCamOffset[eye] = FromOvrVecToVec(eyeRenderDescs[eye].HmdToEyeViewOffset);
    }

}

void
ShaderToyVRResetOVRPosition()
{
    if (g_HMD != nullptr)
    {
        ovrHmd_RecenterPose(g_HMD);
    }
}

// ========================================================================
// DRAW CALLS
// ========================================================================

void ShaderToyVRDrawPositionalCam(const glm::mat4& mvp_mat)
{

    glPushAttrib(GL_LINE_BIT | GL_COLOR_BUFFER_BIT);

    glm::vec3 ovrCameraColor(1.f, 0.f, 0.f);
    ovrTrackingState ts = ovrHmd_GetTrackingState(g_HMD, ovr_GetTimeInSeconds());

    if (ts.StatusFlags & (ovrStatus_PositionTracked))
    {
        ovrCameraColor = glm::vec3(0.f, 1.f, 0.f);
        g_OVRPositionalCamXform = FromOvrPoseToMat(ts.CameraPose);
    }
    
    glPushMatrix();

    glMultMatrixf(glm::value_ptr(g_OVRPositionalCamXform));

    float ovrFarPlaneInMeters = g_HMD->CameraFrustumFarZInMeters;

    glColor3fv(glm::value_ptr(ovrCameraColor));

    float d = ovrFarPlaneInMeters;
    float hw = ovrFarPlaneInMeters * g_OVRPositionalCamTanHalfFov[0];
    float hh = ovrFarPlaneInMeters * g_OVRPositionalCamTanHalfFov[1];

    glLineWidth(5.f);
    glBegin(GL_LINES);

    // frustum box - using classic glVertex* because it's so few calls
    glVertex3f(0.f, 0.f, 0.f);  glVertex3f(hw, hh, d);
    glVertex3f(0.f, 0.f, 0.f);  glVertex3f(-hw, hh, d);
    glVertex3f(0.f, 0.f, 0.f);  glVertex3f(-hw, -hh, d);
    glVertex3f(0.f, 0.f, 0.f);  glVertex3f(hw, -hh, d);
    glVertex3f(hw, hh, d);      glVertex3f(-hw, hh, d);
    glVertex3f(-hw, hh, d);     glVertex3f(-hw, -hh, d);
    glVertex3f(-hw, -hh, d);    glVertex3f(hw, -hh, d);
    glVertex3f(hw, -hh, d);     glVertex3f(hw, hh, d);

    // camera center line
    glColor3f(.7f, .0f, .7f);
    glVertex3f(0.f, 0.f, 0.f);  glVertex3f(0.f, 0.f, .05f * d);

    glEnd();

    glPopMatrix();
    glPopAttrib();
}

// -------------------------------------------------------------------------

void
ShaderToyVRDrawScreenQuad(const ovrEyeType& eye)
{
    glBindBuffer(GL_ARRAY_BUFFER, g_ScreenQuadVertexVBOID->GetIndex());
    
    if (c_DebugMode && c_DebugModeRelink) {
        g_ScreenQuadShaderProgram->ReloadLinkedShaders();
    }

    g_ScreenQuadShaderProgram->EnableVertexAttrib("position", 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    g_ScreenQuadShaderProgram->EnableVertexAttrib("texcoord", 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

    if (g_ScreenQuadShaderProgram) {
        g_ScreenQuadShaderProgram->ShadersBegin();
    }

    // FRAGILE: consider re-implementing GetFragmentShader as a virtual function that
    // returns an already cast STVRFragmentShaderPtr.
    HBGLShaderPtr fragShaderPtr = g_ScreenQuadShaderProgram->GetFragmentShader();
    const STVRFragmentShader* stvrFragShader = static_cast<STVRFragmentShader*>(&*fragShaderPtr);

    // TODO: Lots of stuff happens per eye that can be shared.  Clean up later

    // TODO: For some reason, only the first texture id is bound for all active textures
    // so ShaderToyVR currently doesn't support multiple channels!

    // TODO: There is an unexplained INVALID VALUE error in the glUniform1i call in the below.

    for (uint inputChannel = uint(SHADERTOYVR_CHANNEL_0); 
        inputChannel < SHADERTOYVR_NUMCHANNELS; 
        inputChannel++)
    {
        GLuint texID = g_ChannelTextures[inputChannel]->GetIndex();
        if (texID > 0)
        {
            ShaderToyVRChannelType inputType = stvrFragShader->GetInputType(static_cast<ShaderToyVRInputChannel>(inputChannel));
            glActiveTexture(c_ChannelTextures[inputChannel]);
            glBindTexture(stvrFragShader->Is2DTexInput(inputType) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP, texID);

            char channelBuffer[10];
            sprintf(channelBuffer, "iChannel%i", inputChannel);

            g_ScreenQuadShaderProgram->SetUniform1i(channelBuffer, inputChannel);
        }

    }

    g_ScreenQuadShaderProgram->SetUniform1f("iGlobalTime", g_PlaybackTimeInSecs);

    g_ScreenQuadShaderProgram->SetUniform2f("iResolution", 
                                                (GLfloat) g_OVRTextureSize[eye][0],
                                                (GLfloat) g_OVRTextureSize[eye][1]);

    g_ScreenQuadShaderProgram->SetUniform3fv("iChannelResolution", 4, &g_ChannelResolutions[0][0]);
    g_ScreenQuadShaderProgram->SetUniform4f("iDate", g_Date.x, g_Date.y, g_Date.z, g_Date.w);

    // ChannelTime is not yet supported
    g_ScreenQuadShaderProgram->SetUniform1fv("iChannelTime", 4, &g_ChannelTimes[0]);

    // Mouse is disabled
    // TODO: instead of mouse, allow the user to use a joystick or WASD controls.
    g_ScreenQuadShaderProgram->SetUniform4f("iMouse", 0.f, 0.f, 0.f, 0.f);

    // Sample rate is not supported
    g_ScreenQuadShaderProgram->SetUniform1f("iSampleRate", 0.f);
    
    g_ScreenQuadShaderProgram->SetUniformMatrix4fv("iCameraTransform", 1, GL_FALSE, glm::value_ptr(g_OVRCameraTransform[eye]));

    g_ScreenQuadShaderProgram->SetUniform1f("iFocalLength", g_FocalLengthScalar);

    glDisable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    if (g_ScreenQuadShaderProgram) {
        g_ScreenQuadShaderProgram->ShadersEnd();
    }

    g_ScreenQuadShaderProgram->DisableVertexAttrib("position");
    g_ScreenQuadShaderProgram->DisableVertexAttrib("texcoord");

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// -------------------------------------------------------------------------
 
void
ShaderToyVRDrawSphereGrid(const glm::mat4& mvp_mat)
{

    glBindBuffer(GL_ARRAY_BUFFER, g_SphereGridVertexVBOID->GetIndex());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_SphereGridIndexIBOID->GetIndex());

    if (c_DebugMode && c_DebugModeRelink && g_SphereGridShaderProgram) {
        g_SphereGridShaderProgram->ReloadLinkedShaders();
    }

    // TODO: convert to glVertexAttribPointer
    // For some reason, converting to 150 core is not working

//     g_SphereGridShaderProgram->EnableVertexAttrib("position", 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
//     g_SphereGridShaderProgram->EnableVertexAttrib("texcoord", 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, 5 * sizeof(GLfloat), BUFFER_OFFSET(0));
    glTexCoordPointer(2, GL_FLOAT, 5 * sizeof(GLfloat), BUFFER_OFFSET(12));

    if (g_SphereGridShaderProgram) {
        g_SphereGridShaderProgram->ShadersBegin();
    }

    g_SphereGridShaderProgram->SetUniform2f("grid_dims", 
        float(c_SphGridNumLatSpans), 
        float(c_SphGridNumLongSpans) );

    g_SphereGridShaderProgram->SetUniformMatrix4fv("mvp_matrix", 1, GL_FALSE, glm::value_ptr(mvp_mat));

    glPushAttrib(GL_POLYGON_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);

    glLineWidth(5.f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);	// Draw Our SphereGrid In Wireframe Mode

    glDrawElements(GL_QUADS, g_NumSphereGridIndices,
                   GL_UNSIGNED_INT, BUFFER_OFFSET(0));

    glPopAttrib();

    if (g_SphereGridShaderProgram) {
        g_SphereGridShaderProgram->ShadersEnd();
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

//     g_SphereGridShaderProgram->DisableVertexAttrib("position");
//     g_SphereGridShaderProgram->DisableVertexAttrib("texcoord");

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// ========================================================================
// RENDER CALLBACKS
// ========================================================================

void
ShaderToyVRRenderScene(const ovrEyeType& eye)
{

    glViewport(0, 0, g_OVRTextureSize[eye][0], g_OVRTextureSize[eye][1]);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glCullFace(GL_BACK);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
    glHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_FASTEST);

    glShadeModel(GL_SMOOTH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    const glm::mat4& proj_mat = g_OVRCamPerspective[eye];
    glMultMatrixf(glm::value_ptr(g_OVRCamPerspective[eye]));

    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    glm::mat4 eyePose = FromOvrPoseToMat(ovrHmd_GetHmdPosePerEye(g_HMD, eye));
    glm::mat4 modelview_mat;
    if (g_OVRStereoView) {
        modelview_mat = glm::translate(modelview_mat, g_OVRCamOffset[eye]);
    }
    modelview_mat = modelview_mat * glm::inverse(eyePose);

    glMultMatrixf(glm::value_ptr(modelview_mat));

    glm::mat4 camXform = glm::inverse(modelview_mat);

    float fovScale = g_OVRPositionalCamTanHalfFov[1];
    camXform[0] = glm::normalize(camXform[0]);
    camXform[1] = glm::normalize(camXform[1]);
    camXform[2] = fovScale * glm::normalize(camXform[2]);

    // Have the z-axis be negative down the look direction.
    // This is consistent with how positional data is tracked.
    // Closer to camera is more negative.  We'll need to conform
    // to this convention in the shadertoy shaders.
    camXform = glm::scale(camXform, glm::vec3(1.f, 1.f, -1.f));
    g_OVRCameraTransform[eye] = camXform;

    glm::mat4 modelviewproj_mat = proj_mat * modelview_mat;

    glPushMatrix();

    ShaderToyVRDrawScreenQuad(eye);

    if (g_DisplaySphereGrid) {
        ShaderToyVRDrawSphereGrid(modelviewproj_mat);
    }

    if (g_DisplayPositionalCam) {
        ShaderToyVRDrawPositionalCam(modelviewproj_mat);
    }

    glPopMatrix();

    g_OverlayStats->UpdateData("FPS", g_FramesPerSecond);
    g_OverlayStats->UpdateData("Play Time (seconds)", (float)g_PlaybackTimeInSecs);

    if (g_DisplayOverlay) {
        g_OverlayStats->DrawOverlay(g_OVRTextureSize[eye][0], g_OVRTextureSize[eye][1]);
    }
}

// -------------------------------------------------------------------------

void
ShaderToyVRDraw(void)
{


    // TODO: HMD sensor information sent to overlay display
    ovrTrackingState ts = ovrHmd_GetTrackingState(g_HMD, ovr_GetTimeInSeconds());
    if (ts.StatusFlags & (ovrStatus_OrientationTracked))
    {
        ovrPosef pose = ts.HeadPose.ThePose;
        float eyeYaw = 0.f;
        float eyePitch = 0.f;
        float eyeRoll = 0.f;

        Quatf quatOrientation(pose.Orientation.x, pose.Orientation.y, pose.Orientation.z, pose.Orientation.w);
        quatOrientation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&eyeYaw, &eyePitch, &eyeRoll);

        if (ts.StatusFlags & (ovrStatus_PositionTracked)) 
        {
            g_OverlayStats->UpdateData("Eye X", pose.Position.x);
            g_OverlayStats->UpdateData("Eye Y", pose.Position.y);
            g_OverlayStats->UpdateData("Eye Z", pose.Position.z);
        }
        else 
        {
            g_OverlayStats->UpdateData("Eye X", 0.f);
            g_OverlayStats->UpdateData("Eye Y", 0.f);
            g_OverlayStats->UpdateData("Eye Z", 0.f);
        }

        g_OverlayStats->UpdateData("Eye Yaw", eyeYaw);
        g_OverlayStats->UpdateData("Eye Pitch", eyePitch);
        g_OverlayStats->UpdateData("Eye Roll", eyeRoll);
    }

    ovrHmd_BeginFrame(g_HMD, g_FrameNumber);

    static ovrPosef eyePoses[2];
    for (int i = 0; i < 2; i++)
    {
        ovrEyeType eye = g_HMD->EyeRenderOrder[i];
        eyePoses[eye] = ovrHmd_GetHmdPosePerEye(g_HMD, eye);

        glBindFramebuffer(GL_FRAMEBUFFER, g_OVRFrameBuffer[eye]->GetIndex());

        ShaderToyVRRenderScene(eye);
    }

    ovrTexture textures[2] = { g_EyeTextures[0].Texture, g_EyeTextures[1].Texture };
    ovrHmd_EndFrame(g_HMD, eyePoses, textures);
}

// ========================================================================
// KEYSTROKE CALLBACKS
// ========================================================================

void
ShaderToyVRUpdateOVRScreenPercentage(float delta)
{

    g_ScreenPercentage = std::clamp(g_ScreenPercentage + delta, .2f, 2.0f);
    std::cout << "Screen Percentage: " << g_ScreenPercentage << std::endl;

    for (ovrEyeType eye = ovrEyeType::ovrEye_Left;
        eye < ovrEyeType::ovrEye_Count;
        eye = static_cast<ovrEyeType>(eye + 1))
    {
        ShaderToyVRGenOVRTextures(eye);
    }
}

void
ShaderToyVRUpdateFocalLengthScalar(float delta)
{

    g_FocalLengthScalar = std::clamp(g_FocalLengthScalar + delta, .1f, 4.0f);
    std::cout << "Focal Length Scalar: " << g_FocalLengthScalar << std::endl;
}


void
ShaderToyVRToggleStereoView()
{

    g_OVRStereoView = !g_OVRStereoView;
    if (!g_OVRStereoView) {
        std::cout << "Disabling Stereo View" << std::endl;
    }
    else {
        std::cout << "Enabling Stereo View" << std::endl;
    }

}
void
ShaderToyVRToggleDisplayOverlay()
{
    g_DisplayOverlay = !g_DisplayOverlay;
    if (!g_DisplayOverlay) {
        std::cout << "Hiding Overlay" << std::endl;
    } else {
        std::cout << "Showing Overlay" << std::endl;
    }

}

void
ShaderToyVRToggleDisplaySphereGrid ()
{
    g_DisplaySphereGrid = !g_DisplaySphereGrid;
    if (!g_DisplaySphereGrid) {
        std::cout << "Hiding Sphere Grid" << std::endl;
    }
    else {
        std::cout << "Showing Sphere Grid" << std::endl;
    }

}

void
ShaderToyVRToggleDisplayPositionalCam()
{
    g_DisplayPositionalCam = !g_DisplayPositionalCam;
    if (!g_DisplayPositionalCam) {
        std::cout << "Hiding Positional Camera" << std::endl;
    }
    else {
        std::cout << "Showing Positional Camera" << std::endl;
    }

}

void 
ShaderToyVRGLFWErrorCallback(int error, const char* description)
{
    std::cerr << "ShaderToy GLFW Error: [ " << error << " ] ::\n" << description << std::endl;
}


void
ShaderToyVRGLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // TODO: Implement adaptive ScreenPercentage mode (keyed by p) - adjust ScreenPercentage until FPS is
    // acceptable

    // TODO: Implement ability to reload shaders and resources on the fly (keyed by o)

    // TODO: Implement SpaceBar to play and pause experiences

    // TODO: Implement WASD keys to fly with look direction

    if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        ShaderToyVRToggleStereoView();
    }

    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        ShaderToyVRToggleDisplayOverlay();
    }

    if (key == GLFW_KEY_G && action == GLFW_PRESS)
    {
        ShaderToyVRToggleDisplaySphereGrid();
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        ShaderToyVRToggleDisplayPositionalCam();
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        ShaderToyVRResetOVRPosition();
    }

    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS)
    {
        ShaderToyVRUpdateOVRScreenPercentage(-.1f);
    }

    if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
    {
        ShaderToyVRUpdateOVRScreenPercentage(.1f);
    }
    if (key == GLFW_KEY_COMMA && action == GLFW_PRESS)
    {
        ShaderToyVRUpdateFocalLengthScalar(-.1f);
    }

    if (key == GLFW_KEY_PERIOD && action == GLFW_PRESS)
    {
        ShaderToyVRUpdateFocalLengthScalar(.1f);
    }
}

// ========================================================================
// STATE UPDATE 
// ========================================================================

void
ShaderToyVRResetWorldTimer() {
    g_PlaybackResetInSecs = (float)glfwGetTime();
}

void
ShaderToyVRUpdateTime()
{
    g_FrameNumber++;
    
    if (g_PlaybackTimeInSecs - g_TimebaseInSecs > 1.0f)
    {
        g_FramesPerSecond = g_FrameNumber / (g_PlaybackTimeInSecs - g_TimebaseInSecs);
        g_TimebaseInSecs = g_PlaybackTimeInSecs;
        g_FrameNumber = 0;
    }

    if (g_Playing)
    {
        g_PlaybackTimeInSecs = (float)ovr_GetTimeInSeconds() - g_PlaybackResetInSecs;
    }

    // Channel times are not supported until we have video or music
    g_ChannelTimes[0] = g_PlaybackTimeInSecs;
    g_ChannelTimes[1] = g_PlaybackTimeInSecs;
    g_ChannelTimes[2] = g_PlaybackTimeInSecs;
    g_ChannelTimes[3] = g_PlaybackTimeInSecs;

    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);
    g_Date.x = sysTime.wYear;
    g_Date.y = sysTime.wMonth;
    g_Date.z = sysTime.wDay;
    g_Date.w = sysTime.wHour * 3600.f + sysTime.wMinute * 60.f + sysTime.wSecond;

}

void
ShaderToyVRSetupViewWindow()
{

    int count = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&count);

    std::cout << "Monitors available: " << std::endl;
    for (int idx = 0; idx < count; idx++)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[idx]);
        const char* name = glfwGetMonitorName(monitors[idx]);
        std::cout << "\t" << idx << " [ " << name << " ] : Resolution: " << mode->width << " x " << mode->height << std::endl;
    }

    // TODO: add application argument so user can open in windowed mode.
    // TODO: add ability for user to specify the monitor index of the DK2.
    // TODO: get native display working

    if (count >= 2 && !c_DebugWindowed)
    {
        int dk2idx = count - 1;
        const char* name = glfwGetMonitorName(monitors[dk2idx]);
        const GLFWvidmode* mode = glfwGetVideoMode(monitors[dk2idx]);
        std::cout << "Assuming DK2 Monitor: " << dk2idx << " [ " << name << " ] : " << mode->width << " x " << mode->height << std::endl;

        GLFWmonitor* hmdMonitor = monitors[dk2idx];
        g_GLFWWindow = glfwCreateWindow(c_DefaultWindowWidth, 
                                        c_DefaultWindowHeight, 
                                        "ShaderToyVR", 
                                        hmdMonitor, 
                                        NULL);

        glfwSetInputMode(g_GLFWWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    }
    else
    {

        g_GLFWWindow = glfwCreateWindow(c_DefaultWindowWidth, 
            c_DefaultWindowHeight, 
            "ShaderToyVR", NULL, NULL);

        if (c_DebugWindowed)
        {
            // I use two monitors when developing, so my debug build puts the windowed version
            // on my right monitor.
            glfwSetWindowPos(g_GLFWWindow, 2600, 100);
        }
        else
        {
            glfwSetWindowPos(g_GLFWWindow, 100, 100);
        }

    }

    if (!g_GLFWWindow) { 
        ShaderToyVRErrorAndQuit(); 
    }

    glfwMakeContextCurrent(g_GLFWWindow);
    glfwSetKeyCallback(g_GLFWWindow, ShaderToyVRGLFWKeyCallback);

}

// ========================================================================
// SHUTDOWN
// ========================================================================

void ShaderToyVRShutdown()
{
    ShaderToyVRCloseOVR();
    glfwTerminate();
    HB_CHECK_GL_ERROR();
}

void ShaderToyVRQuit()
{
    ShaderToyVRShutdown();
    exit(EXIT_SUCCESS);
}

void ShaderToyVRErrorAndQuit()
{
    ShaderToyVRShutdown();
    // sleep before exiting so user can see error messages
    std::cerr << "ShaderToyVR FATAL ERROR: Shutting down because of errors above ... " << std::endl;
    Sleep(5000);
    exit(EXIT_FAILURE);
}

// ========================================================================
// MAIN
// ========================================================================

int 
main(int argc, char** argv){

    // TODO: rudimentary argument parsing
    // TODO: Get working on a mac!

#ifdef __MACOSX__
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    glfwSetErrorCallback(ShaderToyVRGLFWErrorCallback);

    if (!glfwInit()) { 
        ShaderToyVRErrorAndQuit(); 
    }

    ShaderToyVRSetupViewWindow();

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
         std::cerr << "ShaderToy GLEW Error: [ " << err << " ] ::" << glewGetErrorString(err) << std::endl;
         ShaderToyVRErrorAndQuit();
    }

    g_OverlayStats = HBGLOverlayStatsPtr(new HBGLOverlayStats());

    ShaderToyVRInitShaderSystem();

    ShaderToyVRInitOVR();
    ShaderToyVRInitOVRGLSystem();

    ShaderToyVRLoadResources();

    ovrHmd_AttachToWindow(g_HMD, glfwGetWin32Window(g_GLFWWindow), nullptr, nullptr);

    ShaderToyVRGenSphereGridBuffers();
    ShaderToyVRGenScreenQuadBuffers();

    ShaderToyVRResetWorldTimer();

    g_OverlayStats->AddDataKey("FPS", g_FramesPerSecond, 4);
    //g_OverlayStats->AddDataKey("Play Time (seconds)", (float) g_PlaybackTimeInSecs);

    // TODO - so annoying!
    ovrHmd_DismissHSWDisplay(g_HMD);

    while (!glfwWindowShouldClose(g_GLFWWindow))
    {
        ShaderToyVRUpdateTime();
        ShaderToyVRDraw();
        glfwPollEvents();
    }
    
    ShaderToyVRQuit();
}
