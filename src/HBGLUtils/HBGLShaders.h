#pragma once

#include <memory>
#include <map>
#include <vector>
#include <iostream>

#include <GL/glew.h>

namespace HBGLUtils
{

    typedef std::map<std::string, GLint> HBGLBoundValuesMap;

    //-----------------------------------------------------------------------------
    // Shader class that defines a generic GL shader.  Subclasses include a vertex
    // and fragment shader class.  This is responsible for loading source from a 
    // file or string, messaging to the gl system about compilation, and reflecting
    // and messaging about GL state.

    class  HBGLShader
    {
    public:

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // PUBLIC STATIC

        static unsigned long GetFileEndPosition(FILE * file);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // CONSTRO/DESTRO

        HBGLShader(const std::string& filePath);
        virtual ~HBGLShader();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // ACCESSORS
        const std::string GetName() const;
        bool IsCompiled(void) const;
        bool IsLinked(void) const;
        GLuint GetShaderIndex(void) const;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // CACHED ACCESSORS

        bool GetShaderLog(std::string* log = NULL);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // MODIFIERS

        virtual bool LoadFile(const std::string& filePath);
        virtual bool LoadSource(const std::string& programSource);

        bool ReloadFile(void);

        bool CompileShader(void);
        bool SetLinked(bool val);

    protected:

        std::string         m_shaderName;
        std::string         m_shaderFilePath;
        GLchar*             m_shaderSource;

        GLuint              m_shaderIndex;

        std::string         m_shaderLog;

        bool                m_isCompiled;
        bool                m_isLinked;

    };

    typedef std::shared_ptr<HBGLShader> HBGLShaderPtr;

    //-----------------------------------------------------------------------------

    class  HBGLVertexShader : public HBGLShader
    {
    public:
        HBGLVertexShader(const std::string& filePath);
        virtual ~HBGLVertexShader();
    };

    //-----------------------------------------------------------------------------

    class  HBGLFragmentShader : public HBGLShader
    {
    public:
        HBGLFragmentShader(const std::string& filePath);
        virtual ~HBGLFragmentShader();

    };

    //-----------------------------------------------------------------------------

    class  HBGLShaderProgram
    {
    public:
        HBGLShaderProgram(const char* name);
        virtual ~HBGLShaderProgram();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // ACCESSORS

        GLuint GetProgramIndex() const;
        std::string GetName() const;
        void ShadersBegin() const;
        void ShadersEnd() const;
        bool IsLinked() const;

        HBGLShaderPtr GetVertexShader() const;
        HBGLShaderPtr GetFragmentShader() const;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // CACHED ACCESSORS

        bool GetProgramLog(std::string* log = NULL);

        bool GetUniformfv(const char* name,
            GLfloat* values);

        bool GetUniformiv(const char* name,
            GLint* values);

        bool GetAttribLocation(const char* varname,
            GLint* index);

        bool GetUniformLocation(const char* varname,
            GLint* index);

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // MODIFIERS

        bool LoadAndCompileShaders(const std::string& vshFilePath,
            const std::string& fshFilePath);

        bool LoadAndCompileShaders(HBGLShaderPtr& vertShaderPtr,
            HBGLShaderPtr& fragShaderPtr);

        bool ReserveAttribLocation(const char* varname,
            GLint* index);

        bool LinkShaders();
        bool ReloadLinkedShaders();

        bool ClearShaders();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // Client Messaging

        bool SetUniform1f(const char* varname,
            GLfloat v0);

        bool SetUniform2f(const char* varname,
            GLfloat v0,
            GLfloat v1);

        bool SetUniform3f(const char* varname,
            GLfloat v0,
            GLfloat v1,
            GLfloat v2);

        bool SetUniform4f(const char* varname,
            GLfloat v0,
            GLfloat v1,
            GLfloat v2,
            GLfloat v3);

        bool SetUniform1i(const char* varname,
            GLint v0);

        bool SetUniform2i(const char* varname,
            GLint v0,
            GLint v1);

        bool SetUniform3i(const char* varname,
            GLint v0,
            GLint v1,
            GLint v2);

        bool SetUniform4i(const char* varname,
            GLint v0,
            GLint v1,
            GLint v2,
            GLint v3);

        bool SetUniform1fv(const char* varname,
            GLsizei count,
            const GLfloat* value);

        bool SetUniform3fv(const char* varname,
            GLsizei count,
            const GLfloat* value);

        bool SetUniformMatrix2fv(const char* varname,
            GLsizei count,
            GLboolean transpose,
            const GLfloat* value);

        bool SetUniformMatrix3fv(const char* varname,
            GLsizei count,
            GLboolean transpose,
            const GLfloat* value);

        bool SetUniformMatrix4fv(const char* varname,
            GLsizei count,
            GLboolean transpose,
            const GLfloat* value);

        // Vertex Attributes

        bool EnableVertexAttrib(const char* varname,
            GLint size,
            GLenum type,
            GLboolean normalized,
            GLsizei stride,
            const GLvoid * pointer) const;

        bool DisableVertexAttrib(const char* varname) const;

    private:

        bool _GetAttribLocation(const GLchar* name,
            GLint* index);

        bool _GetUniformLocation(const GLchar* name,
            GLint* index);

        bool _AttachShader(HBGLShaderPtr& shader);

        bool _BindAttribLocations();

        typedef std::map<std::string, GLint> HBShaderValueIndexMap;

        GLuint                            m_programIndex;
        std::string                       m_programName;
        GLuint                            m_attributeIndex;

        GLchar*                           m_programLog;
        HBGLShaderPtr                     m_vertShader;
        HBGLShaderPtr                     m_fragShader;
        HBGLBoundValuesMap                m_boundAttributesMap;
        HBGLBoundValuesMap                m_boundUniformsMap;

    };

    typedef std::shared_ptr<HBGLShaderProgram> HBGLShaderProgramPtr;

}