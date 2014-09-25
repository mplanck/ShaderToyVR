
#include "STVRShaders.h"
#include "HBGLUtils.h"

#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>

// ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// STATIC FUNCTIONS
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

// ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// STVRVertexShader 
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

static const char* STVRVertexShaderString =
"#version 120\n"

"in vec2 position;\n"

"void main()\n"
"{\n"
"   gl_Position = vec4(position, 0.0, 1.0);\n"

"}\n";


STVRVertexShader::STVRVertexShader() : HBGLVertexShader("")
{
    LoadInternalSource();
}

// ----------------------------------------------------------------------------

STVRVertexShader::~STVRVertexShader()
{

}

// ----------------------------------------------------------------------------

bool
STVRVertexShader::LoadInternalSource()
{
    if (m_shaderSource != 0) {
        delete[] m_shaderSource;
    }

    int srcLength = strlen(STVRVertexShaderString);
    m_shaderSource = (GLchar*) new char[srcLength + 1];

    memcpy(m_shaderSource, STVRVertexShaderString, srcLength + 1);
    m_shaderSource[srcLength] = 0;
    return true;
}

// ----------------------------------------------------------------------------

bool
STVRVertexShader::LoadFile(const std::string& filePath)
{
    std::cerr << "STVRVertexShader::LoadFile not supported" << std::endl;
    return false;
}

// ----------------------------------------------------------------------------

bool
STVRVertexShader::LoadSource(const std::string& programSource)
{
    std::cerr << "STVRVertexShader::LoadSource not supported" << std::endl;
    return false;
}


// ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// STVRFragmentShader 
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

static const char* STVRFragmentShaderHeader =
"#version 120\n"

"uniform float     iGlobalTime;\n"
"uniform vec2      iMouse;\n"
"uniform vec2      iResolution;\n"
"uniform float     iChannelTime[4];\n"
"uniform vec4      iDate;\n"
"uniform vec3      iChannelResolution[4];\n"
"uniform mat4      iCameraTransform;\n"
"uniform float     iFocalLength;\n\n";

static const char* STVRFragmentShaderChannelHeader[4] = {
    "uniform %s iChannel0;\n"
    ,
    "uniform %s iChannel1;\n"
    ,
    "uniform %s iChannel2;\n"
    ,
    "uniform %s iChannel3;\n"
};

STVRFragmentShader::STVRFragmentShader(const std::string& filePath) : 
HBGLFragmentShader(""), 
m_screenPercentage(1.f)
{
    LoadFile(filePath);
}

// ----------------------------------------------------------------------------

STVRFragmentShader::~STVRFragmentShader()
{

}

// ----------------------------------------------------------------------------

bool
STVRFragmentShader::ConvertStringToInputType(const char* inputTypeString,
ShaderToyVRChannelType& outputType) const
{
    if (strcmp(inputTypeString, "noise_rgb_256") == 0) {
        outputType = SHADERTOYVR_RGB_NOISE_256x256_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "noise_r_256") == 0) {
        outputType = SHADERTOYVR_R_NOISE_256x256_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "noise_rgb_64") == 0) {
        outputType = SHADERTOYVR_RGB_NOISE_64x64_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "noise_r_64") == 0) {
        outputType = SHADERTOYVR_R_NOISE_64x64_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "noise_r_8") == 0) {
        outputType = SHADERTOYVR_R_NOISE_8x8_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "stone_tiles") == 0) {
        outputType = SHADERTOYVR_STONE_TILES_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "old_birch") == 0) {
        outputType = SHADERTOYVR_OLD_BIRCH_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "rusted_metal") == 0) {
        outputType = SHADERTOYVR_RUSTED_METAL_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "deepsky_pattern") == 0) {
        outputType = SHADERTOYVR_DEEPSKY_PATTERN_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "london_street") == 0) {
        outputType = SHADERTOYVR_LONDON_STREET_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "finished_wood") == 0) {
        outputType = SHADERTOYVR_FINISHED_WOOD_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "bark_and_lichen") == 0) {
        outputType = SHADERTOYVR_BARK_AND_LICHEN_TEX;
        return true;
    }

    if (strcmp(inputTypeString, "colored_rocks") == 0) {
        outputType = SHADERTOYVR_COLORED_ROCKS_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "cloth_weave") == 0) {
        outputType = SHADERTOYVR_CLOTH_WEAVE_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "animal_print") == 0) {
        outputType = SHADERTOYVR_ANIMAL_PRINT_TEX;
        return true;
    }
    if (strcmp(inputTypeString, "nyan_cat") == 0) {
        outputType = SHADERTOYVR_NYAN_CAT_TEX;
        return true;
    }

    if (strcmp(inputTypeString, "uffizi_gallery_512") == 0) {
        outputType = SHADERTOYVR_UFFIZI_GALLERY_512_CUBEMAP;
        return true;
    }
    if (strcmp(inputTypeString, "uffizi_gallery_64") == 0) {
        outputType = SHADERTOYVR_UFFIZI_GALLERY_64_CUBEMAP;
        return true;
    }
    if (strcmp(inputTypeString, "st_peters_256") == 0) {
        outputType = SHADERTOYVR_ST_PETERS_256_CUBEMAP;
        return true;
    }
    if (strcmp(inputTypeString, "st_peters_64") == 0) {
        outputType = SHADERTOYVR_ST_PETERS_64_CUBEMAP;
        return true;
    }
    if (strcmp(inputTypeString, "grove_512") == 0) {
        outputType = SHADERTOYVR_GROVE_512_CUBEMAP;
        return true;
    }

    if (strcmp(inputTypeString, "grove_64") == 0) {
        outputType = SHADERTOYVR_GROVE_64_CUBEMAP;
        return true;
    }

    std::cerr << "STVRFragmentShader ERROR [ " << this->GetName() << " ]: cannot parse shadertoy input type: " << inputTypeString << std::endl;
    return false;
}

