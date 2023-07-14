#include <Client/Ui/Ui.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <Client/Game.h>
#include <Client/InputData.h>
#include <Client/Renderer/RenderFunctions.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>
#include <Client/Ui/Engine.h>

#include <Shared/StaticData.h>
#include <Shared/Utilities.h>

struct crafting_ring_metadata
{
    float angle;
};

struct crafting_ring_button_metadata
{
    uint32_t count;
    float secondary_animation;
    uint8_t pos;
    uint8_t prev_id;
    uint8_t prev_rarity;
};

struct crafting_inventory_button_metadata
{
    uint8_t id;
    uint8_t rarity;
    uint32_t count;
    float secondary_animation;
};

static void craft_button_on_event(struct rr_ui_element *this, struct rr_game *game)
{
    if (game->input_data->mouse_buttons_up_this_tick & 1)
    {
        if (game->crafting_data.count >= 5 && game->crafting_data.crafting_id != 0 && game->crafting_data.crafting_rarity < rr_rarity_id_ultra)
            game->crafting_data.success_count = 1;
    }
}

static uint8_t crafting_ring_should_show(struct rr_ui_element *this, struct rr_game *game)
{
    return game->crafting_data.success_count == 0;
}

static void crafting_result_container_on_event(struct rr_ui_element *this, struct rr_game *game)
{
    if (game->input_data->mouse_buttons_up_this_tick & 1)
    {
        game->crafting_data.count = game->crafting_data.success_count = 0;
    }
}

static void crafting_ring_petal_animate(struct rr_ui_element *this, struct rr_game *game)
{
    struct crafting_ring_button_metadata *data = this->data;
    data->count = (game->crafting_data.count + data->pos) / 5;
    data->secondary_animation = rr_lerp(data->secondary_animation, data->count == 0 || data->prev_id != game->crafting_data.crafting_id || data->prev_rarity != game->crafting_data.crafting_rarity, 0.4);
    rr_renderer_scale(game->renderer, game->renderer->scale * this->width / 60);
    rr_renderer_render_background(game->renderer, 254);
    if (game->crafting_data.crafting_id != 0)
    {
        data->prev_id = game->crafting_data.crafting_id;
        data->prev_rarity = game->crafting_data.crafting_rarity;
    }
    rr_renderer_scale(game->renderer, (1 - data->secondary_animation));
    rr_renderer_rotate(game->renderer, data->secondary_animation * M_PI * 2);
}

static void crafting_ring_petal_on_render(struct rr_ui_element *this, struct rr_game *game)
{
    struct crafting_ring_button_metadata *data = this->data;
    struct rr_renderer *renderer = game->renderer;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(renderer, &state);
    rr_renderer_render_background(renderer, data->prev_rarity);
    rr_renderer_draw_image(renderer,
                           &game->static_petals[data->prev_id][data->prev_rarity]);
    rr_renderer_context_state_free(renderer, &state);

    rr_renderer_translate(renderer, 25, -25);
    rr_renderer_rotate(renderer, 0.5);
    rr_renderer_set_fill(renderer, 0xffffffff);
    rr_renderer_set_stroke(renderer, 0xff222222);
    rr_renderer_set_text_align(renderer, 1);
    rr_renderer_set_text_baseline(renderer, 1);
    rr_renderer_set_text_size(renderer, 18);
    rr_renderer_set_line_width(renderer, 18 * 0.12);

    char out[12];
    out[sprintf(&out[0], "x%d", data->count)] = 0;
    rr_renderer_stroke_text(renderer, (char const *)&out, 0, 0);
    rr_renderer_fill_text(renderer, (char const *)&out, 0, 0);
}

static struct rr_ui_element *crafting_ring_petal_init(uint8_t pos)
{
    struct rr_ui_element *this = rr_ui_element_init();
    struct crafting_ring_button_metadata *data = malloc(sizeof *data);
    data->pos = pos;
    data->prev_id = data->prev_rarity = 0;
    data->secondary_animation = 0;
    this->data = data;
    this->abs_width = this->width = 60;
    this->abs_height = this->height = 60;
    this->animate = crafting_ring_petal_animate;
    this->on_render = crafting_ring_petal_on_render;
    this->on_event = crafting_result_container_on_event;
    return this;
}

