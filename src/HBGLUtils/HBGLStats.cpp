
#include "HBGLStats.h"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <sstream>

using namespace HBGLUtils;

// ******************************************************************
// CONSTRUCTOR FUNCTIONS
// ******************************************************************

HBGLOverlayStats::HBGLOverlayStats() {
}

HBGLOverlayStats::~HBGLOverlayStats() {
    _dataMap.clear();
    _dataPrecisionMap.clear();
}

// ******************************************************************
// UTILITY FUNCTIONS
// ******************************************************************

bool
HBGLOverlayStats::AddDataKey(const std::string &key,
                                float defValue,
                                int precision)
{
    OverlayStatsMap::const_iterator dataIter = _dataMap.find(key);
    bool addingNewKey = (dataIter == _dataMap.end());

    _dataMap[key] = defValue;
    _dataPrecisionMap[key] = precision;

    return addingNewKey;
}

bool
HBGLOverlayStats::UpdateData(const std::string &key, float value)
{
    OverlayStatsMap::const_iterator dataIter = _dataMap.find(key);

    if (dataIter != _dataMap.end()) {
        _dataMap[key] = value;
        return true;
    }

    return false;
}

void
HBGLOverlayStats::DrawOverlay(GLsizei width, GLsizei height)
{

	_SetOrthographicProjection(width, height);

	glPushMatrix();
	glLoadIdentity();
    OverlayStatsMap::const_iterator dataIter;

	glColor3f(.47f, .84f, .95f);
    float vertOffset = 15.f;
    for (dataIter = _dataMap.begin();
         dataIter != _dataMap.end();
         dataIter++) {

        std::stringstream sstr;
        OverlayStatsPrecisionMap::const_iterator dataPrecIter = \
        _dataPrecisionMap.find(dataIter->first);

        if (dataPrecIter->second > 0) {
            sstr.precision(dataPrecIter->second);
        }

        sstr << dataIter->first << ": " << dataIter->second;

        _RenderBitmapString(10.0f, vertOffset,sstr.str());
        vertOffset += 15.f;
        std::cout << "\r" << sstr.str();
        sstr.clear();
    }

	glPopMatrix();

	_ResetPerspectiveProjection();
}

// ******************************************************************
// INTERNAL UTILITY FUNCTIONS
// ******************************************************************

void
HBGLOverlayStats::_SetOrthographicProjection(GLsizei width, GLsizei height)
{

	glMatrixMode(GL_PROJECTION);

	glPushMatrix();

	glLoadIdentity();
     // set a 2D orthographic projection
	glOrtho(0, width, 0, height, 1, 10000);

     // invert the y axis, down is positive
	glScalef(1.f, -1.f, 1.f);

    // move the origin from the bottom left corner
    // to the upper left corner
    glTranslatef(0.f, (float) -height * .5f, 0.f);

	glMatrixMode(GL_MODELVIEW);

}

void
HBGLOverlayStats::_RenderBitmapString(float x,
                                      float y,
                                      const std::string& str,
                                      float spacing)
{
    // TODO: Write a simple monospace texture based printing system
    // without pulling in dependencies like FTP

}

void
HBGLOverlayStats::_ResetPerspectiveProjection()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}