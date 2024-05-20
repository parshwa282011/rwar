#include <Client/Assets/Render.h>

#include <Client/Renderer/Renderer.h>

void rr_stone_draw(struct rr_renderer *renderer)
{
    rr_renderer_set_fill(renderer, 0x464646);
    rr_renderer_begin_path(renderer);
    rr_renderer_move_to(renderer, 25.0, 25.0);
    rr_renderer_line_to(renderer, -25.0, 25.0);
    rr_renderer_line_to(renderer, -25.0, -25.0);
    rr_renderer_line_to(renderer, 25.0, -25.0);
    rr_renderer_line_to(renderer, 25.0, 25.0);
    rr_renderer_fill(renderer);
}