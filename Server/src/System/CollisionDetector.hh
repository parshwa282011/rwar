#pragma once

#include <SpatialHash.hh>

namespace app
{
    class Simulation;
}

namespace app::system
{
    class CollisionDetector
    {
    public:
        Simulation &m_Simulation;
        SpatialHash m_SpatialHash;
        CollisionDetector(Simulation &);

        void Tick();
    };
}
