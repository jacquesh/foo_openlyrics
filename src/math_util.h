#pragma once

#include "stdafx.h"

inline double lerp(double x, double y, double t)
{
    return x + (y-x)*t;
}

inline t_ui_color lerp(t_ui_color x, t_ui_color y, double t)
{
    double r = lerp((double)GetRValue(x), (double)GetRValue(y), t);
    double g = lerp((double)GetGValue(x), (double)GetGValue(y), t);
    double b = lerp((double)GetBValue(x), (double)GetBValue(y), t);
    return RGB((int)r, (int)g, (int)b);
}

inline double lerp_inverse_clamped(double x, double y, double t)
{
    if(x == y) return 0.0;

    t = (t < x) ? x : ((t > y) ? y : t);
    return (t-x)/(y-x);
}

