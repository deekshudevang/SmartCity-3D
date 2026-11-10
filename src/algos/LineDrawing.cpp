#include "algos/LineDrawing.h"
#include <cstdlib>
#include <sstream>

namespace SmartCity::algos {

int LineRasterizer::GetOctant(int dx, int dy) {
    if (dx >= 0) {
        if (dy >= 0) {
            return (std::abs(dx) >= std::abs(dy)) ? 0 : 1;
        } else {
            return (std::abs(dx) >= std::abs(dy)) ? 7 : 6;
        }
    } else {
        if (dy >= 0) {
            return (std::abs(dx) >= std::abs(dy)) ? 3 : 2;
        } else {
            return (std::abs(dx) >= std::abs(dy)) ? 4 : 5;
        }
    }
}

LineRasterResult LineRasterizer::RasterizeDDA(int x0, int y0, int x1, int y1) {
    LineRasterResult res;
    res.x0 = x0; res.y0 = y0;
    res.x1 = x1; res.y1 = y1;
    res.dx = x1 - x0;
    res.dy = y1 - y0;
    res.octant = GetOctant(res.dx, res.dy);
    res.isHorizontal = (res.dy == 0);
    res.isVertical = (res.dx == 0);
    res.isDegenerate = (res.dx == 0 && res.dy == 0);

    if (res.dx != 0) {
        res.slope = static_cast<float>(res.dy) / static_cast<float>(res.dx);
    } else {
        res.slope = (res.dy > 0) ? INFINITY : ((res.dy < 0) ? -INFINITY : 0.0f);
    }

    if (res.isDegenerate) {
        PixelStep step;
        step.x = x0;
        step.y = y0;
        step.ddaX = static_cast<float>(x0);
        step.ddaY = static_cast<float>(y0);
        step.stepDesc = "Degenerate point (P0 == P1)";
        res.pixels.push_back(step);
        return res;
    }

    int steps = std::max(std::abs(res.dx), std::abs(res.dy));
    float xInc = static_cast<float>(res.dx) / static_cast<float>(steps);
    float yInc = static_cast<float>(res.dy) / static_cast<float>(steps);

    float currentX = static_cast<float>(x0);
    float currentY = static_cast<float>(y0);

    for (int i = 0; i <= steps; ++i) {
        PixelStep step;
        step.ddaX = currentX;
        step.ddaY = currentY;
        step.x = static_cast<int>(std::round(currentX));
        step.y = static_cast<int>(std::round(currentY));

        std::ostringstream ss;
        ss << "Step " << i << ": (" << currentX << ", " << currentY << ") -> [" << step.x << ", " << step.y << "]";
        step.stepDesc = ss.str();

        res.pixels.push_back(step);

        currentX += xInc;
        currentY += yInc;
    }

    return res;
}

LineRasterResult LineRasterizer::RasterizeBresenham(int x0, int y0, int x1, int y1) {
    LineRasterResult res;
    res.x0 = x0; res.y0 = y0;
    res.x1 = x1; res.y1 = y1;
    res.dx = x1 - x0;
    res.dy = y1 - y0;
    res.octant = GetOctant(res.dx, res.dy);
    res.isHorizontal = (res.dy == 0);
    res.isVertical = (res.dx == 0);
    res.isDegenerate = (res.dx == 0 && res.dy == 0);

    if (res.dx != 0) {
        res.slope = static_cast<float>(res.dy) / static_cast<float>(res.dx);
    } else {
        res.slope = (res.dy > 0) ? INFINITY : ((res.dy < 0) ? -INFINITY : 0.0f);
    }

    if (res.isDegenerate) {
        PixelStep step;
        step.x = x0;
        step.y = y0;
        step.decisionVar = 0;
        step.stepDesc = "Degenerate point (P0 == P1)";
        res.pixels.push_back(step);
        return res;
    }

    int absDx = std::abs(res.dx);
    int absDy = std::abs(res.dy);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int currX = x0;
    int currY = y0;

    if (absDx >= absDy) {
        // Shallow slope (|m| <= 1): Primary step along X
        int D = 2 * absDy - absDx;
        for (int i = 0; i <= absDx; ++i) {
            PixelStep step;
            step.x = currX;
            step.y = currY;
            step.decisionVar = D;

            std::ostringstream ss;
            ss << "Step " << i << ": [" << currX << ", " << currY << "], D=" << D;
            step.stepDesc = ss.str();
            res.pixels.push_back(step);

            if (currX == x1 && currY == y1) break;

            if (D > 0) {
                currY += sy;
                D += 2 * (absDy - absDx);
            } else {
                D += 2 * absDy;
            }
            currX += sx;
        }
    } else {
        // Steep slope (|m| > 1): Primary step along Y
        int D = 2 * absDx - absDy;
        for (int i = 0; i <= absDy; ++i) {
            PixelStep step;
            step.x = currX;
            step.y = currY;
            step.decisionVar = D;

            std::ostringstream ss;
            ss << "Step " << i << ": [" << currX << ", " << currY << "], D=" << D;
            step.stepDesc = ss.str();
            res.pixels.push_back(step);

            if (currX == x1 && currY == y1) break;

            if (D > 0) {
                currX += sx;
                D += 2 * (absDx - absDy);
            } else {
                D += 2 * absDx;
            }
            currY += sy;
        }
    }

    return res;
}

} // namespace SmartCity::algos

