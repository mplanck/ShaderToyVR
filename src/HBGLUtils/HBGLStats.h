//
//  HBGLStats
//
//  Created by Maxwell Planck on 9/3/12.
//  Copyright (c) 2012 Maxwell Planck. All rights reserved.
//

#pragma once

#include <iostream>
#include <memory>
#include <map>

#include <GL/glew.h>

namespace HBGLUtils
{

    typedef unsigned int uint;
    typedef std::map<std::string, float> OverlayStatsMap;
    typedef std::map<std::string, int> OverlayStatsPrecisionMap;

    class HBGLOverlayStats {

    public:
        HBGLOverlayStats();
        ~HBGLOverlayStats();

        bool AddDataKey(const std::string& key,
            float defValue,
            int precision = -1);

        bool UpdateData(const std::string& key,
            float value);

        void DrawOverlay(GLsizei width, GLsizei height);

    private:

        void _SetOrthographicProjection(GLsizei width, GLsizei height);

        void _RenderBitmapString(float x,
            float y,
            const std::string& str,
            float spacing = 0);

        void _ResetPerspectiveProjection();

        OverlayStatsMap          _dataMap;
        OverlayStatsPrecisionMap _dataPrecisionMap;

    };

    typedef std::shared_ptr<HBGLOverlayStats> HBGLOverlayStatsPtr;
}