
#include "HBGLShaders.h"
#include "HBGLUtils.h"

#include <fstream>
#include <string>
#include <cstdlib>
#include <cstdio>

using namespace HBGLUtils;

//#define HBGLSHADERS_DEBUG_INPUTSHADER

// ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// STATIC FUNCTIONS
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

unsigned long
HBGLShader::GetFileEndPosition(FILE * file)
{
    if (ferror(file) > 0) {
        return 0;
    }

    fseek(file, 0, SEEK_END);
    unsigned long len = ftell(file);
    fseek(file, 0, SEEK_SET);

    return len;
}

// ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// HBGLShaderProgram 
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,


HBGLShaderProgram::HBGLShaderProgram(const char* name) :
m_programIndex(0),
m_programName(name),
m_attributeIndex(1),
m_programLog(NULL)
{
    m_programIndex = glCreateProgram();
    HB_CHECK_GL_ERROR();
}

// ---------------------------------------------------------------

HBGLShaderProgram::~HBGLShaderProgram()
{
    if (m_programLog != 0) {
        free(m_programLog);
    }

    if (m_programIndex != 0) {
        glDeleteProgram(m_programIndex);
        HB_CHECK_GL_ERROR();
    }
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::ClearShaders()
{

    // XXX: Needs to be implemented
    return true;
}

// ---------------------------------------------------------------


bool
HBGLShaderProgram::_AttachShader(HBGLShaderPtr& shader)
{
    if (!shader) {
        std::cerr << "CODING ERROR [ " << this->GetName() << " ]: expected shader to be non null if we want it to attach to this program. " << std::endl;
        return false;
    }
    
	if (!shader->IsCompiled())
    {
		if (!shader->CompileShader())
        {
            std::cerr << "HBGLShaderProgram ERROR [ " << this->GetName() << " ]: Could not compile [ " << shader->GetName() << " ] " << std::endl;
            return false;
        }
	}

    glAttachShader(m_programIndex, shader->GetShaderIndex());
    HB_CHECK_GL_ERROR();

    return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::_BindAttribLocations()
{
    if (m_programIndex==0) {
        std::cerr << "CODING ERROR [ " << this->GetName() << " ]: expected program index to be created before calling _BindAttribLocations. " << std::endl;
        return false;
    }
    
    for (HBGLBoundValuesMap::const_iterator iter = m_boundAttributesMap.begin();
        iter != m_boundAttributesMap.end();
        iter++) 
    {

        glBindAttribLocation(m_programIndex, iter->second, (iter->first).c_str());
        HB_CHECK_GL_ERROR();
    }
    
    return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::LoadAndCompileShaders(const std::string& vshFilePath, 
                                         const std::string& fshFilePath)
{
    m_vertShader = HBGLShaderPtr(new HBGLVertexShader(vshFilePath));
    m_fragShader = HBGLShaderPtr(new HBGLFragmentShader(fshFilePath));

    bool result = true;
    if (!m_vertShader->CompileShader())
    {
        std::cerr << "HBGLShaderProgram ERROR [ " << this->GetName() << " ]: Could not compile vertex shader [ " << m_vertShader->GetName() << " ] " << std::endl;
        result &= false;
    }

    if (!m_fragShader->CompileShader())
    {
        std::cerr << "HBGLShaderProgram ERROR [ " << this->GetName() << " ]: Could not compile frag shader [ " << m_fragShader->GetName() << " ] " << std::endl;
        result &= false;
    }

    return result;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::LoadAndCompileShaders(HBGLShaderPtr& vertShaderPtr,
                                         HBGLShaderPtr& fragShaderPtr)
{
    if (vertShaderPtr == nullptr || fragShaderPtr == nullptr)
    {
        std::cerr << "HBGLShaderProgram ERROR: null shaders provided to LoadAndCompileShaders" << std::endl;
        return false;
    }

    m_vertShader = vertShaderPtr;
    m_fragShader = fragShaderPtr;

    bool result = true;
    if (!m_vertShader->CompileShader())
    {
        std::cerr << "HBGLShaderProgram ERROR [ " << this->GetName() << " ]: Could not compile vertex shader [ " << m_vertShader->GetName() << " ] " << std::endl;
        result &= false;
    }

    if (!m_fragShader->CompileShader())
    {
        std::cerr << "HBGLShaderProgram ERROR [ " << this->GetName() << " ]: Could not compile frag shader [ " << m_fragShader->GetName() << " ] " << std::endl;
        result &= false;
    }

    return result;
}


// ---------------------------------------------------------------

bool
HBGLShaderProgram::LinkShaders()
{

    if ((m_vertShader == nullptr) || (m_fragShader == nullptr))
    {
        std::cerr << "HBGLShaderProgram ERROR [ " << this->GetName() << " ]: must load and compile vertex and fragment shader before loading." << std::endl;
        return false;
    }

    bool result = true;

    result &= _AttachShader(m_vertShader);
    result &= _AttachShader(m_fragShader);

    _BindAttribLocations();
    
	glLinkProgram(m_programIndex);
	HB_CHECK_GL_ERROR();
    
	GLint linkingStatus;
    glGetProgramiv(m_programIndex, GL_LINK_STATUS, &linkingStatus);
    HB_CHECK_GL_ERROR();

    std::string linkLog;
    if ((linkingStatus == GL_TRUE) & GetProgramLog(&linkLog))
    {

        m_vertShader->SetLinked(true);
        glDetachShader(m_programIndex, m_vertShader->GetShaderIndex());
        HB_CHECK_GL_ERROR();

        m_fragShader->SetLinked(true);
        glDetachShader(m_programIndex, m_fragShader->GetShaderIndex());
        HB_CHECK_GL_ERROR();
        
	} else {
        
        std::cerr << "HBGLShaderProgram ERROR [ " << this->GetName() << " ]: trying to link compiled shaders was not successful:\n" << linkLog << std::endl;

        glDetachShader(m_programIndex, m_vertShader->GetShaderIndex());
        glDeleteShader(m_vertShader->GetShaderIndex());
        HB_CHECK_GL_ERROR();

        glDetachShader(m_programIndex, m_fragShader->GetShaderIndex());
        glDeleteShader(m_fragShader->GetShaderIndex());
        HB_CHECK_GL_ERROR();

        glDeleteProgram(m_programIndex);
        HB_CHECK_GL_ERROR();
        m_programIndex = 0;

        result &= false;
    }

    return result;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::GetProgramLog(std::string* log)
{

    if (!log) {
        std::cerr << "CODING ERROR: HBGLShader::GetProgramLog was passed a NULL log" << std::endl;
        return false;
    }
    
	GLint logLength = 0;

	if (m_programIndex==0) {
        *log = "HBGLShaderProgram::GetProgramLog ERROR: Not a valid program object";
        return false;
    }

    glGetProgramiv(m_programIndex, GL_INFO_LOG_LENGTH, &logLength);
	HB_CHECK_GL_ERROR();

    if (logLength > 1) {
        if (m_programLog!=0) {
			free(m_programLog);
			m_programLog = 0;
        }

        if ((m_programLog = (GLchar*) malloc(logLength)) == NULL)
		{
            *log = "HBGLShaderProgram::LinkerLog ERROR: Out of memory";
            return false;
        }

        glGetProgramInfoLog(m_programIndex, logLength, &logLength, m_programLog);
        HB_CHECK_GL_ERROR();

	}
    
    if (m_programLog!=0) {
        *log = m_programLog;
    }

    return true;
}


// ---------------------------------------------------------------

bool
HBGLShaderProgram::ReloadLinkedShaders()
{
    if (!this->IsLinked()) {
        std::cerr << "HBGLShaderProgram ERROR [ " << this->GetName() << " ]: could not be relinked since it was not " \
        "properly linked to begin with." << std::endl;

        return false;
    }

    bool result = true;
    result &= m_vertShader->ReloadFile();
    result &= m_fragShader->ReloadFile();

    result &= this->LinkShaders();

    return result;
}

// ---------------------------------------------------------------

void
HBGLShaderProgram::ShadersBegin() const
{
    if (m_programIndex == 0) { 
        return; 
    };

    HB_CHECK_GL_ERROR();
	if (this->IsLinked()) {
		glUseProgram(m_programIndex);
		HB_CHECK_GL_ERROR();
	}
}

// ---------------------------------------------------------------

void
HBGLShaderProgram::ShadersEnd() const
{

    if (m_programIndex == 0) {
        return;
    }

    glUseProgram(0);
    HB_CHECK_GL_ERROR();

}

// ---------------------------------------------------------------

HBGLShaderPtr
HBGLShaderProgram::GetVertexShader() const
{
    return m_vertShader;
}

// ---------------------------------------------------------------

HBGLShaderPtr
HBGLShaderProgram::GetFragmentShader() const
{
    return m_fragShader;
}

// ---------------------------------------------------------------

std::string
HBGLShaderProgram::GetName() const
{
    return m_programName;
}


// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniform1f(const char* varname,
                                GLfloat v0)
{

    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }
    
	glUniform1f(index, v0);
    HB_CHECK_GL_ERROR();

	return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniform2f(const char* varname,
                                GLfloat v0,
                                GLfloat v1)
{

    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

	glUniform2f(index, v0, v1);
    HB_CHECK_GL_ERROR();

	return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniform3f(const char* varname,
                                GLfloat v0,
                                GLfloat v1,
                                GLfloat v2)
{

    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

	glUniform3f(index, v0, v1, v2);
    HB_CHECK_GL_ERROR();

	return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniform4f(const char* varname,
                                GLfloat v0,
                                GLfloat v1,
                                GLfloat v2,
                                GLfloat v3)
{

    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

	glUniform4f(index, v0, v1, v2, v3);
    HB_CHECK_GL_ERROR();

	return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniform1i(const char* varname,
                                GLint v0)
{
    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

	glUniform1i(index, v0);
    HB_CHECK_GL_ERROR();

	return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniform2i(const char* varname,
                                GLint v0,
                                GLint v1)
{

    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

	glUniform2i(index, v0, v1);
    HB_CHECK_GL_ERROR();

	return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniform3i(const char* varname,
                                GLint v0,
                                GLint v1,
                                GLint v2)
{

    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

	glUniform3i(index, v0, v1, v2);
    HB_CHECK_GL_ERROR();

	return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniform4i(const char* varname,
                                GLint v0,
                                GLint v1,
                                GLint v2,
                                GLint v3)
{
    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

	glUniform4i(index, v0, v1, v2, v3);
    HB_CHECK_GL_ERROR();

	return true;
}

// ---------------------------------------------------------------

bool 
HBGLShaderProgram::SetUniform1fv(const char* varname,
GLsizei count,
const GLfloat* value)
{

    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

    glUniform1fv(index, count, value);
    HB_CHECK_GL_ERROR();

    return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniform3fv(const char* varname,
GLsizei count,
const GLfloat* value)
{

    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

    glUniform3fv(index, count, value);
    HB_CHECK_GL_ERROR();

    return true;
}


// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniformMatrix2fv(const char* varname,
                                       GLsizei count,
                                       GLboolean transpose,
                                       const GLfloat* value)
{
    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

	glUniformMatrix2fv(index, count, transpose, value);
    HB_CHECK_GL_ERROR();

	return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniformMatrix3fv(const char* varname,
                                       GLsizei count,
                                       GLboolean transpose,
                                       const GLfloat* value)
{
    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

	glUniformMatrix3fv(index, count, transpose, value);
    HB_CHECK_GL_ERROR();

	return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::SetUniformMatrix4fv(const char* varname,
                                       GLsizei count,
                                       GLboolean transpose,
                                       const GLfloat* value)
{

    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }

	glUniformMatrix4fv(index, count, transpose, value);
    HB_CHECK_GL_ERROR();

	return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::GetUniformfv(const char* varname,
                              GLfloat* values)
{

    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }
    
	glGetUniformfv(m_programIndex, index, values);
    HB_CHECK_GL_ERROR();

    return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::GetUniformiv(const char* varname,
                                GLint* values)
{

    GLint index;
    if (!_GetUniformLocation(varname, &index)) {
        return false;
    }
    
	glGetUniformiv(m_programIndex, index, values);
    HB_CHECK_GL_ERROR();
    return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::EnableVertexAttrib(const char* varname,
                                        GLint size,
                                        GLenum type,
                                        GLboolean normalized,
                                        GLsizei stride,
                                        const GLvoid * pointer) const
{

    if (m_programIndex == 0) {
        return false;
    }
     
    HBGLBoundValuesMap::const_iterator bvIter = m_boundAttributesMap.find(varname);

    if (bvIter == m_boundAttributesMap.end()) {
        std::cerr << "Unable to enable [ " << varname << " ] since it wasn't bound before linking the shader [ " << m_programName << " ] " << std::endl;
        return false;
    }

    GLint attribIndex = bvIter->second;
    glEnableVertexAttribArray(attribIndex);
    glVertexAttribPointer(attribIndex, size, type, normalized, stride, pointer);
    HB_CHECK_GL_ERROR();

    return true;
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::DisableVertexAttrib(const char* varname) const
{
    if (m_programIndex == 0) {
        return false;
    }

    HBGLBoundValuesMap::const_iterator bvIter = m_boundAttributesMap.find(varname);

    if (bvIter == m_boundAttributesMap.end()) {
        std::cerr << "Unable to disable [ " << varname << " ] since it wasn't bound before linking the shader [ " << m_programName << " ] " << std::endl;
        return false;
    }

    GLint attribIndex = bvIter->second;
    glDisableVertexAttribArray(attribIndex);
    HB_CHECK_GL_ERROR();

    return true;
}


// ---------------------------------------------------------------
                
bool
HBGLShaderProgram::GetAttribLocation(const char* varname,
                                     GLint *index)
{
    return _GetAttribLocation(varname, index);
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::ReserveAttribLocation(const char* varname,
                                         GLint *index)
{
    return _GetAttribLocation(varname, index);
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::_GetAttribLocation(const GLchar* varname,
                                      GLint* index)
{
    if (m_programIndex == 0) {
        return false;
    }

    HBGLBoundValuesMap::const_iterator bvIter = m_boundAttributesMap.find(varname);

    if (bvIter == m_boundAttributesMap.end()) {

        m_boundAttributesMap[varname] = m_attributeIndex++;
        *index = (GLint)m_boundAttributesMap[varname];
        return true;

    }
    else 
    {

        *index = (GLint)bvIter->second;
        return true;
    }
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::GetUniformLocation(const char* varname,
                                      GLint *index)
{
    return _GetUniformLocation(varname, index);
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::_GetUniformLocation(const GLchar* varname,
                                       GLint* index)
{
    if (m_programIndex == 0) {
        return false;
    }

    HBGLBoundValuesMap::const_iterator bvIter = m_boundUniformsMap.find(varname);

    if (bvIter == m_boundUniformsMap.end()) {

        m_boundUniformsMap[varname] = glGetUniformLocation(m_programIndex, varname);
        *index = (GLint) m_boundUniformsMap[varname];

    } else {

        *index = (GLint) bvIter->second;
    }
    if (*index >= 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

// ---------------------------------------------------------------

bool
HBGLShaderProgram::IsLinked() const
{
    return ((m_vertShader && m_vertShader->IsLinked()) &&
        (m_fragShader && m_fragShader->IsLinked()));
}

// ---------------------------------------------------------------

GLuint
HBGLShaderProgram::GetProgramIndex(void) const
{
	return m_programIndex;
}

// ''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
// HBGLShader 
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,

HBGLShader::HBGLShader(const std::string& filePath) :
m_shaderName(""),
m_shaderFilePath(""),
m_shaderSource(NULL),
m_shaderIndex(0),
m_shaderLog(""),
m_isCompiled(false),
m_isLinked(false)
{
    if (!filePath.empty()) {
        LoadFile(filePath);
    }
}

// ---------------------------------------------------------------

HBGLShader::~HBGLShader()
{

    if (m_shaderSource != 0) {
        delete[] m_shaderSource;
    }

	if (m_isCompiled)
    {
        if (m_shaderIndex) {
            glDeleteShader(m_shaderIndex);
        }
		HB_CHECK_GL_ERROR();
	}
}

// ---------------------------------------------------------------

bool
HBGLShader::LoadFile(const std::string& filePath)
{

    m_shaderName = filePath;
    std::string realFilePath = HBGLUtils::GetRealFilePath(filePath.c_str());

    FILE * shaderFile;
    shaderFile = fopen(realFilePath.c_str(), "r");

    if (!shaderFile) {
        std::cerr << "STVRFragmentShader ERROR [ " << this->GetName() << " ]: Could not load file [ " << filePath << " ] " << std::endl;
        return false;
    }

    if (m_shaderSource != 0) // there is already a source loaded, free it!
    {
        delete[] m_shaderSource;
    }

    unsigned long bufferSize = HBGLShader::GetFileEndPosition(shaderFile);

    // We are allocating more space then we will need based on overall file length
    m_shaderSource = (GLchar*) new char[bufferSize + 1];

    if (m_shaderSource == 0) {
        std::cerr << "HBGLShader ERROR [ " << this->GetName() << " ]: Unable to allocate memory to keep shader source in memory [ " << realFilePath << " ] " << std::endl;
        return false;
    }

    int fileError = 0;
    unsigned long shaderCharIdx = 0;
    while (!feof(shaderFile) && !(fileError = ferror(shaderFile)))
    {
        char currChar = fgetc(shaderFile);
        if (currChar != EOF) {
            m_shaderSource[shaderCharIdx++] = currChar;
        }
    }

    // 0 terminate the shader source
    m_shaderSource[shaderCharIdx] = 0;

    if (fileError > 0) {
        std::cerr << "HBGLShader ERROR [ " << this->GetName() << " ]: parsing ShaderToy fragment shader [ " << realFilePath << " ] Error: " << fileError << std::endl;
        // the shader was invalid, make it an empty string
        m_shaderSource[0] = 0;
        return false;
    }

    fclose(shaderFile);

    m_shaderFilePath = realFilePath;

	return true;
}

// ---------------------------------------------------------------

bool
HBGLShader::ReloadFile(void)
{

    if (!this->m_shaderFilePath.empty()) {
        bool loadresult = this->LoadFile(this->m_shaderFilePath);
        if (loadresult) {
            this->m_isCompiled = false;
            this->m_isLinked = false;
            
            return true;
        }
    }

    return false;

}


// ---------------------------------------------------------------

bool
HBGLShader::LoadSource(const std::string& programSource)
{
    if (m_shaderSource != 0)    // there is already a source loaded, free it!
    {
        delete[] m_shaderSource;
    }

    int srcLength = programSource.length();
    m_shaderSource = (GLchar*) new char[srcLength + 1];
    memcpy(m_shaderSource, programSource.c_str(), srcLength + 1);

    m_shaderSource[srcLength] = 0;
    return true;
}

// ---------------------------------------------------------------

const std::string
HBGLShader::GetName() const {
    return m_shaderName;
}

// ---------------------------------------------------------------

bool
HBGLShader::IsCompiled() const {
    return m_isCompiled;
}

// ---------------------------------------------------------------

bool
HBGLShader::IsLinked() const {
    return m_isLinked;
}

// ---------------------------------------------------------------

bool
HBGLShader::SetLinked(bool val) {
    bool changeOfState = (m_isLinked != val);
    m_isLinked = val;
    return changeOfState;
}

// ---------------------------------------------------------------

GLuint
HBGLShader::GetShaderIndex() const {
    return m_shaderIndex;
}

// ---------------------------------------------------------------

bool
HBGLShader::GetShaderLog(std::string* log)
{
    if (!log) {
        std::cerr << "CODING ERROR: HBGLShader::GetShaderLog was passed a NULL log" << std::endl;
        return false;
    }
    
	GLint logLength = 0;

	if (m_shaderIndex==0) {
        *log = "HBGLShader ERROR: Not a valid program object";
        return false;
    }
    
    glGetShaderiv(m_shaderIndex, GL_INFO_LOG_LENGTH, &logLength);
	HB_CHECK_GL_ERROR();

    GLchar* shaderLog = NULL;
    if (logLength > 1)
	{
        if ((shaderLog = (GLchar*)malloc(logLength)) == NULL)
		{

			*log = "HBGLShader ERROR: Out of memory";
            return false;
        }

        glGetShaderInfoLog(m_shaderIndex, logLength, &logLength, shaderLog);
		HB_CHECK_GL_ERROR();
	}

    if (shaderLog) {
        m_shaderLog = std::string(shaderLog);
        *log = m_shaderLog;
    }
    
    return true;
}

// ---------------------------------------------------------------

bool
HBGLShader::CompileShader(void)
{
	m_isCompiled = false;
    std::string shaderLog;

    if (!m_shaderSource) {
        std::cerr << "HBGLShader ERROR [ " << this->GetName() << " ]: Could not compile shader because there is no valid source code." << std::endl;
        return false;
    }

    glShaderSource(m_shaderIndex, 1, (const GLchar **) &m_shaderSource, NULL);
    HB_CHECK_GL_ERROR();

#if defined(HBGLSHADERS_DEBUG_INPUTSHADER)
    std::cout << m_shaderSource << std::endl;
#endif

    glCompileShader(m_shaderIndex);
    HB_CHECK_GL_ERROR();
    
    GLint compileStatus;
    glGetShaderiv(m_shaderIndex, GL_COMPILE_STATUS, &compileStatus);
    HB_CHECK_GL_ERROR();

    if ((compileStatus == GL_TRUE) & GetShaderLog(&shaderLog)) {

        m_isCompiled=true;

    } else {
        std::cerr << "HBGLShader ERROR [ " << this->GetName() << " : " << this->m_shaderFilePath << " : " << compileStatus << " ]:\n" << shaderLog << std::endl;
        glDeleteShader(m_shaderIndex);
        return false;
    }
    
    return (compileStatus > 0);
}


// ----------------------------------------------------------------------------


HBGLVertexShader::HBGLVertexShader(const std::string& filePath) : HBGLShader(filePath)
{

    m_shaderIndex = glCreateShader(GL_VERTEX_SHADER);
    HB_CHECK_GL_ERROR();
}

// ----------------------------------------------------------------------------

HBGLVertexShader::~HBGLVertexShader()
{
}

// ----------------------------------------------------------------------------

HBGLFragmentShader::HBGLFragmentShader(const std::string& filePath) : HBGLShader(filePath)
{
    m_shaderIndex = glCreateShader(GL_FRAGMENT_SHADER);
    HB_CHECK_GL_ERROR();
}

// ----------------------------------------------------------------------------

HBGLFragmentShader::~HBGLFragmentShader()
{
}
