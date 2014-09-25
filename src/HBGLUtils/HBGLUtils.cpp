#include "HBGLUtils.h"

#include <iostream>

#if defined(_WIN32)
#include <Windows.h>
#include <WinBase.h>
#endif

//-----------------------------------------------------------------------------

const char*
HBGLUtils::ParseGLError(GLenum glErr)
{
    switch (glErr) {
        case GL_INVALID_ENUM:
            return "GLERROR Invalid Enum";
        case GL_INVALID_VALUE:
            return "GLERROR Invalid Value";
        case GL_INVALID_OPERATION:
            return "GLERROR Invalid Operation";
        case GL_OUT_OF_MEMORY:
            return "GLERROR Out of Memory";
        default:
            return "GLERROR Unknown";
    }
}

//-----------------------------------------------------------------------------

bool
HBGLUtils::CheckGLError(const char *file, int line)
{
	GLenum glErr;
	bool    result = true;

	glErr = glGetError();
	if (glErr != GL_NO_ERROR)
	{
        result = false;
        //TODO: create a class that logs errors and only prints out "spamming" errors every 100 or so to avoid stdout spam
        //std::cerr << "GL Error #" << glErr << " ( " << ParseGLError(glErr) << " ) in File " << file << " at line: " << line << std::endl;
	}
	return result;
}

//-----------------------------------------------------------------------------

std::string
HBGLUtils::GetRealFilePath(const char *shortFilePath)
{

#if defined(_WIN32)

    char fullPath[1024];
    char *filePart = 0;
	if (GetFullPathName(shortFilePath, 1024,
        fullPath, &filePart) > 0)
	{
        return std::string(fullPath);
	}
	else
	{
		return "";
	}

#else
	// Mac & Linux implementation
	char realfilename[1024];
    realpath(shortFilePath, fullPath);
    return std::string(fullPath);

#endif
}

//-----------------------------------------------------------------------------
