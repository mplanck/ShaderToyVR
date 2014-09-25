#pragma once

#include "HBGLShaders.h"

#include <GL/glew.h>
#include <map>
#include <string>

using namespace HBGLUtils;

//-----------------------------------------------------------------------------
// Shader class that defines a subclass of a vertex shader. This vertex shader
// comes with a boiler plate setup and so it does not allow loading custom 
// file or source strings.

class  STVRVertexShader : public HBGLVertexShader
{
public:

    STVRVertexShader();
    virtual ~STVRVertexShader();

    // override LoadFile and LoadSource to do nothing since the vertex
    // shader is hard coded.
    virtual bool LoadFile(const std::string& filePath) override;
    virtual bool LoadSource(const std::string& programSource) override;

private:

    // Load a hard coded string to represent the vertex shader
    bool LoadInternalSource();
};

//-----------------------------------------------------------------------------
// This subclass of HBGLFragmentShader loads in a file or source and assumes 
// a special syntax.  The first part of the file (before the colon break) is a 
// simple key/value pairing which specifies which textures to load in iChannel0,
// iChannel1, iChannel2, or iChannel3.  The second part of the file is the glsl 
// code that should be copy and pasted from Shadertoyr
// Example syntax:

// iChannel0 = noise_256x256_TEX
// iChannel1 = noise_8x8
// ScreenPercentage = .5f;

// ::::::::::::::::::::::::::::::::::::::::::::::::::::

// void main(void) 
// {
//    vec3 scene_color = vec3(1.);
//    ...
//    gl_FragColor = vec4(scene_color, 1.);
// }

enum ShaderToyVRChannelType {
    SHADERTOYVR_RGB_NOISE_256x256_TEX = 0,
    SHADERTOYVR_R_NOISE_256x256_TEX,
    SHADERTOYVR_RGB_NOISE_64x64_TEX,
    SHADERTOYVR_R_NOISE_64x64_TEX,
    SHADERTOYVR_R_NOISE_8x8_TEX,
    SHADERTOYVR_STONE_TILES_TEX,
    SHADERTOYVR_OLD_BIRCH_TEX,
    SHADERTOYVR_RUSTED_METAL_TEX,
    SHADERTOYVR_DEEPSKY_PATTERN_TEX,
    SHADERTOYVR_LONDON_STREET_TEX,
    SHADERTOYVR_FINISHED_WOOD_TEX,
    SHADERTOYVR_BARK_AND_LICHEN_TEX,
    SHADERTOYVR_COLORED_ROCKS_TEX,
    SHADERTOYVR_CLOTH_WEAVE_TEX,
    SHADERTOYVR_ANIMAL_PRINT_TEX,
    SHADERTOYVR_NYAN_CAT_TEX,
    SHADERTOYVR_UFFIZI_GALLERY_512_CUBEMAP,
    SHADERTOYVR_UFFIZI_GALLERY_64_CUBEMAP,
    SHADERTOYVR_ST_PETERS_256_CUBEMAP,
    SHADERTOYVR_ST_PETERS_64_CUBEMAP,
    SHADERTOYVR_GROVE_512_CUBEMAP,
    SHADERTOYVR_GROVE_64_CUBEMAP,
    SHADERTOYVR_UNKNOWN_TYPE

};

enum ShaderToyVRInputChannel {
    SHADERTOYVR_CHANNEL_0 = 0,
    SHADERTOYVR_CHANNEL_1,
    SHADERTOYVR_CHANNEL_2,
    SHADERTOYVR_CHANNEL_3,
    SHADERTOYVR_NUMCHANNELS
};

typedef std::map<ShaderToyVRInputChannel, ShaderToyVRChannelType> ShaderToyVRInputMap;


class  STVRFragmentShader : public HBGLFragmentShader
{
public:

    STVRFragmentShader(const std::string& filePath);
    ~STVRFragmentShader();

    // LoadFile parses the syntax mentioned above, 
    virtual bool LoadFile(const std::string& filePath) override;

    // Load source is unsupported
    virtual bool LoadSource(const std::string& programSource) override;

    ShaderToyVRChannelType GetInputType(ShaderToyVRInputChannel inputChannel) const;

    bool Is2DTexInput(ShaderToyVRChannelType inputType) const;
    float GetScreenPercentageResolution() const;

protected:

    bool ConvertKeyAndValue(const char* inputKey,
        const char* inputValue);

    bool ConvertStringToInputType (const char* inputTypeString, 
        ShaderToyVRChannelType& inputType) const;

    bool ConvertStringToInputChannel(const char* inputChannelString,
        ShaderToyVRInputChannel& inputChannel) const;

    ShaderToyVRInputMap m_shaderInputs;
    float m_screenPercentage;
};

//-----------------------------------------------------------------------------
// Shader class that defines a subclass of a vertex shader for drawing the 
// debug grid

class  STVRDebugGridVertexShader : public HBGLVertexShader
{
public:

    STVRDebugGridVertexShader();
    virtual ~STVRDebugGridVertexShader();

    // override LoadFile and LoadSource to do nothing since the vertex
    // shader is hard coded.
    virtual bool LoadFile(const std::string& filePath) override;
    virtual bool LoadSource(const std::string& programSource) override;

private:

    // Load a hard coded string to represent the vertex shader
    bool LoadInternalSource();
};

//-----------------------------------------------------------------------------
// Shader class that defines a subclass of a fragment shader for drawing the 
// debug grid

class  STVRDebugGridFragmentShader : public HBGLFragmentShader
{
public:

    STVRDebugGridFragmentShader();
    ~STVRDebugGridFragmentShader();

    // override LoadFile and LoadSource to do nothing since the vertex
    // shader is hard coded.
    virtual bool LoadFile(const std::string& filePath) override;
    virtual bool LoadSource(const std::string& programSource) override;

private:

    // Load a hard coded string to represent the vertex shader
    bool LoadInternalSource();
};
