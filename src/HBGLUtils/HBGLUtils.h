#pragma once

#include <string>

#include <GL/glew.h>

#ifdef _DEBUG
#define HB_CHECK_GL_ERROR() HBGLUtils::CheckGLError(__FILE__, __LINE__)
#else
#define HB_CHECK_GL_ERROR()
#endif 

namespace HBGLUtils
{
    // Helper function for giving words to those pesky enums.
	const char*
	ParseGLError(GLenum glErr);

	// Check for any gl errors at the given file and line.  This is wrapped by
    // a preprocessor that helps in error reporting.

    // TODO: Turn this function call into a class that keeps track of which
    // errors occur so it can manage spam.
	bool
	CheckGLError(const char *file, int line);

    // Return a fully realized file path given a short file path.  This relies
    // on the native OS to resolve relative paths in the way it sees fit.
	std::string
	GetRealFilePath(const char* shortFilePath);
}