static void crafting_ring_on_render(struct rr_ui_element *this, struct rr_game *game)
{
    struct rr_renderer_context_state state;
    for (uint32_t i = 0; i < this->elements.size; ++i)
    {
        rr_renderer_context_state_init(game->renderer, &state);
        rr_renderer_translate(game->renderer, game->renderer->scale * cosf(i * 2 * M_PI / this->elements.size) * 120, game->renderer->scale * sinf(i * 2 * M_PI / this->elements.size) * 120);
        rr_ui_render_element(this->elements.start[i], game);
        rr_renderer_context_state_free(game->renderer, &state);
    }
}
static struct rr_ui_element *crafting_ring_init()
{
    struct rr_ui_element *this = rr_ui_element_init();
    struct crafting_ring_metadata *data = malloc(sizeof *data);
    data->angle = 0;
    this->abs_width = this->width = 300;
    this->abs_height = this->height = 300;
    this->on_render = crafting_ring_on_render;
    rr_ui_container_add_element(this, crafting_ring_petal_init(0));
    rr_ui_container_add_element(this, crafting_ring_petal_init(1));
    rr_ui_container_add_element(this, crafting_ring_petal_init(2));
    rr_ui_container_add_element(this, crafting_ring_petal_init(3));
    rr_ui_container_add_element(this, crafting_ring_petal_init(4));
    this->poll_events = rr_ui_container_poll_events;
    this->should_show = crafting_ring_should_show;
    return this;
}

static uint8_t crafting_result_container_should_show(struct rr_ui_element *this, struct rr_game *game)
{
    return game->crafting_data.success_count;
}

static void crafting_result_container_on_render(struct rr_ui_element *this, struct rr_game *game)
{
    struct rr_renderer *renderer = game->renderer;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(renderer, &state);
    rr_renderer_render_background(renderer, game->crafting_data.crafting_rarity + 1);
    rr_renderer_draw_image(renderer,
                           &game->static_petals[game->crafting_data.crafting_id][game->crafting_data.crafting_rarity + 1]);
    rr_renderer_context_state_free(renderer, &state);

    rr_renderer_translate(renderer, 25, -25);
    rr_renderer_rotate(renderer, 0.5);
    rr_renderer_set_fill(renderer, 0xffffffff);
    rr_renderer_set_stroke(renderer, 0xff222222);
    rr_renderer_set_text_align(renderer, 1);
    rr_renderer_set_text_baseline(renderer, 1);
    rr_renderer_set_text_size(renderer, 18);
    rr_renderer_set_line_width(renderer, 18 * 0.12);

    char out[12];
    out[sprintf(&out[0], "x%d", game->crafting_data.success_count)] = 0;
    rr_renderer_stroke_text(renderer, (char const *)&out, 0, 0);
    rr_renderer_fill_text(renderer, (char const *)&out, 0, 0);
}

static struct rr_ui_element *crafting_result_container_init() 
{
    struct rr_ui_element *this = rr_ui_element_init();
    this->abs_width = this->width = 60;
    this->abs_height = this->height = 60;
    this->on_render = crafting_result_container_on_render;
    this->on_event = crafting_result_container_on_event;
    this->should_show = crafting_result_container_should_show;
    return this;
}

static struct rr_ui_element *crafting_button_init()
{
    struct rr_ui_element *this = rr_ui_labeled_button_init("Craft", 36, NULL);
    this->on_event = craft_button_on_event;
    return this;
}

static void crafting_inventory_button_animate(struct rr_ui_element *this, struct rr_game *game)
{
    struct crafting_inventory_button_metadata *data = this->data;
    uint32_t count = game->inventory[data->id][data->rarity];
    for (uint32_t i = 0; i < 20; ++i)
    {
        if (game->loadout[i].id == data->id &&
            game->loadout[i].rarity == data->rarity)
            --count;
    }
    if (data->id == game->crafting_data.crafting_id) 
    {
        if (data->rarity == game->crafting_data.crafting_rarity)
            count -= game->crafting_data.count;
        else if (data->rarity == game->crafting_data.crafting_rarity + 1)
            count -= game->crafting_data.success_count;
    }
    data->count = count;
    data->secondary_animation = rr_lerp(data->secondary_animation, count == 0, 0.2);
    rr_renderer_scale(game->renderer, game->renderer->scale * this->width / 60);
    rr_renderer_render_background(game->renderer, 254);
    rr_renderer_scale(game->renderer, (1 - data->secondary_animation));
}

static void crafting_inventory_button_on_event(struct rr_ui_element *this, struct rr_game *game)
{
    struct crafting_inventory_button_metadata *data = this->data;
    if (data->count == 0)
        return;
    if (game->input_data->mouse_buttons_up_this_tick & 1)
    {
        if (game->crafting_data.crafting_id != data->id || game->crafting_data.crafting_rarity != data->rarity)
        {
            game->crafting_data.crafting_id = data->id;
            game->crafting_data.crafting_rarity = data->rarity;
            game->crafting_data.count = 0;
            game->crafting_data.success_count = 0;
        }
        if (data->count > 5)
            game->crafting_data.count += 5;
        else    
            game->crafting_data.count += data->count;
    }
}

static void crafting_container_animate(struct rr_ui_element *this, struct rr_game *game)
{
    this->width = this->abs_width;
    this->height = this->abs_height;
    rr_renderer_translate(game->renderer, 0, -(this->y - this->abs_height / 2) * 2 * this->animation);
}

