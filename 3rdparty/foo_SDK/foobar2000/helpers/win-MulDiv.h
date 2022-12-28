#pragma once

#ifndef _WIN32
inline int MulDiv(int v1, int v2, int v3) {
    return pfc::rint32((double) v1 * (double) v2 / (double) v3);
}
#endif
