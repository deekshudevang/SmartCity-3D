#pragma once

#include <vector>
#include <string>
#include <cmath>

namespace SmartCity::algos {

struct PixelStep {
    int x{ 0 };
    int y{ 0 };
    float ddaX{ 0.0f };
    float ddaY{ 0.0f };
    int decisionVar{ 0 };   // For Bresenham
    std::string stepDesc;
};

struct LineRasterResult {
    std::vector<PixelStep> pixels;
    int x0{ 0 };
    int y0{ 0 };
    int x1{ 0 };
    int y1{ 0 };
    int dx{ 0 };
    int dy{ 0 };
    int octant{ 0 };
    float slope{ 0.0f };
    bool isVertical{ false };
    bool isHorizontal{ false };
    bool isDegenerate{ false };
};

class LineRasterizer {
public:
    // Digital Differential Analyzer (DDA) rasterization
    static LineRasterResult RasterizeDDA(int x0, int y0, int x1, int y1);

    // Bresenham Integer-Only line rasterization (all octants)
    static LineRasterResult RasterizeBresenham(int x0, int y0, int x1, int y1);

    // Utility: determine octant index [0..7]
    static int GetOctant(int dx, int dy);
};

} // namespace SmartCity::algos