// ----------------------------------------------------------------------------

bool
STVRFragmentShader::ConvertStringToInputChannel(const char* inputChannelString,
ShaderToyVRInputChannel& inputChannel) const
{

    if (strcmp(inputChannelString, "iChannel0") == 0) {
        inputChannel = SHADERTOYVR_CHANNEL_0;
        return true;
    }

    if (strcmp(inputChannelString, "iChannel1") == 0) {
        inputChannel = SHADERTOYVR_CHANNEL_1;
        return true;
    }

    if (strcmp(inputChannelString, "iChannel2") == 0) {
        inputChannel = SHADERTOYVR_CHANNEL_2;
        return true;
    }

    if (strcmp(inputChannelString, "iChannel3") == 0) {
        inputChannel = SHADERTOYVR_CHANNEL_3;
        return true;
    }

    std::cerr << "STVRFragmentShader ERROR [ " << this->GetName() << " ]: cannot parse shadertoy input channel: " << inputChannelString << std::endl;
    return false;
}

// ----------------------------------------------------------------------------

ShaderToyVRChannelType
STVRFragmentShader::GetInputType(ShaderToyVRInputChannel inputChannel) const
{
    ShaderToyVRInputMap::const_iterator inputIter = m_shaderInputs.find(inputChannel);
    if (inputIter == m_shaderInputs.end()) {
        return SHADERTOYVR_UNKNOWN_TYPE;
    }
    else {
        return inputIter->second;
    }

}

// ----------------------------------------------------------------------------

