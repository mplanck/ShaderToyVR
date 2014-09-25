#include "HBGLResourceWrappers.h"
#include "HBGLUtils.h"

using namespace HBGLUtils;

//-----------------------------------------------------------------------------

HBGLResource::HBGLResource() : m_glIndex(0)
{

}

//-----------------------------------------------------------------------------

HBGLResource::~HBGLResource()
{

}

//-----------------------------------------------------------------------------

GLuint 
HBGLResource::GetIndex() const
{
    return m_glIndex;
}

//-----------------------------------------------------------------------------

bool
HBGLResource::IsGenerated() const
{
    return m_glIndex > 0;
}


//-----------------------------------------------------------------------------

HBGLTextureResource::HBGLTextureResource() : HBGLResource()
{
}

//-----------------------------------------------------------------------------

GLuint
HBGLTextureResource::Generate()
{
    if (m_glIndex == 0)
    {
        glGenTextures(1, &m_glIndex);
        HB_CHECK_GL_ERROR();
    }
    return m_glIndex;
}

//-----------------------------------------------------------------------------

HBGLTextureResource::~HBGLTextureResource()
{
    if (m_glIndex > 0) {
        glDeleteTextures(1, &m_glIndex);
    }
}

//-----------------------------------------------------------------------------

HBGLBufferResource::HBGLBufferResource() : HBGLResource()
{
}


//-----------------------------------------------------------------------------

GLuint
HBGLBufferResource::Generate()
{
    if (m_glIndex == 0)
    {
        glGenBuffers(1, &m_glIndex);
        HB_CHECK_GL_ERROR();
    }
    return m_glIndex;
}

//-----------------------------------------------------------------------------

HBGLBufferResource::~HBGLBufferResource()
{
    if (m_glIndex > 0) {
        glDeleteBuffers(1, &m_glIndex);
    }
}

//-----------------------------------------------------------------------------

HBGLFrameBufferResource::HBGLFrameBufferResource() : HBGLResource()
{
}


//-----------------------------------------------------------------------------

GLuint
HBGLFrameBufferResource::Generate()
{
    if (m_glIndex == 0)
    {
        glGenFramebuffers(1, &m_glIndex);
        HB_CHECK_GL_ERROR();
    }
    return m_glIndex;
}

//-----------------------------------------------------------------------------

HBGLFrameBufferResource::~HBGLFrameBufferResource()
{
    if (m_glIndex > 0) {
        glDeleteFramebuffers(1, &m_glIndex);
    }
}

//-----------------------------------------------------------------------------

HBGLRenderBufferResource::HBGLRenderBufferResource() : HBGLResource()
{
}

//-----------------------------------------------------------------------------

GLuint
HBGLRenderBufferResource::Generate()
{
    if (m_glIndex == 0)
    {
        glGenRenderbuffers(1, &m_glIndex);
        HB_CHECK_GL_ERROR();
    }
    return m_glIndex;
}

//-----------------------------------------------------------------------------

HBGLRenderBufferResource::~HBGLRenderBufferResource()
{
    if (m_glIndex > 0) {
        glDeleteRenderbuffers(1, &m_glIndex);
    }
}
