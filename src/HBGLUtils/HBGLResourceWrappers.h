#pragma once

#include <memory>

#include <GL/glew.h>

namespace HBGLUtils
{
    // A simple pure virtual parent class that generates and holds onto a 
    // gl index object. It's up to sub-classes to implement the generation 
    // and clean of any gl resource.  This implementation avoids encapsulation
    // other gl features and is just the holder of the resource index.
    // Shared pointers will be used to make sure all cleanup happens automatically. 

    class HBGLResource
    {
    public:

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // CONSTRO/DESTRO

        HBGLResource();
        virtual ~HBGLResource();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // GENERATOR

        virtual GLuint Generate() = 0;

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // ACCESSORS

        GLuint GetIndex() const;
        bool IsGenerated() const;

    protected:

        GLuint m_glIndex;

    };

    // ===========================================================================

    class HBGLTextureResource : public HBGLResource
    {
    public:

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // CONSTRO/DESTRO

        HBGLTextureResource();
        ~HBGLTextureResource();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // GENERATOR

        GLuint Generate() override;

    };

    typedef std::shared_ptr<HBGLTextureResource> HBGLTextureResourcePtr;

    // ===========================================================================

    class HBGLBufferResource : public HBGLResource
    {
    public:

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // CONSTRO/DESTRO

        HBGLBufferResource();
        ~HBGLBufferResource();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // GENERATOR

        GLuint Generate() override;

    };

    typedef std::shared_ptr<HBGLBufferResource> HBGLBufferResourcePtr;

    // ===========================================================================

    class HBGLFrameBufferResource : public HBGLResource
    {
    public:

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // CONSTRO/DESTRO

        HBGLFrameBufferResource();
        ~HBGLFrameBufferResource();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // GENERATOR

        GLuint Generate() override;

    };

    typedef std::shared_ptr<HBGLFrameBufferResource> HBGLFrameBufferResourcePtr;


    // ===========================================================================

    class HBGLRenderBufferResource : public HBGLResource
    {
    public:

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // CONSTRO/DESTRO

        HBGLRenderBufferResource();
        ~HBGLRenderBufferResource();

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
        // GENERATOR

        GLuint Generate() override;

    };

    typedef std::shared_ptr<HBGLRenderBufferResource> HBGLRenderBufferResourcePtr;


}