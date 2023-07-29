#include <Client/Renderer/ComponentRender.h>

#include <math.h>

#include <Client/Game.h>
#include <Client/Renderer/RenderFunctions.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>

void rr_component_drop_render(EntityIdx entity, struct rr_game *game)
{
    struct rr_simulation *simulation = game->simulation;
    struct rr_renderer *renderer = game->renderer;
    struct rr_component_physical *physical =
        rr_simulation_get_physical(simulation, entity);
    struct rr_component_drop *drop = rr_simulation_get_drop(simulation, entity);
    float animation_tick = drop->hidden ? 0.2 * physical->lerp_server_animation_tick : 
    physical->lerp_server_animation_tick == 0 ? 0 : 1 - 0.2 * physical->lerp_server_animation_tick;
    if (animation_tick > 0.99 || drop->hidden == 2)
        return;
    if (physical->lerp_server_animation_tick > 0)
    {
        if (drop->hidden)
        {
            struct rr_component_player_info *player_info = game->player_info;
            rr_renderer_translate(
                renderer,
                (player_info->lerp_camera_x - physical->lerp_x) * animation_tick,
                (player_info->lerp_camera_y - physical->lerp_y) * animation_tick);            
        }
        rr_renderer_scale(renderer, 1 - animation_tick);
        rr_renderer_rotate(renderer, M_PI * 2 * animation_tick);
    }
    rr_renderer_rotate(renderer,
                       physical->lerp_angle + physical->lerp_radius * 0.25);
    rr_renderer_scale(renderer, physical->lerp_radius * 0.04);
    rr_renderer_scale(renderer, 1 + sinf(physical->animation * 3) * 0.05);
    rr_renderer_set_fill(renderer, RR_RARITY_COLORS[drop->rarity]);
    renderer->state.filter.amount = 0.2;
    rr_renderer_set_stroke(renderer, RR_RARITY_COLORS[drop->rarity]);
    rr_renderer_set_line_width(renderer, 5);
    rr_renderer_set_line_cap(renderer, 1);
    rr_renderer_set_line_join(renderer, 1);
    rr_renderer_begin_path(renderer);
    rr_renderer_fill_rect(renderer, -30, -30, 60, 60);
    rr_renderer_stroke_rect(renderer, -30, -30, 60, 60);
    renderer->state.filter.amount = 0;
    rr_renderer_render_petal_with_background(renderer, game, drop->id, drop->rarity);
    // rr_renderer_render_static_petal(renderer, drop->id, drop->rarity);
}