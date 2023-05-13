#include <Server/System/Velocity.hh>

#include <Server/Simulation.hh>
#include <Server/Vector.hh>

namespace app::system
{
    Velocity::Velocity(Simulation &simulation)
        : m_Simulation(simulation)
    {
    }

    void Velocity::Tick()
    {
        m_Simulation.ForEachEntity([&](Entity entity)
        {
            if (!m_Simulation.HasComponent<component::Physical>(entity))
                return;

            component::Physical &physical = m_Simulation.Get<component::Physical>(entity);
            if (physical.DeletionTick() != 0)
                return;
            
            Vector positionVector{physical.X(), physical.Y()};

            physical.m_Velocity *= physical.m_Friction;
            physical.m_Velocity += physical.m_Acceleration;
            physical.m_Acceleration.Set(0, 0);
            positionVector += physical.m_Velocity;

            physical.X(positionVector.m_X);
            physical.Y(positionVector.m_Y); 
        });
    }

    void Velocity::PostTick()
    {
    }
}
