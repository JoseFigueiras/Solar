#include "solar.h"
#include "vec2.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <vector>

// 
struct CelestialBody {
    std::string displayName;

};

int main()
{
    // init
    auto prevFrameRenderData;

    while (1)
    {
        // send render command to gpu, start doing cpu work for the next frame without waiting on gpu
        render(prevFrameRenderData);
        updatePhysics();
    }
    return 0;
}