bool
STVRFragmentShader::Is2DTexInput(ShaderToyVRChannelType inputType) const
{
    if (inputType == SHADERTOYVR_UFFIZI_GALLERY_512_CUBEMAP ||
        inputType == SHADERTOYVR_UFFIZI_GALLERY_64_CUBEMAP ||
        inputType == SHADERTOYVR_ST_PETERS_256_CUBEMAP ||
        inputType == SHADERTOYVR_ST_PETERS_64_CUBEMAP ||
        inputType == SHADERTOYVR_GROVE_512_CUBEMAP ||
        inputType == SHADERTOYVR_GROVE_64_CUBEMAP)
    {
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------

float 
STVRFragmentShader::GetScreenPercentageResolution() const
{
    return m_screenPercentage;
}

// ----------------------------------------------------------------------------

bool
STVRFragmentShader::ConvertKeyAndValue(const char* inputKey, const char* inputValue)
{

    if (strcmp(inputKey, "ScreenPercentage") == 0)
    {
        // BEWARE: undefined behavior if the inputValue is not provided
        m_screenPercentage = static_cast<float>(atof(inputValue));

    }
    else
    {
        ShaderToyVRChannelType inputType;
        ShaderToyVRInputChannel inputChannel;

        if (!ConvertStringToInputChannel(inputKey, inputChannel)) {
            return false;
        }

        if (!ConvertStringToInputType(inputValue, inputType)) {
            return false;
        }

        m_shaderInputs[inputChannel] = inputType;
    }

    return true;
}

bool
STVRFragmentShader::LoadFile(const std::string& filePath)
{

    m_shaderName = filePath;
    std::string realFilePath = HBGLUtils::GetRealFilePath(filePath.c_str());

    FILE * shaderFile; 
    fopen_s(&shaderFile, realFilePath.c_str(), "r");    

    if (!shaderFile) {
        std::cerr << "STVRFragmentShader ERROR [ " << this->GetName() << " ]: Could not load file [ " << filePath << " ] " << std::endl;
        return false;
    }

    if (m_shaderSource != 0) // there is already a source loaded, free it!
    {
        delete[] m_shaderSource;
    }

    unsigned long bufferSize = HBGLShader::GetFileEndPosition(shaderFile);
    bufferSize += strlen(STVRFragmentShaderHeader);

    // We are allocating more space then we will need based on overall file length,
    // but we will 0 terminate the buffer once we've parsed and constructed the 
    // entire shader.
    m_shaderSource = (GLchar*) new char[bufferSize + 1];

    if (m_shaderSource == 0) {
        std::cerr << "STVRFragmentShader ERROR [ " << this->GetName() << " ]: Unable to allocate memory to keep shader source in memory [ " << realFilePath << " ] " << std::endl;
        fclose(shaderFile);
        return false;
    }

    int fileError = 0;
    bool readingHeader = true;
    bool newline = true;
    bool ignoreline = false;
    bool readingInputKey = true;
    unsigned long shaderCharIdx = 0;
    char inputKey[64]; unsigned short inputKeyCharIdx = 0;
    char inputValue[64]; unsigned short inputValueCharIdx = 0;

    // embarrassingly fragile parser of the simple shadertoy file format with
    // input definitions above.
    while (!feof(shaderFile) && !(fileError = ferror(shaderFile))) 
    {
        char currChar = fgetc(shaderFile);
        if (currChar == EOF) {
            break;
        }

        if (readingHeader) {

            if (newline && currChar == ':') 
            {
                readingHeader = false;
                memcpy(&m_shaderSource[0], STVRFragmentShaderHeader, strlen(STVRFragmentShaderHeader));                
                shaderCharIdx += strlen(STVRFragmentShaderHeader);

                for (ShaderToyVRInputMap::const_iterator inputIter = m_shaderInputs.begin();
                    inputIter != m_shaderInputs.end();
                    inputIter++)
                {
                    const char* inputHeaderTemplate = STVRFragmentShaderChannelHeader[inputIter->first];

                    char inputHeader[64];
                    sprintf_s(inputHeader, inputHeaderTemplate, Is2DTexInput(inputIter->second) ? "sampler2D" : "samplerCube");
                    memcpy(&m_shaderSource[shaderCharIdx], inputHeader, strlen(inputHeader));
                    shaderCharIdx += strlen(inputHeader);
                }

                // flush characters until the end of the line and then continue reading the body
                // of the shader
                while ((currChar = fgetc(shaderFile)) != '\n') {}
                continue;

            }

            if (currChar == '\n')
            {
                newline = true;
                readingInputKey = true;
                if (inputKeyCharIdx && inputKeyCharIdx)
                {
                    inputKey[inputKeyCharIdx] = 0;
                    inputValue[inputValueCharIdx] = 0;
                    if (!ConvertKeyAndValue(inputKey, inputValue)) {
                        break;
                    }
                }

                inputKeyCharIdx = 0;
                inputValueCharIdx = 0;
                ignoreline = false;
                continue;
            }

            if ((currChar == ' ') || (currChar == ';')) {
                // ignore whitespace and unnecessary semicolons
                continue;
            }

            if (currChar == '/') {
                char nextChar = fgetc(shaderFile);
                if (nextChar == '/') {
                    ignoreline = true;
                    continue;
                }
            }

            if (ignoreline) {
                continue;
            }

            if (currChar == '=') {
                readingInputKey = false;
                continue;
            }

            if (readingInputKey) {
                inputKey[inputKeyCharIdx++] = currChar;
            }
            else {
                inputValue[inputValueCharIdx++] = currChar;
            }

        } 
        else 
        {
            // reading body of shader
            m_shaderSource[shaderCharIdx++] = currChar;
        }
    }

    // 0 terminate the shader source
    m_shaderSource[shaderCharIdx] = 0;

    if (readingHeader) {
        std::cerr << "STVRFragmentShader ERROR [ " << this->GetName() << " ]: did not match the expected format " << \
            " [ " << realFilePath << " ] There needs to be 2 valid sections to the shader file separated by a line that " << \
            "begins with a colon." << std::endl;
        // the shader was invalid, make it an empty string
        m_shaderSource[0] = 0;
        fclose(shaderFile);
        return false;
    }

    if (fileError > 0) {
        std::cerr << "STVRFragmentShader ERROR [ " << this->GetName() << " ]: parsing ShaderToy fragment shader [ " << realFilePath << " ] Error: " << fileError << std::endl;
        // the shader was invalid, make it an empty string
        m_shaderSource[0] = 0;
        fclose(shaderFile);
        return false;
    }

    fclose(shaderFile);

    m_shaderFilePath = realFilePath;

    return true;
}

// ----------------------------------------------------------------------------

bool
STVRFragmentShader::LoadSource(const std::string& programSource)
{
    std::cerr << "STVRFragmentShader::LoadSource not supported" << std::endl;
    return false;
}

// ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// STVRDebugGridVertexShader 
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,


// static const char* STVRDebugGridVertexShaderString =
// "#version 150 core\n"
// 
// "in vec3 position;\n"
// "in mat4 mvp_matrix;\n"
// 
// "void main()\n"
// "{\n"
// "   gl_Position = mvp_matrix * vec4(position, 1.);\n"
// "}\n";

// TODO: convert over to using glVertexAttribPointer

static const char* STVRDebugGridVertexShaderString =
"#version 130\n"

"varying vec4 texcoord;\n"

"void main()\n"
"{\n"
"   texcoord = gl_MultiTexCoord0;\n"
"   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"}\n";

STVRDebugGridVertexShader::STVRDebugGridVertexShader() : HBGLVertexShader("")
{
    LoadInternalSource();
}

// ----------------------------------------------------------------------------

STVRDebugGridVertexShader::~STVRDebugGridVertexShader()
{

}

// ----------------------------------------------------------------------------

bool
STVRDebugGridVertexShader::LoadInternalSource()
{
    if (m_shaderSource != 0) {
        delete[] m_shaderSource;
    }

    int srcLength = strlen(STVRDebugGridVertexShaderString);
    m_shaderSource = (GLchar*) new char[srcLength + 1];

    memcpy(m_shaderSource, STVRDebugGridVertexShaderString, srcLength + 1);
    m_shaderSource[srcLength] = 0;
    return true;
}

// ----------------------------------------------------------------------------

bool
STVRDebugGridVertexShader::LoadFile(const std::string& filePath)
{
    std::cerr << "STVRDebugGridVertexShader::LoadFile not supported" << std::endl;
    return false;
}

// ----------------------------------------------------------------------------

bool
STVRDebugGridVertexShader::LoadSource(const std::string& programSource)
{
    std::cerr << "STVRDebugGridVertexShader::LoadSource not supported" << std::endl;
    return false;
}

// ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// STVRDebugGridFragmentShader 
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

static const char* STVRDebugGridFragmentShaderString =
"#version 130\n"

"uniform vec2 grid_dims;\n"
"varying vec4 texcoord;\n"

"#define PI 3.14\n\n"

"void main()\n"
"{\n"
"   float lat_mask = mix(1., .0, cos(grid_dims.y * 2. * PI * texcoord.y));\n"
"   float long_mask = mix(1., .0, cos((grid_dims.x-1.) * 2. * PI * texcoord.x));\n"
"   gl_FragColor = pow(max(lat_mask, long_mask), 2.) * mix(vec4(1., .5, 0., 1.), vec4(.2, 1., 0., 1.), texcoord.x);\n"
"}\n";

STVRDebugGridFragmentShader::STVRDebugGridFragmentShader() : HBGLFragmentShader("")
{
    LoadInternalSource();
}

// ----------------------------------------------------------------------------

STVRDebugGridFragmentShader::~STVRDebugGridFragmentShader()
{

}

// ----------------------------------------------------------------------------

bool
STVRDebugGridFragmentShader::LoadInternalSource()
{
    if (m_shaderSource != 0) {
        delete[] m_shaderSource;
    }

    int srcLength = strlen(STVRDebugGridFragmentShaderString);
    m_shaderSource = (GLchar*) new char[srcLength + 1];

    memcpy(m_shaderSource, STVRDebugGridFragmentShaderString, srcLength + 1);
    m_shaderSource[srcLength] = 0;
    return true;
}

// ----------------------------------------------------------------------------

bool
STVRDebugGridFragmentShader::LoadFile(const std::string& filePath)
{
    std::cerr << "STVRDebugGridFragmentShader::LoadFile not supported" << std::endl;
    return false;
}

// ----------------------------------------------------------------------------

bool
STVRDebugGridFragmentShader::LoadSource(const std::string& programSource)
{
    std::cerr << "STVRDebugGridFragmentShader::LoadSource not supported" << std::endl;
    return false;
}

// ----------------------------------------------------------------------------
