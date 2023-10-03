#include <Client/Renderer/ComponentRender.h>

#include <math.h>

#include <Client/Game.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>

#include <Client/Assets/RenderFunctions.h>

void rr_component_petal_render(EntityIdx entity, struct rr_game *game, struct rr_simulation *simulation)
{
    struct rr_renderer *renderer = game->renderer;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, entity);
    struct rr_component_petal *petal =
        rr_simulation_get_petal(simulation, entity);
    struct rr_component_health *health = rr_simulation_get_health(simulation, entity);
    rr_renderer_add_color_filter(renderer, 0xffff0000, 0.5 * health->damage_animation);
    rr_renderer_set_global_alpha(renderer, 1 - physical->deletion_animation);
    rr_renderer_scale(renderer, 1 + physical->deletion_animation * 0.5);
    rr_renderer_rotate(renderer, physical->lerp_angle);
    
    if (rr_simulation_get_relations(simulation, entity)->team == rr_simulation_team_id_mobs)
        rr_renderer_scale(renderer, RR_MOB_RARITY_SCALING[petal->rarity].radius);

    uint8_t use_cache = ((health->damage_animation < 0.1) | game->cache.low_performance_mode) & 1;
    if (petal->id != rr_petal_id_peas || petal->detached == 1)
        rr_renderer_draw_petal(renderer, petal->id, use_cache);
    else
        rr_renderer_draw_static_petal(renderer, petal->id, petal->rarity, use_cache);
}