static void crafting_inventory_button_on_render(struct rr_ui_element *this, struct rr_game *game)
{
    struct crafting_inventory_button_metadata *data = this->data;
    struct rr_renderer *renderer = game->renderer;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(renderer, &state);
    rr_renderer_render_background(renderer, data->rarity);
    rr_renderer_draw_image(renderer,
                           &game->static_petals[data->id][data->rarity]);
    rr_renderer_context_state_free(renderer, &state);

    rr_renderer_translate(renderer, 25, -25);
    rr_renderer_rotate(renderer, 0.5);
    rr_renderer_set_fill(renderer, 0xffffffff);
    rr_renderer_set_stroke(renderer, 0xff222222);
    rr_renderer_set_text_align(renderer, 1);
    rr_renderer_set_text_baseline(renderer, 1);
    rr_renderer_set_text_size(renderer, 18);
    rr_renderer_set_line_width(renderer, 18 * 0.12);

    char out[12];
    out[sprintf(&out[0], "x%d", data->count)] = 0;
    rr_renderer_stroke_text(renderer, (char const *)&out, 0, 0);
    rr_renderer_fill_text(renderer, (char const *)&out, 0, 0);
}

struct rr_ui_element *crafting_inventory_button_init(uint8_t id, uint8_t rarity) 
{
    struct rr_ui_element *this = rr_ui_element_init();
    struct crafting_inventory_button_metadata *data = malloc(sizeof *data);
    data->id = id;
    data->rarity = rarity;
    data->count = 0;
    data->secondary_animation = 0;
    this->data = data;
    this->abs_width = this->width = 50;
    this->abs_height = this->height = 50;
    this->on_render = crafting_inventory_button_on_render;
    this->should_show = rr_ui_always_show;
    this->on_event = crafting_inventory_button_on_event;
    this->animate = crafting_inventory_button_animate;
    return this;
}

static uint8_t crafting_container_should_show(struct rr_ui_element *this, struct rr_game *game)
{
    return game->bottom_ui_open == 3 && !game->simulation_ready;
}

struct rr_ui_element *rr_ui_crafting_container_init()
{
    struct rr_ui_element *this = rr_ui_2d_container_init(rr_rarity_id_max, 6, 15, 15);
    for (uint8_t id = 1; id < rr_petal_id_max; ++id)
        for (uint8_t rarity = 0; rarity < rr_rarity_id_max; ++rarity)
            rr_ui_container_add_element(
                this,
                crafting_inventory_button_init(id, rarity)
            );
    this->fill = 0x00000000;
    struct rr_ui_element *craft = rr_ui_container_init();
    rr_ui_container_add_element(craft, crafting_ring_init());
    rr_ui_container_add_element(craft, crafting_result_container_init());
    craft->abs_width = craft->width = 300;
    craft->abs_height = craft->height = 300;
    struct rr_ui_element *c = rr_ui_set_background(
                rr_ui_pad(rr_ui_set_justify(
                              rr_ui_v_container_init(
                                  rr_ui_container_init(), 10, 10, 3,
                                  rr_ui_text_init("Crafting", 24, 0xffffffff),
                                  rr_ui_h_container_init(rr_ui_container_init(), 0, 25, 2, 
                                    crafting_button_init(),
                                    craft
                                  ),
                                  rr_ui_scroll_container_init(
                                      this, 300)),
                              -1, 1),
                          20),
            0xff13d4c5);
    c->x += 60 + 20;
    c->animate = crafting_container_animate;
    c->should_show = crafting_container_should_show;
    return c;
}

static void crafting_toggle_on_render(struct rr_ui_element *this,
                                     struct rr_game *game)
{
    struct rr_renderer *renderer = game->renderer;
    if (game->focused == this)
        renderer->state.filter.amount = 0.2;
    rr_renderer_scale(renderer, renderer->scale);
    rr_renderer_set_fill(renderer, this->fill);
    renderer->state.filter.amount += 0.2;
    rr_renderer_set_stroke(renderer, this->fill);
    rr_renderer_set_line_width(renderer, 6);
    rr_renderer_begin_path(renderer);
    rr_renderer_round_rect(renderer, -this->width / 2, -this->height / 2,
                           this->width, this->height, 6);
    rr_renderer_fill(renderer);
    rr_renderer_stroke(renderer);
}

void crafting_toggle_button_on_event(struct rr_ui_element *this, struct rr_game *game)
{
    if (game->input_data->mouse_buttons_up_this_tick & 1)
    {
        if (game->bottom_ui_open == 3)
            game->bottom_ui_open = 0;
        else
            game->bottom_ui_open = 3;
    }
}

struct rr_ui_element *rr_ui_crafting_toggle_button_init()
{
    struct rr_ui_element *this = rr_ui_element_init();
    this->fill = 0xffe34545;
    this->abs_width = this->abs_height = this->width = this->height = 60;
    this->on_event = crafting_toggle_button_on_event;
    this->on_render = crafting_toggle_on_render;
    return this;
}