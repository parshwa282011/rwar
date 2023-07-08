#include <Client/Game.h>

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifndef EMSCRIPTEN
#include <libwebsockets.h>
#endif

#include <Client/InputData.h>
#include <Client/Renderer/ComponentRender.h>
#include <Client/Renderer/RenderFunctions.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>
#include <Client/Socket.h>
#include <Client/Ui/Engine.h>
#include <Shared/Bitset.h>
#include <Shared/Component/Arena.h>
#include <Shared/Component/Flower.h>
#include <Shared/Component/Petal.h>
#include <Shared/Component/Physical.h>
#include <Shared/Component/PlayerInfo.h>
#include <Shared/Crypto.h>
#include <Shared/Rivet.h>
#include <Shared/Utilities.h>
#include <Shared/pb.h>

void rr_rivet_on_log_in(char *token, char *avatar_url, char *name,
                        char *account_number, void *captures)
{
    struct rr_game *this = captures;
    strcpy(this->rivet_account.token, token);
    strcpy(this->rivet_account.name, name);
    strcpy(this->rivet_account.avatar_url, avatar_url);
    strcpy(this->rivet_account.account_number, account_number);
}

static uint8_t simulation_not_ready(struct rr_ui_element *this, struct rr_game *game)
{
    return 1 - game->simulation_ready;
}

static uint8_t simulation_ready(struct rr_ui_element *this, struct rr_game *game)
{
    return game->simulation_ready;
}

static uint8_t socket_ready(struct rr_ui_element *this, struct rr_game *game)
{
    return game->socket_ready;
}


static void window_on_event(struct rr_ui_element *this, struct rr_game *game)
{
    if (game->input_data->mouse_buttons_this_tick & 1)
        game->bottom_ui_open = game->top_ui_open = 0;
}

void rr_game_init(struct rr_game *this)
{
    memset(this, 0, sizeof *this);
    this->window = rr_ui_container_init();
    this->window->container = this->window;
    this->window->h_justify = this->window->v_justify = 1;
    this->window->resizeable = 0;
    this->window->on_event = window_on_event;
    this->true_ptr = 1;

#ifdef RIVET_BUILD
    strcpy(this->rivet_account.name, "loading");
    strcpy(this->rivet_account.avatar_url, "");
    strcpy(this->rivet_account.token, "");
    strcpy(this->rivet_account.account_number, "#0000");
    rr_rivet_identities_create_guest(this);
#endif

    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_pad(
                rr_ui_set_justify(
                    rr_ui_rivet_container_init(this),
                -1, -1),
            10)
        , simulation_not_ready)
    );
    rr_ui_container_add_element(this->window, rr_ui_link_toggle(rr_ui_wave_container_init(), simulation_ready));
    rr_ui_container_add_element(this->window, rr_ui_settings_container_init());
    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_set_background(
                rr_ui_v_container_init(
                    rr_ui_container_init(), 10, 20, 6,
                    rr_ui_text_init("rrolf.io", 96, 0xffffffff),
                    rr_ui_h_container_init(
                        rr_ui_container_init(), 10, 20, 2,
                        rr_ui_text_init("name input (TODO)", 25, 0xffffffff),
                        rr_ui_set_background(
                            rr_ui_join_button_init(),
                            0xff1dd129)),
                    rr_ui_set_background(
                        rr_ui_link_toggle(
                            rr_ui_v_container_init(
                                rr_ui_container_init(), 10, 20, 3,
                                rr_ui_h_container_init(
                                    rr_ui_container_init(), 1, 15, 2,
                                    rr_ui_text_init("Squad", 18, 0xffffffff),
                                    rr_ui_info_init(), -1, 0),
                                rr_ui_h_container_init(
                                    rr_ui_container_init(), 10, 20, 4,
                                    rr_ui_squad_player_container_init(
                                        &this->squad_members[0]),
                                    rr_ui_squad_player_container_init(
                                        &this->squad_members[1]),
                                    rr_ui_squad_player_container_init(
                                        &this->squad_members[2]),
                                    rr_ui_squad_player_container_init(
                                        &this->squad_members[3])),
                                rr_ui_set_justify(rr_ui_countdown_init(this), 1,
                                                  0)),
                            socket_ready),
                        0x40ffffff),
                    rr_ui_h_container_init(
                        rr_ui_container_init(), 0, 15, 10,
                        rr_ui_title_screen_loadout_button_init(0),
                        rr_ui_title_screen_loadout_button_init(1),
                        rr_ui_title_screen_loadout_button_init(2),
                        rr_ui_title_screen_loadout_button_init(3),
                        rr_ui_title_screen_loadout_button_init(4),
                        rr_ui_title_screen_loadout_button_init(5),
                        rr_ui_title_screen_loadout_button_init(6),
                        rr_ui_title_screen_loadout_button_init(7),
                        rr_ui_title_screen_loadout_button_init(8),
                        rr_ui_title_screen_loadout_button_init(9)),
                    rr_ui_h_container_init(
                        rr_ui_container_init(), 0, 15, 10,
                        rr_ui_title_screen_loadout_button_init(10),
                        rr_ui_title_screen_loadout_button_init(11),
                        rr_ui_title_screen_loadout_button_init(12),
                        rr_ui_title_screen_loadout_button_init(13),
                        rr_ui_title_screen_loadout_button_init(14),
                        rr_ui_title_screen_loadout_button_init(15),
                        rr_ui_title_screen_loadout_button_init(16),
                        rr_ui_title_screen_loadout_button_init(17),
                        rr_ui_title_screen_loadout_button_init(18),
                        rr_ui_title_screen_loadout_button_init(19)),
                    rr_ui_text_init("powered by Rivet", 15, 0xffffffff)),
                0x00000000),
            simulation_not_ready));
    rr_ui_container_add_element(this->window, rr_ui_inventory_container_init());
    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_pad(
                rr_ui_set_justify(rr_ui_inventory_toggle_button_init(), -1, 1),
                10),
            simulation_not_ready));
    rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_set_justify(
                rr_ui_v_container_init(
                    rr_ui_container_init(), 15, 15, 2,
                    rr_ui_h_container_init(rr_ui_container_init(), 0, 15, 10,
                                           rr_ui_loadout_button_init(0),
                                           rr_ui_loadout_button_init(1),
                                           rr_ui_loadout_button_init(2),
                                           rr_ui_loadout_button_init(3),
                                           rr_ui_loadout_button_init(4),
                                           rr_ui_loadout_button_init(5),
                                           rr_ui_loadout_button_init(6),
                                           rr_ui_loadout_button_init(7),
                                           rr_ui_loadout_button_init(8),
                                           rr_ui_loadout_button_init(9)),
                    rr_ui_h_container_init(rr_ui_container_init(), 0, 15, 10,
                                           rr_ui_loadout_button_init(10),
                                           rr_ui_loadout_button_init(11),
                                           rr_ui_loadout_button_init(12),
                                           rr_ui_loadout_button_init(13),
                                           rr_ui_loadout_button_init(14),
                                           rr_ui_loadout_button_init(15),
                                           rr_ui_loadout_button_init(16),
                                           rr_ui_loadout_button_init(17),
                                           rr_ui_loadout_button_init(18),
                                           rr_ui_loadout_button_init(19))),
                0, 1),
            simulation_ready));

    this->squad_info_tooltip = rr_ui_container_add_element(
        this->window,
        rr_ui_link_toggle(
            rr_ui_set_justify(
                rr_ui_set_background(
                    rr_ui_v_container_init(
                        rr_ui_container_init(), 10, 10, 2,
                        rr_ui_set_justify(
                            rr_ui_h_container_init(
                                rr_ui_container_init(), 5, 10, 2,
                                rr_ui_flower_init(0, 35),
                                rr_ui_text_init("- ready", 15, 0xffffffff)),
                            -1, 0),
                        rr_ui_set_justify(
                            rr_ui_h_container_init(
                                rr_ui_container_init(), 5, 10, 2,
                                rr_ui_flower_init(1, 35),
                                rr_ui_text_init("- not ready", 15, 0xffffffff)),
                            -1, 0)),
                    0x80000000),
                -1, -1),
            rr_ui_never_show));
    this->squad_info_tooltip->poll_events = rr_ui_no_focus;
    for (uint32_t id = 0; id < rr_petal_id_max; ++id)
    {
        for (uint32_t rarity = 0; rarity < rr_rarity_id_max; ++rarity)
        {
            this->inventory[id][rarity] = rand() % 10;
            rr_renderer_init(&this->static_petals[id][rarity]);
            rr_renderer_set_dimensions(&this->static_petals[id][rarity], 50,
                                       50);
            rr_renderer_translate(&this->static_petals[id][rarity], 25, 25);
            rr_renderer_render_static_petal(&this->static_petals[id][rarity],
                                            id, rarity);
            char *cd = malloc((sizeof *cd) * 16);
            if (id == rr_petal_id_missile || id == rr_petal_id_peas)
                cd[sprintf(cd, "↻ %.1f + 0.5s",
                           (RR_PETAL_DATA[id].cooldown * 2 / 5) * 0.1)] = 0;
            else
                cd[sprintf(cd, "↻ %.1fs",
                           (RR_PETAL_DATA[id].cooldown * 2 / 5) * 0.1)] = 0;
            char *hp = malloc((sizeof *hp) * 16);
            hp[sprintf(hp, "%.1f",
                       RR_PETAL_DATA[id].health *
                           RR_PETAL_RARITY_SCALE[rarity])] = 0;
            char *dmg = malloc((sizeof *dmg) * 16);
            dmg[sprintf(dmg, "%.1f",
                        RR_PETAL_DATA[id].damage *
                            RR_PETAL_RARITY_SCALE[rarity] /
                            RR_PETAL_DATA[id].count[rarity])] = 0;
            this->petal_tooltips[id][rarity] = rr_ui_set_background(
                rr_ui_v_container_init(
                    rr_ui_container_init(), 10, 5, 6,
                    rr_ui_h_container_init(
                        rr_ui_container_init(), 0, 20, 2,
                        rr_ui_set_justify(
                            rr_ui_text_init(RR_PETAL_NAMES[id], 24, 0xffffffff),
                            -1, 0),
                        rr_ui_set_justify(rr_ui_text_init(cd, 16, 0xffffffff),
                                          1, 0)),
                    rr_ui_set_justify(rr_ui_text_init(RR_RARITY_NAMES[rarity],
                                                      16,
                                                      RR_RARITY_COLORS[rarity]),
                                      -1, 0),
                    rr_ui_static_space_init(10),
                    rr_ui_set_justify(rr_ui_text_init(RR_PETAL_DESCRIPTIONS[id],
                                                      16, 0xffffffff),
                                      -1, 0),
                    rr_ui_set_justify(
                        rr_ui_h_container_init(
                            rr_ui_container_init(), 0, 0, 2,
                            rr_ui_text_init("Health: ", 12, 0xff44ff44),
                            rr_ui_text_init(hp, 12, 0xff44ff44)),
                        -1, 0),
                    rr_ui_set_justify(
                        rr_ui_h_container_init(
                            rr_ui_container_init(), 0, 0, 2,
                            rr_ui_text_init("Damage: ", 12, 0xffff4444),
                            rr_ui_text_init(dmg, 12, 0xffff4444)),
                        -1, 0)),
                0x80000000);
            rr_ui_container_add_element(
                this->window,
                rr_ui_link_toggle(
                    rr_ui_set_justify(this->petal_tooltips[id][rarity], -1, -1),
                    rr_ui_never_show));
            this->petal_tooltips[id][rarity]->poll_events = rr_ui_no_focus;
            // remember that these don't have a container
        }
    }
    for (uint32_t id = 0; id < rr_mob_id_max; ++id)
    {
        for (uint32_t rarity = 0; rarity < rr_rarity_id_max; ++rarity)
        {
            char *hp = malloc((sizeof *hp) * 16);
            hp[sprintf(hp, "%.1f",
                       RR_MOB_DATA[id].health *
                           RR_MOB_RARITY_SCALING[rarity].health)] = 0;
            char *dmg = malloc((sizeof *dmg) * 16);
            dmg[sprintf(dmg, "%.1f",
                        RR_MOB_DATA[id].damage *
                            RR_MOB_RARITY_SCALING[rarity].damage)] = 0;
            this->mob_tooltips[id][rarity] = rr_ui_set_background(
                rr_ui_v_container_init(
                    rr_ui_container_init(), 10, 5, 3,
                        rr_ui_text_init(RR_MOB_NAMES[id], 24, 0xffffffff),
                    rr_ui_set_justify(
                        rr_ui_h_container_init(
                            rr_ui_container_init(), 0, 0, 2,
                            rr_ui_text_init("Health: ", 12, 0xff44ff44),
                            rr_ui_text_init(hp, 12, 0xff44ff44)),
                        -1, 0),
                    rr_ui_set_justify(
                        rr_ui_h_container_init(
                            rr_ui_container_init(), 0, 0, 2,
                            rr_ui_text_init("Damage: ", 12, 0xffff4444),
                            rr_ui_text_init(dmg, 12, 0xffff4444)),
                        -1, 0)),
                0x80000000);
            rr_ui_container_add_element(
                this->window,
                rr_ui_link_toggle(
                    rr_ui_set_justify(this->mob_tooltips[id][rarity], -1, -1),
                    rr_ui_never_show));
            this->mob_tooltips[id][rarity]->poll_events = rr_ui_no_focus;
            // remember that these don't have a container
        }
    }

    // TODO: move these out of this file
    rr_renderer_init(&this->mob_pteranodon_wings[0]);
    rr_renderer_set_dimensions(&this->mob_pteranodon_wings[0], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_pteranodon_wings[0],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#786750\" d=\"m386.3582 "
        "301.79822l0 0c-7.286682 -30.751251 -2.472046 -58.219604 10.753723 "
        "-61.35225l0 0c6.3512573 -1.5043488 13.832428 2.9191895 20.79773 "
        "12.297501c6.9653015 9.378296 12.844177 22.943146 16.343353 37.71042l0 "
        "0c7.2866516 30.751282 2.472046 58.219604 -10.753754 61.352264l0 "
        "0c-13.2258 3.1326294 -29.8544 -19.256683 -37.141052 -50.007935z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#786750\" d=\"m425.99774 "
        "351.18234l0 0c-16.637115 -23.047913 41.370087 -93.318115 129.56274 "
        "-156.9529c88.19263 -63.634796 173.17395 -96.53695 189.8111 "
        "-73.48904l0 0c-109.98651 59.214455 -228.51099 144.73492 -319.37384 "
        "230.44193z\" fill-rule=\"evenodd\"/><path fill=\"#786750\" "
        "d=\"m386.5859 262.60226l0 0c-10.515381 -26.405014 61.266968 -79.75401 "
        "160.33035 -119.158325c99.063416 -39.404305 187.89453 -49.942307 "
        "198.40991 -23.537292l0 0c-48.336426 1.7942276 -118.317444 19.371002 "
        "-188.89001 47.44255c-70.57257 28.071564 -133.4996 63.361435 "
        "-169.85025 95.25307z\" fill-rule=\"evenodd\"/><path fill=\"#786750\" "
        "d=\"m388.62253 256.97452l35.822784 88.68811l278.1125 "
        "-224.07074l-180.89618 47.403603z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#8c785b\" d=\"m398.67795 265.84482l0 0c-7.445648 -16.34021 "
        "54.72229 -60.642395 138.85593 -98.95169c84.13367 -38.30931 158.37335 "
        "-56.118782 165.81903 -39.77858l0 0c-40.205444 7.0623016 -99.1416 "
        "27.28038 -159.07825 54.571854c-59.936676 27.291473 -113.87814 "
        "58.470947 -145.59671 84.15842z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#8c785b\" d=\"m421.81674 338.33755l0 0c-10.827576 -14.326294 "
        "43.2312 -73.36975 120.74362 -131.87724c77.51245 -58.507492 149.12616 "
        "-94.323425 159.95374 -79.99713c-39.302673 16.843552 -94.93146 "
        "51.28653 -150.15118 92.96715c-55.219727 41.680634 -103.585175 "
        "85.73401 -130.54617 118.90723z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#8c785b\" d=\"m403.61795 257.5594l22.612823 67.74063l109.2616 "
        "-107.66379l145.43219 -86.43564l-93.3125 20.967926l-105.67853 "
        "49.48456z\" fill-rule=\"evenodd\"/><path fill=\"#8c785b\" "
        "d=\"m401.74457 301.68045l0 0c-5.4796143 -21.34262 -5.6539917 "
        "-39.741333 -0.38946533 -41.094666l0 0c2.5281372 -0.64990234 5.9980164 "
        "2.7982483 9.646362 9.585907c3.648346 6.7876587 7.1762695 16.358826 "
        "9.807678 26.60791l0 0c5.4796143 21.34262 5.6539917 39.741364 "
        "0.3894348 41.094696l0 0c-2.5281067 0.6498718 -5.9980164 -2.7982788 "
        "-9.646332 -9.5859375c-3.648346 -6.7876587 -7.1762695 -16.358795 "
        "-9.807678 -26.60791z\" fill-rule=\"evenodd\"/><path fill=\"#897d6d\" "
        "d=\"m409.55832 306.4071c3.7759094 3.7217102 6.6447754 8.6409 "
        "10.245789 15.942261c3.6010437 7.301361 8.966553 19.117004 11.360443 "
        "27.865906c2.3938904 8.748901 3.6963806 21.931 3.0029297 "
        "24.627563c-0.6934509 2.6965332 -5.4374084 -6.6534424 -7.1636047 "
        "-8.448242c-1.7261963 -1.7947998 -2.5322266 -2.5182495 -3.193634 "
        "-2.3205261c-0.6614075 0.1977539 -0.45843506 0.506958 -0.77474976 "
        "3.506958c-0.3163147 3.0000305 0.6402893 14.338989 -1.1231384 "
        "14.493195c-1.7634277 0.15423584 -4.887146 -7.3967285 -9.457428 "
        "-13.567841c-4.5703125 -6.171112 -12.010529 -16.673492 -17.964355 "
        "-23.458801c-5.9538574 -6.785309 -13.25296 -13.249695 -17.758728 "
        "-17.253021c-4.505768 -4.0033264 -9.984436 -3.0722046 -9.27594 "
        "-6.7669983c0.7085266 -3.6947632 8.578247 -11.90033 13.527039 "
        "-15.401733c4.9487915 -3.5014038 11.403198 -5.736908 16.165771 "
        "-5.6066895c4.7625427 0.13018799 8.6336975 2.6662598 12.409607 "
        "6.38797z\" fill-rule=\"evenodd\"/><path fill=\"#9d8f7d\" "
        "d=\"m405.77994 311.72452c3.1200867 3.1376343 6.2944946 8.12912 "
        "9.558929 14.066803c3.2644348 5.937683 7.855316 15.918823 10.027649 "
        "21.559296c2.1723633 5.6404724 3.131836 10.486755 3.0064087 "
        "12.283539c-0.12542725 1.7967834 -2.4639587 -1.4606018 -3.7590027 "
        "-1.5028687c-1.295044 -0.042266846 -3.092163 0.2880249 -4.0112915 "
        "1.2492676c-0.9191284 0.9612427 -1.0439758 3.3057861 -1.503479 "
        "4.5182495c-0.45950317 1.2124634 -0.20877075 3.5505676 -1.25354 "
        "2.7565002c-1.0447998 -0.7940979 -2.2984009 -3.6070557 -5.0152283 "
        "-7.5210266c-2.7168274 -3.913971 -6.9446106 -10.708252 -11.285706 "
        "-15.962799c-4.3410645 -5.254547 -10.750183 -11.455902 -14.760773 "
        "-15.564453c-4.010559 -4.108551 -9.068817 -6.2897034 -9.302612 "
        "-9.086914c-0.23376465 -2.7972107 4.710449 -5.7705994 7.899933 "
        "-7.69635c3.1895142 -1.9257812 7.837311 -4.008423 11.237091 "
        "-3.8582764c3.3997803 0.15011597 6.0415344 1.6213989 9.161621 "
        "4.759033z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_pteranodon_wings[1]);
    rr_renderer_set_dimensions(&this->mob_pteranodon_wings[1], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_pteranodon_wings[1],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#786750\" d=\"m413.6418 "
        "301.79822l0 0c7.286682 -30.751251 2.472046 -58.219604 -10.753723 "
        "-61.35225l0 0c-6.3512573 -1.5043488 -13.832428 2.9191895 -20.79773 "
        "12.297501c-6.9653015 9.378296 -12.844177 22.943146 -16.343353 "
        "37.71042l0 0c-7.2866516 30.751282 -2.472046 58.219604 10.753754 "
        "61.352264l0 0c13.2258 3.1326294 29.8544 -19.256683 37.141052 "
        "-50.007935z\" fill-rule=\"evenodd\"/><path fill=\"#786750\" "
        "d=\"m374.00226 351.18234l0 0c16.637115 -23.047913 -41.370087 "
        "-93.318115 -129.56271 -156.9529c-88.19266 -63.634796 -173.17398 "
        "-96.53695 -189.81111 -73.48904l0 0c109.98651 59.214455 228.51094 "
        "144.73492 319.37384 230.44193z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#786750\" d=\"m413.4141 262.60226l0 0c10.515381 -26.405014 "
        "-61.266968 -79.75401 -160.33037 -119.158325c-99.0634 -39.404305 "
        "-187.89453 -49.942307 -198.40991 -23.537292l0 0c48.33645 1.7942276 "
        "118.31744 19.371002 188.89001 47.44255c70.57257 28.071564 133.4996 "
        "63.361435 169.85025 95.25307z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#786750\" d=\"m411.37747 256.97452l-35.822784 "
        "88.68811l-278.11252 -224.07074l180.89618 47.403603z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#8c785b\" d=\"m401.32205 "
        "265.84482l0 0c7.445648 -16.34021 -54.72229 -60.642395 -138.85596 "
        "-98.95169c-84.13365 -38.30931 -158.37335 -56.118782 -165.81898 "
        "-39.77858l0 0c40.2054 7.0623016 99.14157 27.28038 159.07823 "
        "54.571854c59.936676 27.291473 113.87814 58.470947 145.59671 "
        "84.15842z\" fill-rule=\"evenodd\"/><path fill=\"#8c785b\" "
        "d=\"m378.18326 338.33755l0 0c10.827576 -14.326294 -43.2312 -73.36975 "
        "-120.74365 -131.87724c-77.512436 -58.507492 -149.12616 -94.323425 "
        "-159.9537 -79.99713c39.302666 16.843552 94.93148 51.28653 150.15121 "
        "92.96715c55.21971 41.680634 103.58516 85.73401 130.54616 118.90723z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#8c785b\" d=\"m396.38205 "
        "257.5594l-22.612823 67.74063l-109.2616 -107.66379l-145.43219 "
        "-86.43564l93.31251 20.967926l105.67853 49.48456z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#8c785b\" d=\"m398.25543 "
        "301.68045l0 0c5.4796143 -21.34262 5.6539917 -39.741333 0.38946533 "
        "-41.094666l0 0c-2.5281372 -0.64990234 -5.9980164 2.7982483 -9.646362 "
        "9.585907c-3.648346 6.7876587 -7.1762695 16.358826 -9.807678 "
        "26.60791l0 0c-5.4796143 21.34262 -5.6539917 39.741364 -0.3894348 "
        "41.094696l0 0c2.5281067 0.6498718 5.9980164 -2.7982788 9.646332 "
        "-9.5859375c3.648346 -6.7876587 7.1762695 -16.358795 9.807678 "
        "-26.60791z\" fill-rule=\"evenodd\"/><path fill=\"#897d6d\" "
        "d=\"m390.44168 306.4071c-3.7759094 3.7217102 -6.6447754 8.6409 "
        "-10.245789 15.942261c-3.6010437 7.301361 -8.966553 19.117004 "
        "-11.360443 27.865906c-2.3938904 8.748901 -3.6963806 21.931 -3.0029297 "
        "24.627563c0.6934509 2.6965332 5.4374084 -6.6534424 7.1636047 "
        "-8.448242c1.7261963 -1.7947998 2.5322266 -2.5182495 3.193634 "
        "-2.3205261c0.6614075 0.1977539 0.45843506 0.506958 0.77474976 "
        "3.506958c0.3163147 3.0000305 -0.6402893 14.338989 1.1231384 "
        "14.493195c1.7634277 0.15423584 4.887146 -7.3967285 9.457428 "
        "-13.567841c4.5703125 -6.171112 12.010529 -16.673492 17.964355 "
        "-23.458801c5.9538574 -6.785309 13.25296 -13.249695 17.758728 "
        "-17.253021c4.505768 -4.0033264 9.984436 -3.0722046 9.27594 "
        "-6.7669983c-0.7085266 -3.6947632 -8.578247 -11.90033 -13.527039 "
        "-15.401733c-4.9487915 -3.5014038 -11.403198 -5.736908 -16.165771 "
        "-5.6066895c-4.7625427 0.13018799 -8.6336975 2.6662598 -12.409607 "
        "6.38797z\" fill-rule=\"evenodd\"/><path fill=\"#9d8f7d\" "
        "d=\"m394.22006 311.72452c-3.1200867 3.1376343 -6.2944946 8.12912 "
        "-9.558929 14.066803c-3.2644348 5.937683 -7.855316 15.918823 "
        "-10.027649 21.559296c-2.1723633 5.6404724 -3.131836 10.486755 "
        "-3.0064087 12.283539c0.12542725 1.7967834 2.4639587 -1.4606018 "
        "3.7590027 -1.5028687c1.295044 -0.042266846 3.092163 0.2880249 "
        "4.0112915 1.2492676c0.9191284 0.9612427 1.0439758 3.3057861 1.503479 "
        "4.5182495c0.45950317 1.2124634 0.20877075 3.5505676 1.25354 "
        "2.7565002c1.0447998 -0.7940979 2.2984009 -3.6070557 5.0152283 "
        "-7.5210266c2.7168274 -3.913971 6.9446106 -10.708252 11.285706 "
        "-15.962799c4.3410645 -5.254547 10.750183 -11.455902 14.760773 "
        "-15.564453c4.010559 -4.108551 9.068817 -6.2897034 9.302612 "
        "-9.086914c0.23376465 -2.7972107 -4.710449 -5.7705994 -7.899933 "
        "-7.69635c-3.1895142 -1.9257812 -7.837311 -4.008423 -11.237091 "
        "-3.8582764c-3.3997803 0.15011597 -6.0415344 1.6213989 -9.161621 "
        "4.759033z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_pteranodon_body);
    rr_renderer_set_dimensions(&this->mob_pteranodon_body, 800, 600);
    rr_renderer_draw_svg(
        &this->mob_pteranodon_body,
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#ad9168\" d=\"m462.61646 "
        "301.64597l0 0c0 49.584717 -27.998322 89.78104 -62.53598 89.78104l0 "
        "0c-16.585602 0 -32.49185 -9.459045 -44.219635 -26.296234c-11.727783 "
        "-16.83722 -18.316376 -39.67337 -18.316376 -63.484802l0 0c0 -49.584686 "
        "27.998322 -89.78102 62.53601 -89.78102l0 0c34.53766 0 62.53598 "
        "40.196335 62.53598 89.78102z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#ad9168\" d=\"m422.5955 374.0688l0 0c0 18.418182 -10.082184 "
        "33.34909 -22.519165 33.34909l0 0c-5.972473 0 -11.700317 -3.5135498 "
        "-15.923462 -9.767731c-4.223175 -6.254181 -6.5957336 -14.736633 "
        "-6.5957336 -23.58136l0 0c0 -18.418213 10.082184 -33.34912 22.519196 "
        "-33.34912l0 0c12.436981 0 22.519165 14.930908 22.519165 33.34912z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#c4a575\" d=\"m431.3498 "
        "253.4032l0 0c17.272339 26.452606 17.272339 69.34076 0 "
        "95.793396q-13.934265 21.340363 -31.274323 37.464783q-17.340027 "
        "-16.12442 -31.274292 -37.464783l0 0c-17.272339 -26.452637 -17.272339 "
        "-69.34079 0 -95.793396l0 0c17.272308 -26.452621 45.276306 -26.452621 "
        "62.548615 0z\" fill-rule=\"evenodd\"/><path fill=\"#ad9168\" "
        "d=\"m413.19513 401.32907l27.48291 -31.596985l-26.58612 5.8094788z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#ad9168\" d=\"m386.9635 "
        "401.32907l-27.48291 -31.596985l26.58612 5.8094788z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#897d6d\" d=\"m445.55667 "
        "221.80867c3.3269348 -5.1845856 -14.765411 -23.04506 -22.133942 "
        "-36.058426c-7.3684998 -13.013382 -17.649597 -36.6138 -22.077087 "
        "-42.021866c-4.42749 -5.4080505 -4.8404236 -2.6147308 -4.4878235 "
        "9.573486c0.35256958 12.188232 -1.5131531 52.138077 6.6033325 "
        "63.555862c8.116486 11.417801 38.768585 10.135513 42.09552 4.950943z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#9d8f7d\" d=\"m350.43872 "
        "221.80872c-3.3269348 -5.1845856 14.765411 -23.045044 22.133942 "
        "-36.058426c7.3684998 -13.013382 17.649597 -36.613785 22.077087 "
        "-42.02185c4.42749 -5.408066 4.8404236 -2.614746 4.4878235 "
        "9.573486c-0.35256958 12.188217 1.5131531 52.13806 -6.6033325 "
        "63.555862c-8.116486 11.417801 -38.768585 10.135513 -42.09552 "
        "4.9509277z\" fill-rule=\"evenodd\"/><path fill=\"#d4c0a2\" "
        "d=\"m331.98386 291.96344l0 0c0 -30.645203 30.449554 -55.488037 "
        "68.01099 -55.488037l0 0c37.56143 0 68.01099 24.842834 68.01099 "
        "55.488037l0 0c0 30.645172 -30.449554 55.488007 -68.01099 55.488007l0 "
        "0c-37.56143 0 -68.01099 -24.842834 -68.01099 -55.488007z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#bca787\" d=\"m322.81824 "
        "291.9576l0 0c0 -34.77191 34.555298 -62.960144 77.18146 -62.960144l0 "
        "0c42.62613 0 77.18143 28.188232 77.18143 62.960144l0 0c0 34.771942 "
        "-34.555298 62.960175 -77.18143 62.960175l0 0c-42.62616 0 -77.18146 "
        "-28.188232 -77.18146 -62.960175zm15.780334 0l0 0c0 26.056702 "
        "27.490204 47.17984 61.401123 47.17984c33.91089 0 61.401093 -21.123138 "
        "61.401093 -47.17984c0 -26.056671 -27.490204 -47.17981 -61.401093 "
        "-47.17981l0 0c-33.91092 0 -61.401123 21.123138 -61.401123 47.17981z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#ad9168\" d=\"m462.5986 "
        "252.8799l0 0c0 34.50688 -27.995972 62.48024 -62.53073 62.48024l0 "
        "0c-16.584198 0 -32.489136 -6.582733 -44.21591 -18.300049c-11.726776 "
        "-11.717316 -18.31482 -27.609406 -18.31482 -44.18019l0 0c0 -34.506866 "
        "27.995941 -62.48021 62.53073 -62.48021l0 0c34.53476 0 62.53073 "
        "27.973343 62.53073 62.48021z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#ad9168\" d=\"m422.58112 302.24533l0 0c0 12.423004 -10.082184 "
        "22.493866 -22.519165 22.493866l0 0c-5.9724426 0 -11.700256 -2.369873 "
        "-15.923431 -6.5882874c-4.2231445 -4.218445 -6.595703 -9.93985 "
        "-6.595703 -15.905579l0 0c0 -12.423035 10.082153 -22.493896 22.519135 "
        "-22.493896l0 0c12.436981 0 22.519165 10.070862 22.519165 22.493896z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#c4a575\" d=\"m431.3259 "
        "221.6119l0 0c17.270874 17.27089 17.270874 45.272507 0 "
        "62.54338q-13.452759 13.452759 -31.271698 22.539368q-17.818909 "
        "-9.086609 -31.271667 -22.539368l0 0c-17.270874 -17.270874 -17.270874 "
        "-45.27249 0 -62.54338l0 0c17.270874 -17.270874 45.27249 -17.270874 "
        "62.543365 0z\" fill-rule=\"evenodd\"/><path fill=\"#ad9168\" "
        "d=\"m415.2795 318.79263l25.217743 -18.475739l-24.390503 0z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#ad9168\" d=\"m384.8538 "
        "318.79263l-25.217743 -18.475739l24.39047 0z\" "
        "fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_trex_head);
    rr_renderer_set_dimensions(&this->mob_trex_head, 800, 600);
    rr_renderer_draw_svg(
        &this->mob_trex_head,
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m361.76172 "
        "74.20284l0 0c0 -21.117855 17.119415 -38.23726 38.237244 -38.23726l0 "
        "0l0 0c10.141174 0 19.866974 4.028557 27.037842 11.199432c7.170868 "
        "7.1708794 11.199432 16.896675 11.199432 27.037827l0 24.729721c0 "
        "21.117859 -17.119415 38.237267 -38.237274 38.237267l0 0l0 "
        "0c-21.117828 0 -38.237244 -17.119408 -38.237244 -38.237267z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#c4a575\" d=\"m349.24887 "
        "140.63026l0 0c0 -28.573807 22.719788 -51.73745 50.746063 -51.73745l0 "
        "0c13.458679 0 26.36612 5.450897 35.882874 15.153549c9.516724 9.702652 "
        "14.863159 22.862274 14.863159 36.5839l0 0c0 28.573792 -22.719788 "
        "51.737442 -50.746033 51.737442l0 0c-28.026276 0 -50.746063 -23.16365 "
        "-50.746063 -51.737442z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#a68b63\" d=\"m342.4096 140.6352l0 0c0 -32.42755 25.783966 "
        "-58.71527 57.59015 -58.71527l0 0c15.273865 0 29.92215 6.186058 "
        "40.72238 17.197304c10.800262 11.011246 16.867767 25.945702 16.867767 "
        "41.517967l0 0c0 32.427536 -25.783997 58.715256 -57.59015 58.715256l0 "
        "0c-31.806183 0 -57.59015 -26.28772 -57.59015 -58.715256zm15.981262 "
        "0l0 0c0 23.601334 18.628937 42.733994 41.608887 42.733994c22.97995 0 "
        "41.608887 -19.13266 41.608887 -42.733994c0 -23.601341 -18.628937 "
        "-42.734 -41.608887 -42.734l0 0c-22.97995 0 -41.608887 19.13266 "
        "-41.608887 42.734z\" fill-rule=\"evenodd\"/><path fill=\"#c4a575\" "
        "d=\"m377.22412 72.82035l0 0c0 -12.578781 10.197113 -22.775894 "
        "22.775879 -22.775894l0 0l0 0c6.040558 0 11.833679 2.3995972 16.10498 "
        "6.6709023c4.271332 4.271309 6.670929 10.064449 6.670929 16.104992l0 "
        "41.573578l0 0c0 12.578781 -10.197144 22.775902 -22.77591 22.775902l0 "
        "0l0 0c-12.578766 0 -22.775879 -10.197121 -22.775879 -22.775902z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#c4a575\" d=\"m432.84137 "
        "114.40563l0 0c-6.2485046 4.807129 -13.571838 0.79649353 -16.357178 "
        "-8.958c-2.7853394 -9.754486 0.022125244 -21.559013 6.2705994 "
        "-26.366142l0 0c-0.7946472 13.998566 3.0236511 27.370613 10.086578 "
        "35.324142z\" fill-rule=\"evenodd\"/><path fill=\"#a68b63\" "
        "d=\"m446.4799 106.050095l0 0c-5.134369 3.9500961 -11.153473 "
        "0.65000916 -13.444061 -7.3709564c-2.2905884 -8.020966 0.01473999 "
        "-17.725433 5.1490784 -21.675537l0 0c-0.602417 11.493248 2.527649 "
        "22.453804 8.294983 29.046494z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#c4a575\" d=\"m367.15952 114.40408l0 0c6.248474 4.807129 "
        "13.571838 0.79649353 16.357178 -8.958c2.7853088 -9.754494 "
        "-0.022125244 -21.559013 -6.2705994 -26.366142l0 0c0.7946472 13.998566 "
        "-3.0236511 27.370613 -10.086578 35.324142z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m353.52124 "
        "106.0485l0 0c5.1343384 3.9500961 11.153442 0.65000916 13.444061 "
        "-7.3709564c2.2905884 -8.020966 -0.01473999 -17.725433 -5.149109 "
        "-21.675537l0 0c0.6024475 11.493248 -2.5276184 22.453804 -8.294952 "
        "29.046494z\" fill-rule=\"evenodd\"/><path fill=\"#a68b63\" "
        "d=\"m421.1062 142.60603l0 0c-9.791565 -8.484848 -15.427643 -20.795135 "
        "-15.452637 -33.751472l13.350006 -0.025733948c0.017547607 9.093269 "
        "3.973175 17.733131 10.845276 23.688126z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m405.58594 "
        "108.76873l0 0c0 -3.6657257 2.9716492 -6.6373825 6.63739 -6.6373825l0 "
        "0c1.760315 0 3.448578 0.69929504 4.693329 1.944046c1.244751 1.244751 "
        "1.9440308 2.932991 1.9440308 4.6933365l0 0c0 3.6657257 -2.9716492 "
        "6.6373825 -6.6373596 6.6373825l0 0c-3.665741 0 -6.63739 -2.9716568 "
        "-6.63739 -6.6373825z\" fill-rule=\"evenodd\"/><path fill=\"#a68b63\" "
        "d=\"m418.8607 137.52274l0 0c0 -3.6657257 2.9716797 -6.63739 6.63739 "
        "-6.63739l0 0c1.7603455 0 3.4486084 0.69929504 4.6933594 "
        "1.944046c1.244751 1.244751 1.9440308 2.9329987 1.9440308 4.693344l0 "
        "0c0 3.6657257 -2.9716492 6.637375 -6.63739 6.637375l0 0c-3.6657104 0 "
        "-6.63739 -2.9716492 -6.63739 -6.637375z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m378.89423 "
        "142.60652l0 0c9.791565 -8.484848 15.427643 -20.795143 15.452637 "
        "-33.75148l-13.350006 -0.025733948c-0.01751709 9.093277 -3.973175 "
        "17.733131 -10.845276 23.688133z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#a68b63\" d=\"m394.4145 108.76922l0 0c0 -3.6657257 -2.9716492 "
        "-6.6373825 -6.6373596 -6.6373825l0 0c-1.7603455 0 -3.4486084 "
        "0.6992874 -4.6933594 1.9440384c-1.244751 1.244751 -1.9440308 "
        "2.9329987 -1.9440308 4.693344l0 0c0 3.6657257 2.9716492 6.6373825 "
        "6.63739 6.6373825l0 0c3.6657104 0 6.6373596 -2.9716568 6.6373596 "
        "-6.6373825z\" fill-rule=\"evenodd\"/><path fill=\"#a68b63\" "
        "d=\"m381.13974 137.52322l0 0c0 -3.665741 -2.9716797 -6.63739 -6.63739 "
        "-6.63739l0 0c-1.7603455 0 -3.448578 0.69929504 -4.693329 "
        "1.944046c-1.244751 1.244751 -1.9440613 2.9329987 -1.9440613 "
        "4.693344l0 0c0 3.6657257 2.9716492 6.637375 6.63739 6.637375l0 "
        "0c3.6657104 0 6.63739 -2.9716492 6.63739 -6.637375z\" "
        "fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_trex_legs[0]);
    rr_renderer_set_dimensions(&this->mob_trex_legs[0], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_trex_legs[0],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m470.09436 "
        "255.96356c1.7186279 -3.789627 2.4355774 -8.201233 3.0183105 "
        "-14.564209c0.58273315 -6.362961 1.1873169 -16.529968 0.4781494 "
        "-23.613617c-0.7091675 -7.083664 -3.465973 -17.10794 -4.733124 "
        "-18.88829c-1.2671814 -1.780365 -2.1105347 6.405258 -2.869934 "
        "8.206131c-0.7593994 1.8008881 -1.1462708 2.5578308 -1.6864929 "
        "2.5991669c-0.54022217 0.0413208 -0.47857666 -0.24234009 -1.5548706 "
        "-2.3511963c-1.0763245 -2.1088715 -3.5676575 -10.685425 -4.903015 "
        "-10.301971c-1.3353577 0.38345337 -1.4981079 6.794922 -3.1091614 "
        "12.602692c-1.611023 5.807785 -4.105316 15.596725 -6.5570984 "
        "22.243988c-2.4517517 6.6472473 -5.979431 13.43811 -8.153473 "
        "17.639557c-2.1740417 4.2014313 -6.450012 5.061615 -4.8907776 "
        "7.5690765c1.5592346 2.5074463 9.634888 6.303589 14.246246 "
        "7.475647c4.6113586 1.1720276 9.969391 0.99279785 13.421936 "
        "-0.44335938c3.4525452 -1.4361572 5.574707 -4.383972 7.2933044 "
        "-8.1736145z\" fill-rule=\"evenodd\"/><path fill=\"#c4a575\" "
        "d=\"m465.82925 253.13156c1.4025574 -3.1771088 2.322998 -7.7276764 "
        "3.0430603 -12.996811c0.72003174 -5.2691345 1.2735596 -13.874069 "
        "1.2771606 -18.618011c0.0036010742 -4.7439423 -0.6577759 -8.564606 "
        "-1.2554932 -9.845673c-0.5977478 -1.2810669 -1.3939819 1.7637329 "
        "-2.3308716 2.1592865c-0.93688965 0.39553833 -2.3464966 0.6595001 "
        "-3.2904968 0.21401978c-0.94400024 -0.4454956 -1.6954956 -2.1280365 "
        "-2.3734741 -2.8869476c-0.6779785 -0.7589264 -1.1525269 -2.5424957 "
        "-1.6944275 -1.6665955c-0.5418701 0.87590027 -0.668396 3.289688 "
        "-1.5569458 6.9220123c-0.8885803 3.6323395 -2.0732422 9.800278 "
        "-3.7743835 14.872025c-1.7011414 5.0717316 -4.6508484 11.419342 "
        "-6.4324646 15.558456c-1.7816162 4.1391144 -4.8734436 7.1611023 "
        "-4.2572327 9.276245c0.61621094 2.1151428 5.0755615 2.9016113 "
        "7.9544373 3.4145813c2.8788757 0.51297 6.87027 0.73031616 9.318817 "
        "-0.336792c2.4485168 -1.0670776 3.969757 -2.8887024 5.3723145 "
        "-6.065796z\" fill-rule=\"evenodd\"/><path fill=\"#c4a575\" "
        "d=\"m315.73773 315.4619l0 0c0 -20.723022 16.476898 -37.52234 "
        "36.802155 -37.52234l0 0c9.760559 0 19.121307 3.953247 26.023071 "
        "10.990051c6.9017334 7.036804 10.779114 16.58075 10.779114 26.532288l0 "
        "0c0 20.722992 -16.476898 37.52231 -36.802185 37.52231l0 0c-20.325256 "
        "0 -36.802155 -16.799316 -36.802155 -37.52231z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m310.77774 "
        "315.46548l0 0c0 -23.517914 18.699127 -42.582947 41.765656 "
        "-42.582947l0 0c11.076935 0 21.700195 4.486389 29.532776 "
        "12.4722595c7.832611 7.98584 12.23291 18.816986 12.23291 30.110687l0 "
        "0c0 23.517914 -18.699127 42.582947 -41.765686 42.582947l0 "
        "0c-23.066528 0 -41.765656 -19.065033 -41.765656 -42.582947zm11.589966 "
        "0l0 0c0 17.116943 13.510101 30.992981 30.17569 30.992981c16.665588 0 "
        "30.17569 -13.876038 30.17569 -30.992981c0 -17.116943 -13.510101 "
        "-30.992981 -30.17569 -30.992981l0 0c-16.665588 0 -30.17569 13.876038 "
        "-30.17569 30.992981z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_trex_legs[1]);
    rr_renderer_set_dimensions(&this->mob_trex_legs[1], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_trex_legs[1],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m329.8556 "
        "255.96257c-1.7185974 -3.789627 -2.4355469 -8.201233 -3.01828 "
        "-14.564209c-0.58273315 -6.362961 -1.1873169 -16.529968 -0.4781494 "
        "-23.613617c0.7091675 -7.083664 3.465973 -17.10794 4.733124 "
        "-18.88829c1.2671814 -1.780365 2.1105347 6.405258 2.869934 "
        "8.206131c0.7593994 1.8008881 1.1462708 2.5578308 1.6864929 "
        "2.5991669c0.54022217 0.0413208 0.47857666 -0.24234009 1.5548706 "
        "-2.3511963c1.0763245 -2.1088715 3.5676575 -10.685425 4.903015 "
        "-10.301971c1.3353577 0.38345337 1.4981079 6.794922 3.1091614 "
        "12.602692c1.611023 5.807785 4.105316 15.596725 6.557068 "
        "22.243988c2.4517822 6.6472473 5.9794617 13.43811 8.153503 "
        "17.639557c2.1740417 4.2014313 6.450012 5.061615 4.8907776 "
        "7.5690613c-1.5592651 2.5074768 -9.634888 6.3036194 -14.246246 "
        "7.475647c-4.6113586 1.1720581 -9.969391 0.99279785 -13.421936 "
        "-0.44335938c-3.4525452 -1.4361572 -5.574707 -4.383972 -7.293335 "
        "-8.173599z\" fill-rule=\"evenodd\"/><path fill=\"#c4a575\" "
        "d=\"m334.12073 253.13057c-1.4025574 -3.1771088 -2.3230286 -7.7276764 "
        "-3.0430603 -12.996811c-0.72003174 -5.2691345 -1.2735596 -13.874069 "
        "-1.2771606 -18.618011c-0.0036010742 -4.7439423 0.6577759 -8.564606 "
        "1.2554932 -9.845673c0.5977478 -1.2810669 1.3939819 1.7637329 "
        "2.3308716 2.1592865c0.93688965 0.39553833 2.3464966 0.6595001 "
        "3.2904968 0.21401978c0.94400024 -0.4454956 1.6954956 -2.1280365 "
        "2.3734741 -2.8869476c0.6779785 -0.7589264 1.1525269 -2.5424957 "
        "1.694397 -1.6665955c0.54190063 0.87590027 0.6684265 3.289688 "
        "1.5569763 6.9220123c0.8885803 3.6323395 2.0732422 9.800278 3.7743835 "
        "14.872025c1.7011414 5.0717316 4.6508484 11.419342 6.4324646 "
        "15.558456c1.7816162 4.1391144 4.8734436 7.1611023 4.2572327 "
        "9.27623c-0.61621094 2.1151428 -5.0755615 2.9016418 -7.9544373 "
        "3.4146118c-2.8788757 0.51297 -6.8703003 0.73031616 -9.318817 "
        "-0.336792c-2.4485168 -1.0671082 -3.969757 -2.8887024 -5.3723145 "
        "-6.065811z\" fill-rule=\"evenodd\"/><path fill=\"#c4a575\" "
        "d=\"m410.69073 315.4619l0 0c0 -20.723022 16.476898 -37.52234 "
        "36.802185 -37.52234l0 0c9.760529 0 19.121307 3.953247 26.02304 "
        "10.990051c6.901764 7.036804 10.779114 16.58075 10.779114 26.532288l0 "
        "0c0 20.722992 -16.476898 37.52231 -36.802155 37.52231l0 0c-20.325287 "
        "0 -36.802185 -16.799316 -36.802185 -37.52231z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m405.73074 "
        "315.46548l0 0c0 -23.517914 18.699127 -42.582947 41.765656 "
        "-42.582947l0 0c11.076965 0 21.700226 4.486389 29.532806 "
        "12.4722595c7.8325806 7.98584 12.23288 18.816986 12.23288 30.110687l0 "
        "0c0 23.517914 -18.699127 42.582947 -41.765686 42.582947l0 "
        "0c-23.066528 0 -41.765656 -19.065033 -41.765656 -42.582947zm11.589966 "
        "0l0 0c0 17.116943 13.510132 30.992981 30.17569 30.992981c16.665588 0 "
        "30.17572 -13.876038 30.17572 -30.992981c0 -17.116943 -13.510132 "
        "-30.992981 -30.17572 -30.992981l0 0c-16.665558 0 -30.17569 13.876038 "
        "-30.17569 30.992981z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_trex_body);
    rr_renderer_set_dimensions(&this->mob_trex_body, 800, 600);
    rr_renderer_draw_svg(
        &this->mob_trex_body,
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#c4a575\" d=\"m338.42426 "
        "282.12485l0 0c0 -52.46553 27.562622 -94.99724 61.562866 -94.99724l0 "
        "0c16.327484 0 31.986237 10.008606 43.531525 27.824036c11.545288 "
        "17.815445 18.031342 41.97838 18.031342 67.1732l0 0c0 52.465515 "
        "-27.562622 94.99725 -61.562866 94.99725l0 0c-34.000244 0 -61.562866 "
        "-42.53174 -61.562866 -94.99725z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#a68b63\" d=\"m330.12714 282.1339l0 0c0 -59.541565 31.279999 "
        "-107.809525 69.865845 -107.809525l0 0c18.529572 0 36.300232 11.358475 "
        "49.40262 31.576675c13.102386 20.2182 20.463226 47.63997 20.463226 "
        "76.23285l0 0c0 59.541534 -31.279999 107.80951 -69.865845 107.80951l0 "
        "0c-38.585846 0 -69.865845 -48.267975 -69.865845 -107.80951zm15.858154 "
        "0l0 0c0 50.783325 24.180054 91.951355 54.00769 91.951355c29.827637 0 "
        "54.00769 -41.16803 54.00769 -91.951355l0 0c0 -50.783356 -24.180054 "
        "-91.95137 -54.00769 -91.95137l0 0c-29.827637 0 -54.00769 41.168015 "
        "-54.00769 91.95137z\" fill-rule=\"evenodd\"/><path fill=\"#a68b63\" "
        "d=\"m350.12894 170.72566l0 0c0 -27.52063 22.309875 -49.830513 "
        "49.830536 -49.830513l0 0l0 0c13.215851 0 25.890442 5.2499847 "
        "35.235474 14.595024c9.345032 9.345032 14.595032 22.019608 14.595032 "
        "35.23549l0 32.227585c0 27.520645 -22.309875 49.83052 -49.830505 "
        "49.83052l0 0l0 0c-27.52066 0 -49.830536 -22.309875 -49.830536 "
        "-49.83052z\" fill-rule=\"evenodd\"/><path fill=\"#c4a575\" "
        "d=\"m364.14603 171.58035l0 0c0 -19.778717 16.033813 -35.81253 35.8125 "
        "-35.81253l0 0l0 0c9.498077 0 18.607147 3.7731018 25.323303 "
        "10.489243c6.7161255 6.716156 10.489227 15.825211 10.489227 "
        "25.323288l0 60.26358c0 19.778702 -16.033813 35.81253 -35.81253 "
        "35.81253l0 0l0 0c-19.778687 0 -35.8125 -16.033829 -35.8125 "
        "-35.81253z\" fill-rule=\"evenodd\"/><path fill=\"#a68b63\" "
        "d=\"m393.34647 194.77385l0 0c0 -3.6616058 2.9682922 -6.6299133 "
        "6.6299133 -6.6299133l0 0l0 0c1.7583618 0 3.4447021 0.6985016 "
        "4.6880493 1.9418488c1.2433777 1.2433624 1.941864 2.9297028 1.941864 "
        "4.6880646l0 54.33072c0 3.6616058 -2.9683228 6.6299133 -6.6299133 "
        "6.6299133l0 0l0 0c-3.661621 0 -6.6299133 -2.9683075 -6.6299133 "
        "-6.6299133z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_trex_body_line);
    rr_renderer_set_dimensions(&this->mob_trex_body_line, 800, 600);
    rr_renderer_draw_svg(
        &this->mob_trex_body_line,
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m393.34647 "
        "247.45406l0 0c0 -3.6615906 2.9682922 -6.6299133 6.6299133 "
        "-6.6299133l0 0l0 0c1.7583618 0 3.4447021 0.6985016 4.6880493 "
        "1.941864c1.2433777 1.2433472 1.941864 2.9296875 1.941864 4.6880493l0 "
        "104.976395c0 3.6615906 -2.9683228 6.6299133 -6.6299133 6.6299133l0 "
        "0l0 0c-3.661621 0 -6.6299133 -2.9683228 -6.6299133 -6.6299133z\" "
        "fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_trex_tail_bottom);
    rr_renderer_set_dimensions(&this->mob_trex_tail_bottom, 800, 600);
    rr_renderer_draw_svg(
        &this->mob_trex_tail_bottom,
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m452.13782 "
        "366.3936l-35.899292 181.47806l-32.544678 0l-35.899292 -181.47806z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m346.26303 "
        "352.96597l0 0c0 -34.214844 24.040436 -61.951447 53.69583 -61.951447l0 "
        "0c14.241028 0 27.898773 6.527008 37.96869 18.145172c10.069916 "
        "11.618134 15.727142 27.375732 15.727142 43.806274l0 0c0 34.214813 "
        "-24.040436 61.951416 -53.69583 61.951416l0 0c-29.655396 0 -53.69583 "
        "-27.736603 -53.69583 -61.951416z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#c4a575\" d=\"m362.78302 352.9578l0 0c0 -23.683838 16.641022 "
        "-42.883392 37.168762 -42.883392l0 0c9.857788 0 19.311829 4.5180664 "
        "26.282318 12.560272c6.9704895 8.042175 10.886475 18.949738 10.886475 "
        "30.32312l0 0c0 23.683838 -16.641022 42.883392 -37.168793 42.883392l0 "
        "0c-20.52774 0 -37.168762 -19.199554 -37.168762 -42.883392z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#a68b63\" d=\"m383.5574 "
        "545.11273l0 0c0 -10.450134 7.3425903 -18.92163 16.400146 -18.92163l0 "
        "0c4.3496094 0 8.521057 1.9935303 11.59668 5.542053c3.0756226 3.548462 "
        "4.8034973 8.361267 4.8034973 13.379578l0 0c0 10.450134 -7.342621 "
        "18.92163 -16.400177 18.92163l0 0c-9.057556 0 -16.400146 -8.471497 "
        "-16.400146 -18.92163z\" fill-rule=\"evenodd\"/><path fill=\"#c4a575\" "
        "d=\"m435.77188 364.40717l-35.815582 183.17505l-35.815582 "
        "-183.17505z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_trex_tail_top);
    rr_renderer_set_dimensions(&this->mob_trex_tail_top, 800, 600);
    rr_renderer_draw_svg(
        &this->mob_trex_tail_top,
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#c4a575\" d=\"m362.78302 "
        "352.9578l0 0c0 -23.683838 16.641022 -42.883392 37.168762 -42.883392l0 "
        "0c9.857788 0 19.311829 4.5180664 26.282318 12.560272c6.9704895 "
        "8.042175 10.886475 18.949738 10.886475 30.32312l0 0c0 23.683838 "
        "-16.641022 42.883392 -37.168793 42.883392l0 0c-20.52774 0 -37.168762 "
        "-19.199554 -37.168762 -42.883392z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#c4a575\" d=\"m435.77188 364.40717l-35.815582 "
        "183.17505l-35.815582 -183.17505z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#a68b63\" d=\"m393.34647 351.39896l0 0c0 -3.661621 2.9682922 "
        "-6.6299133 6.6299133 -6.6299133l0 0l0 0c1.7583618 0 3.4447021 "
        "0.6984863 4.6880493 1.9418335c1.2433777 1.2433777 1.941864 2.929718 "
        "1.941864 4.68808l0 197.5748c0 3.661621 -2.9683228 6.629944 -6.6299133 "
        "6.629944l0 0l0 0c-3.661621 0 -6.6299133 -2.9683228 -6.6299133 "
        "-6.629944z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_triceratops[6]);
    rr_renderer_set_dimensions(&this->mob_triceratops[6], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_triceratops[6],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m485.06863 "
        "202.43504l0 0c-6.953583 -33.520355 -29.766357 -57.63098 -50.953827 "
        "-53.8526l0 0c-10.174561 1.8144379 -18.605896 9.949097 -23.439209 "
        "22.614426c-4.8333435 12.665329 -5.672699 28.823868 -2.333496 "
        "44.9209l0 0c6.953583 33.520355 29.766357 57.631012 50.953796 "
        "53.85263l0 0c21.18747 -3.7783813 32.726288 -34.014984 25.772736 "
        "-67.535355z\" fill-rule=\"evenodd\"/><path fill=\"#898276\" "
        "d=\"m482.174 235.9138l0 0c-1.6950073 -21.378479 -21.825867 -37.467407 "
        "-44.963593 -35.93564l0 0c-23.137695 1.531784 -40.520447 20.104218 "
        "-38.82547 41.482697l0 0c1.6950073 21.378494 21.825897 37.467422 "
        "44.963593 35.935623l0 0c23.137726 -1.5317688 40.520477 -20.104187 "
        "38.82547 -41.48268z\" fill-rule=\"evenodd\"/><path fill=\"#898276\" "
        "d=\"m314.93427 202.43819l0 0c6.9542847 -33.521576 29.768097 "
        "-57.632507 50.956055 -53.85327l0 0c10.174805 1.8148651 18.606232 "
        "9.950134 23.439484 22.61618c4.833252 12.666031 5.672394 28.825287 "
        "2.3328247 44.922913l0 0c-6.9542847 33.521576 -29.768097 57.632492 "
        "-50.956055 53.853256l0 0c-21.187958 -3.7792358 -32.726593 -34.0175 "
        "-25.772308 -67.53908z\" fill-rule=\"evenodd\"/><path fill=\"#898276\" "
        "d=\"m317.82944 235.91829l0 0c1.6950378 -21.380447 21.826538 "
        "-37.470108 44.964935 -35.93727l0 0c23.138397 1.5328369 40.521698 "
        "20.107727 38.82666 41.48816l0 0c-1.6950378 21.380463 -21.826508 "
        "37.470123 -44.964935 35.937286l0 0c-23.138397 -1.5328369 -40.521698 "
        "-20.107727 -38.82666 -41.488174z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#898276\" d=\"m388.9561 266.44037l0 0c0 -14.176102 4.875244 "
        "-25.668106 10.88913 -25.668106c6.0138855 0 10.88913 11.492004 "
        "10.88913 25.668106l0 0c-6.9535522 -7.025177 -14.824707 -7.025177 "
        "-21.77826 0z\" fill-rule=\"evenodd\"/><path fill=\"#898276\" "
        "d=\"m368.70175 71.605125l0 0c11.142853 4.1062927 15.321747 20.611313 "
        "9.333832 36.86499c-5.9879456 16.253677 -19.875214 26.101082 "
        "-31.018066 21.99479l0 0c12.724091 -16.545746 20.632599 -38.01258 "
        "21.684235 -58.85978z\" fill-rule=\"evenodd\"/><path fill=\"#898276\" "
        "d=\"m431.29953 71.60513l0 0c-11.142883 4.1063004 -15.321747 20.61132 "
        "-9.333832 36.864998c5.9879456 16.253677 19.875214 26.101067 31.018066 "
        "21.994774l0 0c-12.724091 -16.545738 -20.632599 -38.012573 -21.684235 "
        "-58.85977z\" fill-rule=\"evenodd\"/><path fill=\"#898276\" "
        "d=\"m333.88748 170.9469l0 0c0 -37.235962 29.62561 -67.42168 66.170654 "
        "-67.42168l0 0c17.54956 0 34.38031 7.1033325 46.789734 "
        "19.747353c12.409393 12.64402 19.38092 29.792976 19.38092 47.674324l0 "
        "0c0 37.235962 -29.62561 67.42168 -66.170654 67.42168l0 0c-36.545044 0 "
        "-66.170654 -30.185715 -66.170654 -67.42168z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m368.29858 "
        "76.69052l0 0c0 -17.841312 14.193146 -32.304554 31.701233 -32.304554l0 "
        "0c8.407715 0 16.471039 3.4035034 22.416168 9.461784c5.9451294 "
        "6.058281 9.285065 14.275074 9.285065 22.84277l0 0c0 17.841316 "
        "-14.193115 32.304558 -31.701233 32.304558l0 0c-17.508087 0 -31.701233 "
        "-14.463242 -31.701233 -32.304558z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#a49b8a\" d=\"m381.93546 76.69052l0 0c0 -10.162834 8.0876465 "
        "-18.40144 18.06427 -18.40144l0 0c4.7909546 0 9.385681 1.9387169 "
        "12.773407 5.389656c3.3876953 3.4509392 5.2908936 8.13142 5.2908936 "
        "13.011784l0 0c0 10.162842 -8.0876465 18.401443 -18.0643 18.401443l0 "
        "0c-9.976624 0 -18.06427 -8.238602 -18.06427 -18.401443z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" d=\"m347.01355 "
        "170.9469l0 0c0 -29.82698 23.722748 -54.00654 52.986237 -54.00654l0 "
        "0c14.052795 0 27.53006 5.6899567 37.46689 15.818153c9.936859 "
        "10.128189 15.519318 23.86496 15.519318 38.188385l0 0c0 29.826996 "
        "-23.722748 54.00656 -52.986206 54.00656l0 0c-29.263489 0 -52.986237 "
        "-24.179565 -52.986237 -54.00656z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#a49b8a\" d=\"m381.79868 76.44061l0 0c9.643433 3.5574875 "
        "12.606781 19.617607 6.6188354 35.871284c-5.9879456 16.25367 "
        "-18.659668 26.545967 -28.3031 22.988487l0 0c12.582489 -16.25978 "
        "20.708893 -38.318092 21.684265 -58.85977z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" d=\"m381.79868 "
        "76.44061l0 0c9.643433 3.5574875 12.606781 19.617607 6.6188354 "
        "35.871284c-5.9879456 16.25367 -18.659668 26.545967 -28.3031 "
        "22.988487l0 0c12.582489 -16.25978 20.708893 -38.318092 21.684265 "
        "-58.85977z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" "
        "d=\"m418.17267 76.44061l0 0c-9.643402 3.5574875 -12.606781 19.617607 "
        "-6.6188354 35.871284c5.9879456 16.25367 18.659668 26.545982 28.3031 "
        "22.988487l0 0c-12.582489 -16.25978 -20.708893 -38.318092 -21.684265 "
        "-58.85977z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" "
        "d=\"m418.17267 76.44061l0 0c-9.643402 3.5574875 -12.606781 19.617607 "
        "-6.6188354 35.871284c5.9879456 16.25367 18.659668 26.545982 28.3031 "
        "22.988487l0 0c-12.582489 -16.25978 -20.708893 -38.318092 -21.684265 "
        "-58.85977z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" "
        "d=\"m382.88553 120.686l10.700928 -35.40104l12.828003 0l10.700958 "
        "35.40104z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" "
        "d=\"m330.6247 204.40997l0 0c6.545807 -25.851257 22.978271 -44.324356 "
        "36.702972 -41.260803l0 0c6.590851 1.4711761 11.663025 7.813614 "
        "14.100739 17.632065c2.4377136 9.818451 2.0412598 22.308624 -1.1021423 "
        "34.72284l0 0c-6.5457764 25.851273 -22.978271 44.324356 -36.702972 "
        "41.26082l0 0c-13.724701 -3.0635529 -19.544373 -26.503647 -12.998596 "
        "-52.35492z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" "
        "d=\"m331.21335 236.80766l0 0c1.1335144 -14.281265 14.805176 "
        "-25.014114 30.536499 -23.972504l0 0c15.731323 1.0416107 27.565216 "
        "13.463242 26.431702 27.744492l0 0c-1.1335144 14.28125 -14.805176 "
        "25.0141 -30.53653 23.972504l0 0c-15.731323 -1.0415955 -27.565186 "
        "-13.463242 -26.431671 -27.744492z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#a49b8a\" d=\"m381.17923 255.85406l0 0c0 -29.205841 8.356964 "
        "-52.881866 18.665802 -52.881866c10.308868 0 18.665833 23.676025 "
        "18.665833 52.881866l0 0c-12.141327 -10.994492 -25.190308 -10.994492 "
        "-37.331635 0z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" "
        "d=\"m469.4979 204.40952l0 0c-6.5457764 -25.851273 -22.978271 "
        "-44.32437 -36.702972 -41.26082l0 0c-6.590851 1.4711761 -11.663025 "
        "7.813614 -14.100739 17.632065c-2.4377136 9.818451 -2.0412598 "
        "22.308624 1.1021423 34.722855l0 0c6.545807 25.851257 22.978271 "
        "44.324356 36.702972 41.260788l0 0c13.724731 -3.0635376 19.544403 "
        "-26.503632 12.998596 -52.35489z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#a49b8a\" d=\"m468.90927 236.80719l0 0c-1.1335144 -14.28125 "
        "-14.805176 -25.014114 -30.53653 -23.972504l0 0c-15.731323 1.0416107 "
        "-27.565186 13.463242 -26.431671 27.744492l0 0c1.1335144 14.28125 "
        "14.805176 25.014114 30.536499 23.972519l0 0c15.731323 -1.041626 "
        "27.565216 -13.463257 26.431702 -27.744507z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" d=\"m369.18845 "
        "230.46399l11.531891 -36.88788l38.249634 0l11.531891 36.88788z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#bfbab0\" d=\"m392.59192 "
        "87.54464l0 0c-4.0915833 -5.6743317 -4.0915833 -14.874245 0 "
        "-20.548576q3.7042542 -5.137142 7.408478 -10.274284q3.7042542 5.137142 "
        "7.4085083 10.274284l0 0c4.0915833 5.6743317 4.0915833 14.874245 0 "
        "20.548576l0 0c-4.091614 5.6743317 -10.725403 5.6743317 -14.816986 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m442.21332 "
        "245.71992l0 0c-20.200531 -6.4423676 -33.928955 -25.192368 -33.96982 "
        "-46.39528l14.549072 -0.028045654c0.028686523 14.881058 9.663849 "
        "28.040558 23.8414 32.562057z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#898276\" d=\"m408.16977 199.23111l0 0c0 -3.9949799 3.238556 "
        "-7.233551 7.233551 -7.233551l0 0c1.918457 0 3.7583313 0.7621155 "
        "5.1148987 2.1186676c1.3565369 1.3565521 2.1186523 3.1964264 2.1186523 "
        "5.1148834l0 0c0 3.9949799 -3.2385864 7.233551 -7.233551 7.233551l0 "
        "0c-3.994995 0 -7.233551 -3.2385712 -7.233551 -7.233551z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m437.93234 "
        "238.95361l0 0c0 -3.9949799 3.238556 -7.233551 7.233551 -7.233551l0 "
        "0c1.918457 0 3.7583313 0.7621155 5.114868 2.1186676c1.3565674 "
        "1.3565521 2.1186523 3.1964264 2.1186523 5.1148834l0 0c0 3.9949799 "
        "-3.238556 7.233551 -7.2335205 7.233551l0 0c-3.994995 0 -7.233551 "
        "-3.2385712 -7.233551 -7.233551z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#898276\" d=\"m357.78513 245.71991l0 0c20.200531 -6.4423676 "
        "33.928955 -25.192368 33.96985 -46.39528l-14.549103 "
        "-0.028045654c-0.028686523 14.881058 -9.663849 28.040558 -23.84137 "
        "32.562057z\" fill-rule=\"evenodd\"/><path fill=\"#898276\" "
        "d=\"m391.82867 199.2311l0 0c0 -3.9949799 -3.238556 -7.233551 "
        "-7.233551 -7.233551l0 0c-1.918457 0 -3.7583313 0.7621002 -5.114868 "
        "2.1186676c-1.3565674 1.3565521 -2.1186523 3.1964264 -2.1186523 "
        "5.1148834l0 0c0 3.9949799 3.238556 7.233536 7.2335205 7.233536l0 "
        "0c3.994995 0 7.233551 -3.238556 7.233551 -7.233536z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m362.06613 "
        "238.9536l0 0c0 -3.9949799 -3.2385864 -7.233551 -7.233551 -7.233551l0 "
        "0c-1.918457 0 -3.7583313 0.7621155 -5.1148987 2.1186676c-1.3565369 "
        "1.3565521 -2.1186523 3.1964264 -2.1186523 5.1148834l0 0c0 3.9949799 "
        "3.238556 7.233551 7.233551 7.233551l0 0c3.9949646 0 7.233551 "
        "-3.2385712 7.233551 -7.233551z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#bfbab0\" d=\"m421.43738 140.30916c-0.03942871 -7.8772583 "
        "13.839783 -16.676247 15.418823 -29.289406c1.5790405 -12.613167 "
        "-8.070465 -29.421265 -5.9445496 -46.389557c2.125946 -16.9683 "
        "15.511017 -55.57722 18.700165 -55.420227c3.189148 0.15699291 "
        "-1.2475586 38.18477 0.43475342 56.362183c1.682312 18.177414 11.818085 "
        "37.25045 9.659119 52.7023c-2.158966 15.451851 -16.234833 36.33634 "
        "-22.612885 40.008797c-6.3780518 3.6724396 -15.615997 -10.096848 "
        "-15.655426 -17.97409z\" fill-rule=\"evenodd\"/><path fill=\"#bfbab0\" "
        "d=\"m378.68457 140.30934c0.03942871 -7.877243 -13.839783 -16.676239 "
        "-15.418823 -29.289398c-1.579071 -12.613167 8.070465 -29.421265 "
        "5.944519 -46.389565c-2.125946 -16.968292 -15.510986 -55.577213 "
        "-18.700134 -55.42022c-3.189148 0.15699291 1.2475586 38.18477 "
        "-0.43475342 56.362183c-1.682312 18.177414 -11.818085 37.25045 "
        "-9.659149 52.702293c2.158966 15.451851 16.234863 36.33634 22.612915 "
        "40.008797c6.3780518 3.6724548 15.615967 -10.096832 15.655426 "
        "-17.97409z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_triceratops[5]);
    rr_renderer_set_dimensions(&this->mob_triceratops[5], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_triceratops[5],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m391.06537 "
        "273.96182l0 0c0 -4.940094 4.0047607 -8.944855 8.944885 -8.944855l0 "
        "0l0 0c2.372345 0 4.647522 0.9423828 6.325012 2.619873c1.6774902 "
        "1.6774902 2.619873 3.9526672 2.619873 6.3249817l0 99.811035c0 "
        "4.9401245 -4.0047607 8.944885 -8.944885 8.944885l0 0l0 0c-4.9401245 0 "
        "-8.944885 -4.0047607 -8.944885 -8.944885z\" "
        "fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_triceratops[4]);
    rr_renderer_set_dimensions(&this->mob_triceratops[4], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_triceratops[4],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" d=\"m362.8296 "
        "379.71463l0 0c0 -23.683838 16.641022 -42.88336 37.168762 -42.88336l0 "
        "0c9.857788 0 19.311829 4.518036 26.282318 12.560242c6.9704895 "
        "8.042206 10.886475 18.949738 10.886475 30.32312l0 0c0 23.683868 "
        "-16.641022 42.883392 -37.168793 42.883392l0 0c-20.52774 0 -37.168762 "
        "-19.199524 -37.168762 -42.883392z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#a49b8a\" d=\"m435.81845 391.16403l-35.815582 "
        "183.17502l-35.815582 -183.17502z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#898276\" d=\"m391.06552 377.38177l0 0c0 -4.9401245 4.0047607 "
        "-8.944885 8.944885 -8.944885l0 0l0 0c2.372345 0 4.647522 0.94241333 "
        "6.325012 2.6199036c1.6774902 1.6774902 2.619873 3.9526672 2.619873 "
        "6.3249817l0 192.94489c0 4.9401245 -4.0047607 8.944885 -8.944885 "
        "8.944885l0 0l0 0c-4.9401245 0 -8.944885 -4.0047607 -8.944885 "
        "-8.944885z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_triceratops[3]);
    rr_renderer_set_dimensions(&this->mob_triceratops[3], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_triceratops[3],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" d=\"m322.638 "
        "324.62808l0 0c0 -65.93576 34.637024 -119.38727 77.36398 -119.38727l0 "
        "0c20.518188 0 40.196014 12.578262 54.70459 34.967728c14.508545 "
        "22.38945 22.659393 52.75609 22.659393 84.41954l0 0c0 65.93579 "
        "-34.637054 119.3873 -77.36398 119.3873l0 0c-42.72696 0 -77.36398 "
        "-53.451508 -77.36398 -119.3873z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#898276\" d=\"m312.2113 324.63947l0 0c0 -74.82852 39.308533 "
        "-135.48903 87.798035 -135.48903l0 0c23.285492 0 45.61728 14.274689 "
        "62.08261 39.683823c16.465332 25.409119 25.715454 59.871277 25.715454 "
        "95.80521l0 0c0 74.82852 -39.308533 135.48904 -87.798065 135.48904l0 "
        "0c-48.489502 0 -87.798035 -60.660522 -87.798035 -135.48904zm19.928406 "
        "0l0 0c0 63.822388 30.386261 115.56064 67.86963 115.56064c37.483368 0 "
        "67.86966 -51.73825 67.86966 -115.56064l0 0c0 -63.822357 -30.386292 "
        "-115.56062 -67.86966 -115.56062l0 0c-37.483368 0 -67.86963 51.738266 "
        "-67.86963 115.56062z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_triceratops[2]);
    rr_renderer_set_dimensions(&this->mob_triceratops[2], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_triceratops[2],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m452.1844 "
        "393.15045l-35.899292 181.47803l-32.544678 0l-35.899292 -181.47803z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m346.3096 "
        "379.72916l0 0c0 -34.218353 24.042908 -61.957764 53.701324 "
        "-61.957764l0 0c14.242493 0 27.901611 6.5276794 37.972565 "
        "18.147003c10.070953 11.619324 15.72876 27.37854 15.72876 43.81076l0 "
        "0c0 34.218323 -24.042908 61.957733 -53.701324 61.957733l0 "
        "0c-29.658417 0 -53.701324 -27.73941 -53.701324 -61.957733z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m362.8296 "
        "379.71463l0 0c0 -23.683838 16.641022 -42.88336 37.168762 -42.88336l0 "
        "0c9.857788 0 19.311829 4.518036 26.282318 12.560242c6.9704895 "
        "8.042206 10.886475 18.949738 10.886475 30.32312l0 0c0 23.683868 "
        "-16.641022 42.883392 -37.168793 42.883392l0 0c-20.52774 0 -37.168762 "
        "-19.199524 -37.168762 -42.883392z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#898276\" d=\"m383.60397 571.8696l0 0c0 -10.450134 7.3425903 "
        "-18.921692 16.400146 -18.921692l0 0c4.3496094 0 8.521057 1.9935303 "
        "11.59668 5.542053c3.0756226 3.548462 4.8034973 8.361267 4.8034973 "
        "13.379639l0 0c0 10.450073 -7.342621 18.92163 -16.400177 18.92163l0 "
        "0c-9.057556 0 -16.400146 -8.471558 -16.400146 -18.92163z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m435.81845 "
        "391.16403l-35.815582 183.17502l-35.815582 -183.17502z\" "
        "fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_triceratops[1]);
    rr_renderer_set_dimensions(&this->mob_triceratops[1], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_triceratops[1],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" d=\"m297.58875 "
        "248.20563l0 0c0 -20.723007 16.476898 -37.522324 36.802185 "
        "-37.522324l0 0c9.760529 0 19.121307 3.9532318 26.02304 "
        "10.990036c6.901764 7.036804 10.779114 16.58075 10.779114 26.532288l0 "
        "0c0 20.722992 -16.476898 37.52231 -36.802155 37.52231l0 0c-20.325287 "
        "0 -36.802185 -16.799316 -36.802185 -37.52231z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m292.62875 "
        "248.2092l0 0c0 -23.517914 18.699127 -42.582947 41.765686 -42.582947l0 "
        "0c11.076935 0 21.700195 4.4864044 29.532776 12.4722595c7.8325806 "
        "7.985855 12.23288 18.816986 12.23288 30.110687l0 0c0 23.517914 "
        "-18.699127 42.582947 -41.765656 42.582947l0 0c-23.066559 0 -41.765686 "
        "-19.065033 -41.765686 -42.582947zm11.589966 0l0 0c0 17.116943 "
        "13.510132 30.992981 30.17572 30.992981c16.665558 0 30.17569 "
        "-13.876038 30.17569 -30.992981c0 -17.116943 -13.510132 -30.992981 "
        "-30.17569 -30.992981l0 0c-16.665588 0 -30.17572 13.876038 -30.17572 "
        "30.992981z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" "
        "d=\"m424.80817 381.81046l0 0c0 -20.722992 16.476898 -37.52231 "
        "36.802185 -37.52231l0 0c9.760529 0 19.121307 3.9532166 26.02304 "
        "10.990021c6.901764 7.036804 10.779114 16.58075 10.779114 26.532288l0 "
        "0c0 20.722992 -16.476898 37.52231 -36.802155 37.52231l0 0c-20.325287 "
        "0 -36.802185 -16.799316 -36.802185 -37.52231z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m419.84818 "
        "381.81403l0 0c0 -23.517914 18.699127 -42.582947 41.765686 "
        "-42.582947l0 0c11.076935 0 21.700195 4.486389 29.532776 "
        "12.4722595c7.8325806 7.98584 12.23288 18.816986 12.23288 30.110687l0 "
        "0c0 23.517914 -18.699127 42.582947 -41.765656 42.582947l0 "
        "0c-23.066559 0 -41.765686 -19.065033 -41.765686 -42.582947zm11.589966 "
        "0l0 0c0 17.116943 13.510132 30.992981 30.17572 30.992981c16.665558 0 "
        "30.17569 -13.876038 30.17569 -30.992981c0 -17.116943 -13.510132 "
        "-30.992981 -30.17569 -30.992981l0 0c-16.665588 0 -30.17572 13.876038 "
        "-30.17572 30.992981z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_triceratops[0]);
    rr_renderer_set_dimensions(&this->mob_triceratops[0], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_triceratops[0],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" d=\"m428.79636 "
        "248.20563l0 0c0 -20.723007 16.476898 -37.522324 36.802185 "
        "-37.522324l0 0c9.760529 0 19.121307 3.9532318 26.02304 "
        "10.990036c6.901764 7.036804 10.779114 16.58075 10.779114 26.532288l0 "
        "0c0 20.722992 -16.476898 37.52231 -36.802155 37.52231l0 0c-20.325287 "
        "0 -36.802185 -16.799316 -36.802185 -37.52231z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m423.83636 "
        "248.2092l0 0c0 -23.517914 18.699127 -42.582947 41.765686 -42.582947l0 "
        "0c11.076935 0 21.700195 4.4864044 29.532776 12.4722595c7.8325806 "
        "7.985855 12.23288 18.816986 12.23288 30.110687l0 0c0 23.517914 "
        "-18.699127 42.582947 -41.765656 42.582947l0 0c-23.066559 0 -41.765686 "
        "-19.065033 -41.765686 -42.582947zm11.589966 0l0 0c0 17.116943 "
        "13.510132 30.992981 30.17572 30.992981c16.665558 0 30.17569 "
        "-13.876038 30.17569 -30.992981c0 -17.116943 -13.510132 -30.992981 "
        "-30.17569 -30.992981l0 0c-16.665588 0 -30.17572 13.876038 -30.17572 "
        "30.992981z\" fill-rule=\"evenodd\"/><path fill=\"#a49b8a\" "
        "d=\"m301.60056 381.81046l0 0c0 -20.722992 16.476898 -37.52231 "
        "36.802185 -37.52231l0 0c9.760529 0 19.121307 3.9532166 26.02304 "
        "10.990021c6.901764 7.036804 10.779114 16.58075 10.779114 26.532288l0 "
        "0c0 20.722992 -16.476898 37.52231 -36.802155 37.52231l0 0c-20.325287 "
        "0 -36.802185 -16.799316 -36.802185 -37.52231z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#898276\" d=\"m296.64056 "
        "381.81403l0 0c0 -23.517914 18.699127 -42.582947 41.765686 "
        "-42.582947l0 0c11.076935 0 21.700195 4.486389 29.532776 "
        "12.4722595c7.8325806 7.98584 12.23288 18.816986 12.23288 30.110687l0 "
        "0c0 23.517914 -18.699127 42.582947 -41.765656 42.582947l0 "
        "0c-23.066559 0 -41.765686 -19.065033 -41.765686 -42.582947zm11.589966 "
        "0l0 0c0 17.116943 13.510132 30.992981 30.17572 30.992981c16.665558 0 "
        "30.17569 -13.876038 30.17569 -30.992981c0 -17.116943 -13.510132 "
        "-30.992981 -30.17569 -30.992981l0 0c-16.665588 0 -30.17572 13.876038 "
        "-30.17572 30.992981z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_dakotaraptor[0]);
    rr_renderer_set_dimensions(&this->mob_dakotaraptor[0], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_dakotaraptor[0],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m527.91205 "
        "334.14203l-3.973755 18.351685l-10.643005 1.6325684l-7.084015 "
        "-3.5590515l-6.3726807 -9.086639l1.8766174 -6.9474792l-7.643036 "
        "-1.671936l-7.2886963 -13.51181l3.333313 -6.464569l-6.803131 "
        "-1.792633l-11.40683 -21.477692l-31.84253 15.157471l35.391083 "
        "-66.08923z\" fill-rule=\"evenodd\"/><path fill=\"#596a74\" "
        "d=\"m458.37927 286.59348l15.979004 -7.4724426l14.175842 "
        "27.317566l9.572174 4.126007l-4.4199524 9.278198l3.6089478 "
        "7.7296753l11.120728 2.322815l-2.8740234 11.595795l3.6089172 "
        "4.6378174l5.6719055 2.577423l4.380615 -1.0288696l4.123352 "
        "-16.753296l-37.370087 -74.22571l-12.629913 -2.0630035l-16.236237 "
        "15.7217865z\" fill-rule=\"evenodd\"/><path fill=\"#6e7a82\" "
        "d=\"m527.41187 338.64587c-3.3118896 8.370514 -28.894104 -66.216095 "
        "-41.58789 -76.2782c-12.693787 -10.062134 -30.304474 20.91687 -34.5748 "
        "15.905487c-4.270355 -5.0113525 -0.12161255 -34.9523 8.952728 "
        "-45.97374c9.074371 -11.021439 34.29178 -37.879257 45.49344 "
        "-20.154861c11.201691 17.724411 25.028473 118.1308 21.716522 "
        "126.50131z\" fill-rule=\"evenodd\"/><path fill=\"#6e7a82\" "
        "d=\"m428.07562 257.6345l0 0c-6.6708984 -5.1140747 -7.276428 "
        "-14.094086 -1.352478 -20.057434l49.6026 -49.93239c2.8447876 "
        "-2.8636932 6.8456116 -4.6345215 11.1223755 -4.9228973c4.2767944 "
        "-0.28837585 8.4791565 0.9292908 11.682617 3.3851624l0 0l0 0c6.6708984 "
        "5.1140594 7.276428 14.094086 1.352478 20.057419l-49.60257 "
        "49.93239c-5.9239807 5.9633484 -16.134125 6.651825 -22.805023 "
        "1.5377502z\" fill-rule=\"evenodd\"/><path fill=\"#6e7a82\" "
        "d=\"m485.9651 216.58777c3.7287903 22.788269 37.932617 117.96326 "
        "41.044617 114.20996c3.1119995 -3.7532654 -15.531921 -117.69466 "
        "-22.37268 -136.72964c-6.84079 -19.035004 -22.400696 -0.26860046 "
        "-18.671936 22.519684z\" fill-rule=\"evenodd\"/><path fill=\"#85929c\" "
        "d=\"m430.22177 261.66568l0 0c-4.8705444 -3.7311707 -5.314026 "
        "-10.284988 -0.99057007 -14.638336l48.122894 -48.455597c2.0762024 "
        "-2.090561 4.9964905 -3.3836975 8.118439 -3.5949707c3.1219788 "
        "-0.21125793 6.18985 0.67666626 8.528778 2.4684448l0 0l0 0c4.8705444 "
        "3.7311707 5.314026 10.284988 0.99057007 14.638336l-48.122894 "
        "48.455597c-4.323456 4.353363 -11.776672 4.857727 -16.647217 "
        "1.1265259z\" fill-rule=\"evenodd\"/><path fill=\"#85929c\" "
        "d=\"m497.6797 201.79991c7.216522 12.671921 21.175842 88.1006 "
        "19.070862 96.39107c-2.1049805 8.290466 -19.75943 -42.09491 -31.700806 "
        "-46.648285c-11.941376 -4.5533752 -38.014435 24.22528 -39.94751 "
        "19.328094c-1.933075 -4.897217 19.586182 -37.199493 28.34909 "
        "-48.711304c8.762909 -11.51181 17.01181 -33.031494 24.228363 "
        "-20.359573z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_dakotaraptor[1]);
    rr_renderer_set_dimensions(&this->mob_dakotaraptor[1], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_dakotaraptor[1],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m272.08707 "
        "334.144l3.973755 18.351715l10.643036 1.6325378l7.084015 "
        "-3.5590515l6.3726807 -9.086609l-1.8766174 -6.94751l7.643036 "
        "-1.6719055l7.2886963 -13.51181l-3.333313 -6.464569l6.803131 "
        "-1.7926636l11.40683 -21.477692l31.84253 15.157501l-35.391083 "
        "-66.08925z\" fill-rule=\"evenodd\"/><path fill=\"#596a74\" "
        "d=\"m341.61987 286.59546l-15.979004 -7.4724426l-14.175842 "
        "27.317596l-9.572174 4.1259766l4.4199524 9.278229l-3.6089478 "
        "7.729645l-11.120728 2.3228455l2.8740234 11.595795l-3.6089172 "
        "4.637787l-5.671936 2.577423l-4.380554 -1.0288696l-4.1233826 "
        "-16.753265l37.370087 -74.22574l12.629913 -2.0629883l16.236237 "
        "15.721802z\" fill-rule=\"evenodd\"/><path fill=\"#6e7a82\" "
        "d=\"m272.58725 338.6479c3.3118896 8.370514 28.894135 -66.216095 "
        "41.58792 -76.27823c12.693787 -10.062119 30.304474 20.9169 34.5748 "
        "15.905518c4.270355 -5.011383 0.12161255 -34.952316 -8.952728 "
        "-45.973755c-9.074371 -11.021439 -34.29178 -37.879257 -45.49344 "
        "-20.154846c-11.201691 17.724396 -25.028442 118.1308 -21.716553 "
        "126.50131z\" fill-rule=\"evenodd\"/><path fill=\"#6e7a82\" "
        "d=\"m371.92352 257.63647l0 0c6.6708984 -5.1140594 7.276428 -14.09407 "
        "1.352478 -20.057419l-49.6026 -49.93239c-2.8447876 -2.8636932 "
        "-6.8456116 -4.6345215 -11.1223755 -4.9228973c-4.2767944 -0.28837585 "
        "-8.4791565 0.9292908 -11.682617 3.3851624l0 0l0 0c-6.6708984 "
        "5.1140594 -7.276428 14.094086 -1.352478 20.057419l49.60257 "
        "49.932404c5.9239807 5.963318 16.134125 6.6517944 22.805023 "
        "1.5377197z\" fill-rule=\"evenodd\"/><path fill=\"#6e7a82\" "
        "d=\"m314.03406 216.58977c-3.7287903 22.788269 -37.932648 117.96324 "
        "-41.044617 114.20998c-3.1119995 -3.753296 15.531921 -117.69467 "
        "22.37268 -136.72966c6.84079 -19.035004 22.400726 -0.26860046 "
        "18.671936 22.519684z\" fill-rule=\"evenodd\"/><path fill=\"#85929c\" "
        "d=\"m369.77737 261.6677l0 0c4.8705444 -3.7312012 5.314026 -10.285004 "
        "0.99057007 -14.638351l-48.122894 -48.455597c-2.0762024 -2.090561 "
        "-4.9964905 -3.3836975 -8.118439 -3.5949707c-3.1219788 -0.21125793 "
        "-6.18985 0.67666626 -8.528778 2.4684448l0 0l0 0c-4.8705444 3.7311707 "
        "-5.314026 10.284988 -0.99057007 14.638336l48.122894 "
        "48.455612c4.323456 4.3533325 11.776672 4.8576965 16.647217 "
        "1.1265259z\" fill-rule=\"evenodd\"/><path fill=\"#85929c\" "
        "d=\"m302.3195 201.80191c-7.2165527 12.671921 -21.175873 88.10062 "
        "-19.070892 96.39108c2.105011 8.290466 19.75943 -42.09494 31.700806 "
        "-46.6483c11.941376 -4.5533752 38.014435 24.225296 39.94751 "
        "19.32808c1.933075 -4.8971863 -19.586182 -37.19948 -28.34909 "
        "-48.71129c-8.762909 -11.51181 -17.01181 -33.031494 -24.228333 "
        "-20.359573z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_dakotaraptor[2]);
    rr_renderer_set_dimensions(&this->mob_dakotaraptor[2], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_dakotaraptor[2],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m372.30493 "
        "533.1803l0 0c0 15.309326 12.410645 27.71997 27.71997 27.71997l0 "
        "0c7.3518066 0 14.402496 -2.9204712 19.600983 -8.1189575c5.198517 "
        "-5.1984863 8.119019 -12.249207 8.119019 -19.601013l0 0c0 -15.309326 "
        "-12.410675 -27.71997 -27.720001 -27.71997l0 0c-15.309326 0 -27.71997 "
        "12.410645 -27.71997 27.71997z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#46555e\" d=\"m390.61642 562.2273l0 0c0 5.1949463 4.2113037 "
        "9.40625 9.4062195 9.40625l0 0c2.49469 0 4.887207 -0.99102783 6.651245 "
        "-2.755066c1.7640076 -1.763977 2.755005 -4.156494 2.755005 -6.651184l0 "
        "0c0 -5.1949463 -4.2113037 -9.40625 -9.40625 -9.40625l0 0c-5.194916 0 "
        "-9.4062195 4.2113037 -9.4062195 9.40625z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m420.70642 "
        "551.11035l-13.223145 16.814148l-1.469635 -9.467773z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m379.22748 "
        "551.11035l13.223145 16.814148l1.469635 -9.467773z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m407.7576 "
        "518.5195l0 0c-6.4751587 13.873657 -22.9711 19.871277 -36.844727 "
        "13.396118l0 0c-6.6623535 -3.109497 -11.816589 -8.73822 -14.328857 "
        "-15.647949c-2.5122375 -6.909729 -2.1767273 -14.534424 0.9327698 "
        "-21.196777l0 0c6.4751587 -13.873627 22.9711 -19.871246 36.844727 "
        "-13.396088l0 0c13.873596 6.4751587 19.871246 22.9711 13.396088 "
        "36.844696z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m378.87778 537.09753l0 0c-2.197235 4.7077026 -7.7948 6.74292 "
        "-12.502533 4.5456543l0 0c-2.2607422 -1.0551147 -4.009735 -2.965149 "
        "-4.862213 -5.3098145c-0.85250854 -2.3446655 -0.73864746 -4.931946 "
        "0.3164978 -7.192688l0 0c2.1972046 -4.7077637 7.7948 -6.74292 "
        "12.502533 -4.5457153l0 0c4.7077637 2.1972046 6.7429504 7.7948 "
        "4.5457153 12.502563z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m356.31155 514.2964l4.87146 20.83014l5.336273 -7.958313z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m393.90063 "
        "531.84015l-19.094696 9.64447l2.6726074 -9.201416z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m392.10364 "
        "490.06427l0 0c-9.497559 12.011108 -26.933777 14.048737 -38.944885 "
        "4.551178l0 0c-5.7679443 -4.5608826 -9.4878235 -11.226257 -10.341339 "
        "-18.529846c-0.8535156 -7.303589 1.2292786 -14.647095 5.790161 "
        "-20.41504l0 0c9.497559 -12.011108 26.933777 -14.048737 38.944885 "
        "-4.551178l0 0c12.011108 9.497559 14.048737 26.933777 4.551178 "
        "38.944885z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m359.72617 501.48798l0 0c-3.2267456 4.0791016 -9.149292 4.77005 "
        "-13.228363 1.5433044l0 0c-1.9588318 -1.54953 -3.2218933 -3.8137207 "
        "-3.511322 -6.294525c-0.2894287 -2.4808044 0.41848755 -4.9749756 "
        "1.9680176 -6.9338074l0 0c3.2267456 -4.079071 9.149261 -4.77005 "
        "13.2283325 -1.5433044l0 0c4.0791016 3.2267456 4.77005 9.149261 "
        "1.543335 13.2283325z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m343.00696 474.11356l-0.05441284 21.388275l7.023651 -6.514984z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m375.5436 "
        "499.83698l-20.79834 4.989044l4.7181396 -8.337616z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m380.6604 457.053l0 "
        "0c-9.088776 12.324219 -26.447449 14.947052 -38.771667 5.8582764l0 "
        "0c-5.918274 -4.3645935 -9.860352 -10.901459 -10.958984 "
        "-18.172546c-1.0986633 -7.2710876 0.736145 -14.680817 5.1007385 "
        "-20.599121l0 0c9.088776 -12.324188 26.447418 -14.947052 38.771637 "
        "-5.858246l0 0c12.324219 9.088776 14.947052 26.447418 5.8582764 "
        "38.771637z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m348.68454 469.56186l0 0c-3.0875854 4.183441 -8.981903 5.071808 "
        "-13.165344 1.9842529l0 0c-2.0089722 -1.482727 -3.3466492 -3.7027588 "
        "-3.71875 -6.1717224c-0.37213135 -2.4689941 0.25180054 -4.98468 "
        "1.7344971 -6.9936523l0 0c3.0875854 -4.183441 8.981903 -5.071808 "
        "13.165344 -1.9842529l0 0c4.183441 3.0875854 5.0718384 8.981934 "
        "1.9842529 13.165375z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m331.0569 442.75787l0.6633911 21.381134l6.802063 -6.7479553z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m364.44168 "
        "467.3775l-20.622192 5.684906l4.436371 -8.492462z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m372.82925 "
        "393.65265l0 0c-9.088776 7.99292 -22.936218 7.1045227 -30.929138 "
        "-1.9842529l0 0c-3.8383179 -4.3645935 -5.785614 -10.075195 -5.4134827 "
        "-15.875519c0.37210083 -5.800354 3.0331726 -11.2153015 7.3977356 "
        "-15.053619l0 0c9.088776 -7.99292 22.936249 -7.1045227 30.929138 "
        "1.9842529l0 0c7.99292 9.088776 7.1045227 22.936218 -1.9842529 "
        "30.929138z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m346.02658 397.94427l0 0c-3.0875854 2.713562 -7.790344 2.4104004 "
        "-10.503937 -0.67718506l0 0c-1.3031311 -1.4826965 -1.9638672 "
        "-3.4223328 -1.836853 -5.392212c0.12698364 -1.9698792 1.031311 "
        "-3.8085938 2.5140076 -5.111725l0 0c3.0875854 -2.7135925 7.790344 "
        "-2.4104004 10.503937 0.67718506l0 0c2.7135925 3.087555 2.4104004 "
        "7.790344 -0.67715454 10.503937z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#46555e\" d=\"m336.91342 374.27344l-3.0836487 "
        "16.631683l6.3905945 -4.0697327z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#46555e\" d=\"m358.57 398.9078l-16.889893 0.92388916l4.85614 "
        "-5.8154907z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m371.01764 368.5804l0 0c-8.175537 4.827057 -18.716248 2.1125488 "
        "-23.543304 -6.0629883l0 0c-2.3180237 -3.926056 -2.9815063 -8.612122 "
        "-1.8444824 -13.027344c1.1370544 -4.415222 3.9814453 -8.197937 "
        "7.9074707 -10.515961l0 0c8.175568 -4.827057 18.716278 -2.1125793 "
        "23.543335 6.0629883l0 0c4.8270264 8.175537 2.1125488 18.716248 "
        "-6.063019 23.543304z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m349.7334 367.95917l0 0c-2.774475 1.6351013 -6.3491516 0.7114563 "
        "-7.984253 -2.0629883l0 0c-0.7852173 -1.332367 -1.0090027 -2.922058 "
        "-0.62210083 -4.419403c0.38687134 -1.497345 1.3527527 -2.779663 "
        "2.685089 -3.5648499l0 0c2.774475 -1.6351318 6.3491516 -0.7114868 "
        "7.984253 2.0629883l0 0c1.6351013 2.774475 0.7114868 6.349121 "
        "-2.0629883 7.984253z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m346.19315 348.3778l-4.816864 12.361908l5.521179 -2.197754z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m359.26227 "
        "370.53806l-13.149017 -1.7670288l4.5951233 -3.7680664z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m390.4238 "
        "518.51416l0 0c6.4751587 13.873596 22.9711 19.871216 36.844727 "
        "13.396057l0 0c6.6623535 -3.109497 11.816589 -8.73822 14.328857 "
        "-15.647949c2.5122375 -6.9096985 2.1767273 -14.534424 -0.9327698 "
        "-21.196777l0 0c-6.4751587 -13.873596 -22.9711 -19.871246 -36.844727 "
        "-13.396088l0 0c-13.873596 6.4751587 -19.871246 22.97113 -13.396088 "
        "36.844757z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m419.30362 537.09216l0 0c2.197235 4.7077026 7.7948 6.74292 "
        "12.502533 4.5457153l0 0c2.2607422 -1.0551758 4.009735 -2.965149 "
        "4.862213 -5.3098755c0.85250854 -2.3446655 0.73864746 -4.931946 "
        "-0.3164978 -7.192688l0 0c-2.1972046 -4.7077637 -7.7948 -6.74292 "
        "-12.502533 -4.5457153l0 0c-4.7077637 2.1972046 -6.7429504 7.7948 "
        "-4.5457153 12.502563z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m441.86984 514.291l-4.87146 20.83014l-5.336273 -7.958252z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m404.28076 "
        "531.8348l19.094696 9.644531l-2.6726074 -9.201477z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m406.07776 "
        "490.05893l0 0c9.497559 12.011108 26.933777 14.048737 38.944885 "
        "4.551178l0 0c5.7679443 -4.5608826 9.4878235 -11.226288 10.341339 "
        "-18.529877c0.8535156 -7.3035583 -1.2292786 -14.647064 -5.790161 "
        "-20.415009l0 0c-9.497559 -12.011108 -26.933777 -14.048737 -38.944885 "
        "-4.551178l0 0c-12.011108 9.497559 -14.048737 26.933777 -4.551178 "
        "38.944885z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m438.45523 501.48264l0 0c3.2267456 4.079071 9.149292 4.77005 "
        "13.228363 1.5433044l0 0c1.9588318 -1.54953 3.2218933 -3.8137512 "
        "3.511322 -6.2945557c0.2894287 -2.480774 -0.41848755 -4.974945 "
        "-1.9680176 -6.933777l0 0c-3.2267456 -4.0791016 -9.149261 -4.77005 "
        "-13.2283325 -1.543335l0 0c-4.0791016 3.2267456 -4.77005 9.149292 "
        "-1.543335 13.228363z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m455.17444 474.10822l0.05441284 21.388275l-7.023651 -6.514984z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m422.6378 "
        "499.83163l20.79834 4.989044l-4.7181396 -8.337616z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m417.521 "
        "457.04767l0 0c9.088776 12.324219 26.447449 14.947052 38.771667 "
        "5.858246l0 0c5.918274 -4.364563 9.860352 -10.901428 10.958984 "
        "-18.172516c1.0986633 -7.271118 -0.736145 -14.680817 -5.1007385 "
        "-20.599121l0 0c-9.088776 -12.324219 -26.447418 -14.947052 -38.771637 "
        "-5.8582764l0 0c-12.324219 9.088776 -14.947052 26.447449 -5.8582764 "
        "38.771667z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m374.52863 432.03262l0 0c-11.811066 9.741089 -29.282532 8.063019 "
        "-39.02362 -3.7480469l0 0c-4.677826 -5.671875 -6.9109497 -12.969696 "
        "-6.2080383 -20.288055c0.70288086 -7.318329 4.2841797 -14.057709 "
        "9.956055 -18.735565l0 0c11.811066 -9.741089 29.282532 -8.063019 "
        "39.02362 3.7480469l0 0c9.741089 11.811066 8.063049 29.282532 "
        "-3.7480164 39.02362z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m340.47363 436.38525l0 0c-4.009491 3.3049927 -9.939087 2.7338867 "
        "-13.24408 -1.2756042l0 0c-1.5871277 -1.925415 -2.3443909 -4.4024963 "
        "-2.1051636 -6.8862305c0.2392273 -2.4837646 1.4553223 -4.770752 "
        "3.3807373 -6.357849l0 0c4.0095215 -3.3050232 9.939087 -2.7339172 "
        "13.24411 1.2755737l0 0c3.3049927 4.009491 2.7338867 9.939087 "
        "-1.2756042 13.24411z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m329.90982 406.09396l-4.5633545 20.902344l8.241669 -4.889374z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m356.29797 "
        "438.1027l-21.389008 0.49261475l6.371643 -7.1577454z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m423.65277 "
        "432.02725l0 0c11.811066 9.741089 29.282532 8.063049 39.02362 "
        "-3.7480164l0 0c4.6778564 -5.671875 6.9109497 -12.969696 6.2080383 "
        "-20.288055c-0.70288086 -7.3183594 -4.2841797 -14.057739 -9.956055 "
        "-18.735565l0 0c-11.811066 -9.741089 -29.282532 -8.063049 -39.02362 "
        "3.7480164l0 0c-9.741089 11.811066 -8.063049 29.282562 3.7480164 "
        "39.02362z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m457.70776 436.37988l0 0c4.009491 3.3050232 9.939087 2.7339172 "
        "13.24408 -1.2755737l0 0c1.5871277 -1.9254456 2.3443909 -4.4024963 "
        "2.1051636 -6.8862305c-0.2392273 -2.4837646 -1.4553223 -4.770752 "
        "-3.3807373 -6.3578796l0 0c-4.0095215 -3.3050232 -9.939087 -2.7339172 "
        "-13.24411 1.2756042l0 0c-3.3049927 4.009491 -2.7338867 9.939087 "
        "1.2756042 13.24408z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m468.27158 406.08862l4.5633545 20.902313l-8.241669 -4.889374z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m441.88342 "
        "438.09735l21.389008 0.49261475l-6.371643 -7.1577454z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m449.49686 "
        "469.5565l0 0c3.0875854 4.1834717 8.981903 5.0718384 13.165344 "
        "1.9842529l0 0c2.0089722 -1.4826965 3.3466492 -3.7027283 3.71875 "
        "-6.1717224c0.37213135 -2.4689636 -0.25180054 -4.9846497 -1.7344971 "
        "-6.993622l0 0c-3.0875854 -4.183441 -8.981903 -5.0718384 -13.165344 "
        "-1.9842529l0 0c-4.183441 3.0875854 -5.0718384 8.981903 -1.9842529 "
        "13.165344z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m467.1245 442.7525l-0.6633911 21.381165l-6.802063 -6.7479553z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m433.73972 "
        "467.37216l20.622192 5.684906l-4.436371 -8.492462z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m425.35214 "
        "393.6473l0 0c9.088776 7.9928894 22.936218 7.1045227 30.929138 "
        "-1.9842529l0 0c3.8383179 -4.3645935 5.785614 -10.075195 5.413513 "
        "-15.875519c-0.37213135 -5.800354 -3.0332031 -11.2153015 -7.397766 "
        "-15.053619l0 0c-9.088776 -7.99292 -22.936249 -7.104553 -30.929138 "
        "1.9842529l0 0c-7.99292 9.088776 -7.1045227 22.936218 1.9842529 "
        "30.929138z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m452.15482 397.9389l0 0c3.0875854 2.7135925 7.790344 2.4104004 "
        "10.503937 -0.67715454l0 0c1.3031311 -1.482727 1.9638672 -3.4223633 "
        "1.836853 -5.392212c-0.12698364 -1.9698792 -1.031311 -3.8086243 "
        "-2.5140076 -5.111725l0 0c-3.0875854 -2.7135925 -7.790344 -2.410431 "
        "-10.503937 0.67715454l0 0c-2.7135925 3.0875854 -2.4104004 7.790344 "
        "0.67715454 10.503937z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m461.26797 374.2681l3.0836487 16.631683l-6.3905945 -4.0697327z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m439.6114 "
        "398.90244l16.889893 0.9239197l-4.85614 -5.8155212z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m427.16376 "
        "368.57507l0 0c8.175537 4.8270264 18.716248 2.1125488 23.543304 "
        "-6.063019l0 0c2.3180237 -3.9260254 2.9815063 -8.612091 1.8444824 "
        "-13.027344c-1.1370544 -4.415222 -3.9814453 -8.1979065 -7.9074707 "
        "-10.515961l0 0c-8.175568 -4.827057 -18.716278 -2.1125488 -23.543304 "
        "6.0629883l0 0c-4.827057 8.175568 -2.1125793 18.716278 6.0629883 "
        "23.543335z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m448.448 367.9538l0 0c2.774475 1.6351318 6.3491516 0.7114868 "
        "7.984253 -2.0629883l0 0c0.7852173 -1.3323364 1.0090027 -2.9220276 "
        "0.62210083 -4.4193726c-0.38687134 -1.497345 -1.3527527 -2.779663 "
        "-2.685089 -3.5648804l0 0c-2.774475 -1.6351013 -6.3491516 -0.7114563 "
        "-7.984253 2.0629883l0 0c-1.6351013 2.774475 -0.7114868 6.3491516 "
        "2.0629883 7.984253z\" fill-rule=\"evenodd\"/><path fill=\"#46555e\" "
        "d=\"m451.98825 348.37247l4.816864 12.361908l-5.521179 -2.197754z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#46555e\" d=\"m438.91913 "
        "370.53268l13.149017 -1.7669983l-4.5951233 -3.7680664z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#596a74\" d=\"m386.11142 "
        "519.3277l0 0c-7.651886 7.6519165 -7.651886 20.058105 0 "
        "27.710022q7.7101746 7.710144 13.855011 16.985657q6.1448364 -9.275513 "
        "13.855011 -16.985657l0 0c7.651886 -7.6519165 7.651886 -20.058105 0 "
        "-27.710022l0 0c-7.6519165 -7.651886 -20.058105 -7.651886 -27.710022 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#596a74\" d=\"m401.10492 "
        "500.12643l0 0c3.697876 10.170746 -1.5493774 21.413483 -11.720123 "
        "25.111359q-10.248169 3.7260742 -19.739868 9.532715q-1.6454468 "
        "-11.004639 -5.3714905 -21.252808l0 0c-3.6979065 -10.170715 1.5493774 "
        "-21.413452 11.720093 -25.111359l0 0c10.170746 -3.6979065 21.413483 "
        "1.5493774 25.11139 11.720093z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#596a74\" d=\"m389.87042 470.63348l0 0c1.2524414 10.7499695 "
        "-6.446869 20.479858 -17.196838 21.73227q-10.831848 1.2619629 "
        "-21.40747 4.723053q0.9371338 -11.088043 -0.3248291 -21.919891l0 "
        "0c-1.2524109 -10.75 6.446869 -20.479858 17.196869 -21.7323l0 "
        "0c10.7499695 -1.2524109 20.479858 6.446869 21.73227 17.196869z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#596a74\" d=\"m377.76736 "
        "437.71783l0 0c1.6177368 10.697815 -5.7431335 20.681488 -16.440948 "
        "22.299225q-10.779236 1.6300354 -21.22757 5.4484863q0.55841064 "
        "-11.110199 -1.0716248 -21.889435l0 0c-1.6177063 -10.697815 5.7431335 "
        "-20.681488 16.440948 -22.299225l0 0c10.697784 -1.6177063 20.681488 "
        "5.743164 22.299194 16.440948z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#596a74\" d=\"m373.84818 378.2135l0 0c-0.5479431 8.540863 "
        "-7.915863 15.020386 -16.456696 14.472443q-8.605896 -0.5520935 "
        "-17.323853 0.64297485q2.299286 -8.493774 2.85141 -17.09967l0 "
        "0c0.5479431 -8.5408325 7.9158325 -15.020355 16.456696 -14.472443l0 "
        "0c8.5408325 0.5479431 15.020355 7.915863 14.472443 16.456696z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#596a74\" d=\"m374.07755 "
        "356.8482l0 0c-1.6785889 6.4969482 -8.306183 10.403015 -14.803162 "
        "8.724426q-6.546417 -1.6914062 -13.436218 -2.053711q3.0204468 "
        "-6.2030334 4.7118225 -12.749451l0 0c1.6785889 -6.4969482 8.306183 "
        "-10.403015 14.803162 -8.724426l0 0c6.4969482 1.6786194 10.402985 "
        "8.306213 8.724396 14.803162z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#596a74\" d=\"m397.07648 500.1211l0 0c-3.697876 10.170715 "
        "1.5493774 21.413452 11.720123 25.11139q10.248169 3.7260132 19.739868 "
        "9.532715q1.6454468 -11.004639 5.3714905 -21.252869l0 0c3.6979065 "
        "-10.170715 -1.5493774 -21.413452 -11.720093 -25.111328l0 0c-10.170746 "
        "-3.6979065 -21.413483 1.5493469 -25.11139 11.720093z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#596a74\" d=\"m408.31097 "
        "470.6281l0 0c-1.2524414 10.75 6.446869 20.479858 17.196838 "
        "21.7323q10.831848 1.2619629 21.40747 4.7230225q-0.9371338 -11.088043 "
        "0.3248291 -21.91986l0 0c1.2524109 -10.75 -6.446869 -20.479858 "
        "-17.196869 -21.7323l0 0c-10.7499695 -1.2524109 -20.479858 6.446869 "
        "-21.73227 17.196838z\" fill-rule=\"evenodd\"/><path fill=\"#596a74\" "
        "d=\"m420.41403 437.7125l0 0c-1.6177368 10.697784 5.7431335 20.681488 "
        "16.440948 22.299194q10.779236 1.6300354 21.22757 5.448517q-0.55841064 "
        "-11.110199 1.0716248 -21.889465l0 0c1.6177063 -10.697784 -5.7431335 "
        "-20.681488 -16.440948 -22.299194l0 0c-10.697784 -1.6177063 -20.681488 "
        "5.7431335 -22.299194 16.440948z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#596a74\" d=\"m376.43317 412.58087l0 0c-1.0349731 10.7673645 "
        "-10.602692 18.657043 -21.370056 17.62204q-10.849365 -1.0428772 "
        "-21.910461 0.11694336q3.2455444 -10.637665 4.288391 -21.48703l0 "
        "0c1.0350037 -10.7673645 10.602722 -18.657013 21.370087 -17.62204l0 "
        "0c10.767395 1.0350037 18.657043 10.602692 17.62204 21.370087z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#596a74\" d=\"m421.74823 "
        "412.57553l0 0c1.0349731 10.7673645 10.602692 18.657013 21.370056 "
        "17.62204q10.849365 -1.0428772 21.910461 0.11691284q-3.2455444 "
        "-10.637634 -4.288391 -21.487l0 0c-1.0350037 -10.7673645 -10.602722 "
        "-18.657043 -21.370087 -17.62204l0 0c-10.767395 1.0349731 -18.657043 "
        "10.602692 -17.62204 21.370087z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#596a74\" d=\"m424.33322 378.20816l0 0c0.5479431 8.5408325 "
        "7.915863 15.020386 16.456696 14.472443q8.605896 -0.552124 17.323853 "
        "0.64297485q-2.299286 -8.493805 -2.85141 -17.09967l0 0c-0.5479431 "
        "-8.5408325 -7.9158325 -15.020386 -16.456696 -14.472443l0 0c-8.5408325 "
        "0.5479431 -15.020355 7.915863 -14.472443 16.456696z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#596a74\" d=\"m424.10385 "
        "356.84286l0 0c1.6785889 6.4969482 8.306183 10.402985 14.803162 "
        "8.724396q6.546417 -1.6913757 13.436218 -2.0536804q-3.0204468 "
        "-6.2030334 -4.7118225 -12.749481l0 0c-1.6785889 -6.4969482 -8.306183 "
        "-10.402985 -14.803162 -8.724396l0 0c-6.4969482 1.6785889 -10.402985 "
        "8.306183 -8.724396 14.803162z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#6e7a82\" d=\"m446.73877 346.6637l-32.161896 "
        "162.55118l-29.156525 0l-32.161896 -162.55118z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#6e7a82\" d=\"m351.8936 "
        "334.62173l0 0c0 -30.649628 21.539673 -55.496063 48.11023 -55.496063l0 "
        "0c12.759613 0 24.996643 5.846863 34.019073 16.254395c9.02243 "
        "10.407532 14.091156 24.523193 14.091156 39.24167l0 0c0 30.649628 "
        "-21.539673 55.496063 -48.11023 55.496063l0 0c-26.570557 0 -48.11023 "
        "-24.846436 -48.11023 -55.496063z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#6e7a82\" d=\"m366.69257 334.6087l0 0c0 -21.212952 14.905029 "
        "-38.409454 33.29135 -38.409454l0 0c8.829407 0 17.29718 4.046692 "
        "23.540527 11.249847c6.2433167 7.203186 9.750793 16.972778 9.750793 "
        "27.159607l0 0c0 21.212952 -14.905029 38.409424 -33.29132 38.409424l0 "
        "0c-18.386322 0 -33.29135 -17.196472 -33.29135 -38.409424z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#6e7a82\" d=\"m385.30276 "
        "506.73865l0 0c0 -9.358398 6.578247 -16.944885 14.692932 -16.944885l0 "
        "0c3.8967896 0 7.6340027 1.7852478 10.389435 4.963043c2.7554626 "
        "3.1777954 4.303467 7.487793 4.303467 11.981842l0 0c0 9.358398 "
        "-6.578247 16.944885 -14.692902 16.944885l0 0c-8.114685 0 -14.692932 "
        "-7.586487 -14.692932 -16.944885z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#6e7a82\" d=\"m432.0778 344.86115l-32.078735 "
        "164.09448l-32.078735 -164.09448z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_dakotaraptor[3]);
    rr_renderer_set_dimensions(&this->mob_dakotaraptor[3], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_dakotaraptor[3],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#85929c\" d=\"m346.134 "
        "276.56253l0 0c0 -53.518356 24.091827 -96.903564 53.810608 "
        "-96.903564l0 0c14.271454 0 27.958374 10.209457 38.049835 "
        "28.3824c10.091431 18.172943 15.760742 42.82074 15.760742 68.521164l0 "
        "0c0 53.51834 -24.091827 96.903534 -53.810577 96.903534l0 0c-29.71878 "
        "0 -53.810608 -43.385193 -53.810608 -96.903534z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#6e7a82\" d=\"m338.8817 "
        "276.57175l0 0c0 -60.73636 27.341064 -109.972916 61.068024 "
        "-109.972916l0 0c16.196228 0 31.729126 11.586395 43.18161 "
        "32.210327c11.452454 20.623917 17.886383 48.595963 17.886383 "
        "77.76259l0 0c0 60.73636 -27.341064 109.9729 -61.067993 109.9729l0 "
        "0c-33.72696 0 -61.068024 -49.23654 -61.068024 -109.9729zm13.861206 "
        "0l0 0c0 53.081024 21.135193 96.111694 47.206818 96.111694c26.071594 0 "
        "47.206787 -43.03067 47.206787 -96.111694l0 0c0 -53.081024 -21.135193 "
        "-96.111694 -47.206787 -96.111694l0 0c-26.071625 0 -47.206818 43.03067 "
        "-47.206818 96.111694z\" fill-rule=\"evenodd\"/><path fill=\"#6e7a82\" "
        "d=\"m358.52267 159.78925l0 0c0 -22.89064 18.556519 -41.447166 "
        "41.447174 -41.447166l0 0l0 0c10.992462 0 21.534698 4.366745 29.307556 "
        "12.139595c7.7728577 7.7728577 12.139618 18.31511 12.139618 "
        "29.307571l0 43.84256c0 22.89064 -18.55655 41.447174 -41.447174 "
        "41.447174l0 0l0 0c-22.890656 0 -41.447174 -18.556534 -41.447174 "
        "-41.447174z\" fill-rule=\"evenodd\"/><path fill=\"#85929c\" "
        "d=\"m370.18155 162.42134l0 0c0 -16.451202 13.336334 -29.787521 "
        "29.787506 -29.787521l0 0l0 0c7.9001465 0 15.476746 3.138321 21.062988 "
        "8.724564c5.5862427 5.5862427 8.724548 13.162811 8.724548 21.062958l0 "
        "67.16185c0 16.451187 -13.336334 29.787537 -29.787537 29.787537l0 0l0 "
        "0c-16.451172 0 -29.787506 -13.3363495 -29.787506 -29.787537z\" "
        "fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_dakotaraptor[4]);
    rr_renderer_set_dimensions(&this->mob_dakotaraptor[4], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_dakotaraptor[4],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#85929c\" d=\"m366.69266 "
        "334.6138l0 0c0 -21.215668 14.90744 -38.4144 33.29669 -38.4144l0 "
        "0c8.830841 0 17.299988 4.047241 23.544342 11.251312c6.2443237 "
        "7.2041016 9.75238 16.974945 9.75238 27.163086l0 0c0 21.215668 "
        "-14.90744 38.414368 -33.296722 38.414368l0 0c-18.389252 0 -33.29669 "
        "-17.1987 -33.29669 -38.414368z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#85929c\" d=\"m432.07788 344.87l-32.084503 "
        "164.08582l-32.084473 -164.08582z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_dakotaraptor[5]);
    rr_renderer_set_dimensions(&this->mob_dakotaraptor[5], 800, 600);
    rr_renderer_draw_svg(
        &this->mob_dakotaraptor[5],
        "<svg version=\"1.1\" viewBox=\"0.0 0.0 800.0 600.0\" fill=\"none\" "
        "stroke=\"none\" stroke-linecap=\"square\" stroke-miterlimit=\"10\" "
        "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
        "xmlns=\"http://www.w3.org/2000/svg\"><clipPath id=\"p.0\"><path "
        "d=\"m0 0l800.0 0l0 600.0l-800.0 0l0 -600.0z\" "
        "clip-rule=\"nonzero\"/></clipPath><g clip-path=\"url(#p.0)\"><path "
        "fill=\"#000000\" fill-opacity=\"0.0\" d=\"m0 0l800.0 0l0 600.0l-800.0 "
        "0z\" fill-rule=\"evenodd\"/><path fill=\"#6e6356\" d=\"m362.36746 "
        "94.56693l15.724487 -59.023624l43.795105 0l15.724487 59.023624z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#6e6356\" d=\"m377.77954 "
        "39.301838l0 0c0 -12.272028 9.948425 -22.220472 22.220459 -22.220472l0 "
        "0c5.8932495 0 11.545105 2.3410778 15.71225 6.5082245c4.167145 "
        "4.1671467 6.508209 9.81901 6.508209 15.712248l0 0c0 12.272026 "
        "-9.948425 22.22047 -22.220459 22.22047l0 0c-12.272034 0 -22.220459 "
        "-9.948444 -22.220459 -22.22047z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#85929c\" d=\"m358.1146 127.42417l0 0c0 -25.283333 18.72647 "
        "-45.779526 41.82675 -45.779526l0 0c11.09317 0 21.731964 4.823189 "
        "29.57602 13.408516c7.8440247 8.585327 12.250763 20.22953 12.250763 "
        "32.37101l0 0c0 25.283333 -18.72647 45.779533 -41.826782 45.779533l0 "
        "0c-23.10028 0 -41.82675 -20.4962 -41.82675 -45.779533z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#6e7a82\" d=\"m352.4766 "
        "127.427025l0 0c0 -28.692719 21.25766 -51.95276 47.480316 -51.95276l0 "
        "0c12.59256 0 24.669373 5.4735794 33.57367 15.216606c8.904266 9.743034 "
        "13.906647 22.95742 13.906647 36.736153l0 0c0 28.69271 -21.25766 "
        "51.95275 -47.480316 51.95275l0 0c-26.222656 0 -47.480316 -23.26004 "
        "-47.480316 -51.95275zm13.175781 0c0 21.415932 15.358673 38.77697 "
        "34.304535 38.77697c18.945862 0 34.304535 -17.361038 34.304535 "
        "-38.77697c0 -21.415932 -15.358673 -38.77697 -34.304535 -38.77697l0 "
        "0c-18.945862 0 -34.304535 17.361038 -34.304535 38.77697z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#85929c\" d=\"m369.70798 "
        "109.006996l0 0c4.4009094 3.1310577 9.420959 0.17684174 11.212616 "
        "-6.598427c1.7916565 -6.775276 -0.32351685 -14.805946 -4.7244263 "
        "-17.937012l0 0c-0.03262329 9.332344 -2.3563232 18.11953 -6.4881897 "
        "24.535439z\" fill-rule=\"evenodd\"/><path fill=\"#85929c\" "
        "d=\"m430.23688 109.006996l0 0c-4.400879 3.1310654 -9.420929 "
        "0.17684937 -11.212585 -6.598427c-1.7916565 -6.7752686 0.32351685 "
        "-14.805939 4.7243958 -17.937004l0 0c0.03265381 9.332336 2.3563538 "
        "18.11953 6.4881897 24.53543z\" fill-rule=\"evenodd\"/><path "
        "fill=\"#6e6356\" d=\"m362.63034 93.55555c-0.81585693 2.6202927 "
        "-2.469818 4.940941 -1.2887268 5.9291306c1.1811218 0.9881897 6.1098022 "
        "0.8591461 8.375336 0c2.2655334 -0.8591385 3.8433838 -2.6202927 "
        "5.2178345 -5.154854c1.3744507 -2.5345612 2.5026245 -6.744972 "
        "3.0288696 -10.05249c0.5262451 -3.3075256 2.08313 -9.706917 0.12863159 "
        "-9.792656c-1.9545288 -0.085739136 -9.278656 6.099739 -11.855652 "
        "9.2782135c-2.5769958 3.178482 -2.7904663 7.1723557 -3.6062927 "
        "9.792656z\" fill-rule=\"evenodd\"/><path fill=\"#6e6356\" "
        "d=\"m437.27478 93.55643c0.8158264 2.6203003 2.4697876 4.9409485 "
        "1.2886963 5.929138c-1.1810913 0.9881897 -6.1098022 0.8591385 "
        "-8.375336 0c-2.2655334 -0.8591461 -3.8433838 -2.6203003 -5.2178345 "
        "-5.154854c-1.3744507 -2.5345612 -2.5026245 -6.744972 -3.0288696 "
        "-10.052498c-0.5262451 -3.3075256 -2.08313 -9.706909 -0.12860107 "
        "-9.792648c1.9544983 -0.085739136 9.2786255 6.099739 11.855621 "
        "9.2782135c2.5769958 3.178482 2.7904663 7.1723557 3.6063232 "
        "9.792648z\" fill-rule=\"evenodd\"/><path fill=\"#827566\" "
        "d=\"m375.09192 94.56693l14.85556 -59.023624l19.989655 0l14.85556 "
        "59.023624z\" fill-rule=\"evenodd\"/><path fill=\"#85929c\" "
        "d=\"m375.1497 96.98425l0 0c0 -13.63752 11.125885 -24.692917 24.850403 "
        "-24.692917l0 0c6.5907288 0 12.91153 2.6015701 17.571869 "
        "7.2323914c4.66037 4.630821 7.2785034 10.91156 7.2785034 17.460526l0 "
        "0c0 13.63752 -11.125885 24.69291 -24.850372 24.69291l0 0c-13.724518 0 "
        "-24.850403 -11.055389 -24.850403 -24.69291z\" "
        "fill-rule=\"evenodd\"/><path fill=\"#827566\" d=\"m389.3071 "
        "39.301838l0 0c0 -5.905533 4.7873535 -10.692915 10.692902 -10.692915l0 "
        "0c2.8359375 0 5.555725 1.1265717 7.561035 3.1318836c2.00531 2.00531 "
        "3.1318665 4.725094 3.1318665 7.5610313l0 0c0 5.905533 -4.7873535 "
        "10.692913 -10.692902 10.692913l0 0c-5.905548 0 -10.692902 -4.78738 "
        "-10.692902 -10.692913z\" fill-rule=\"evenodd\"/></g></svg>",
        0, 0);

    rr_renderer_init(&this->mob_stump);
    rr_renderer_set_dimensions(&this->mob_stump, 800, 600);
    rr_renderer_draw_svg(&this->mob_stump,
#include <Client/Assets/Mob/Stump.h>
                         , 0, 0);

    rr_renderer_init(&this->tiles[0]);
    rr_renderer_set_dimensions(&this->tiles[0], 256, 256);
    rr_renderer_draw_svg(&this->tiles[0],
#include <Client/Assets/Tile/1.h>
                         , 0, 0);

    rr_renderer_init(&this->tiles[1]);
    rr_renderer_set_dimensions(&this->tiles[1], 256, 256);
    rr_renderer_draw_svg(&this->tiles[1],
#include <Client/Assets/Tile/2.h>
                         , 0, 0);

    rr_renderer_init(&this->tiles[2]);
    rr_renderer_set_dimensions(&this->tiles[2], 256, 256);
    rr_renderer_draw_svg(&this->tiles[2],
#include <Client/Assets/Tile/3.h>
                         , 0, 0);

    this->tiles_size = 3;

    rr_renderer_init(&this->background_features[0]);
    rr_renderer_set_dimensions(&this->background_features[0], 100, 100);
    rr_renderer_draw_svg(&this->background_features[0],
#include <Client/Assets/MapFeature/Moss.h>
                         , 0, 0);

    rr_renderer_init(&this->background_features[1]);
    rr_renderer_set_dimensions(&this->background_features[1], 200, 200);
    rr_renderer_draw_svg(&this->background_features[1],
#include <Client/Assets/MapFeature/WaterLettuce.h>
                         , 0, 0);

    rr_renderer_init(&this->background_features[2]);
    rr_renderer_set_dimensions(&this->background_features[2], 200, 200);
    rr_renderer_draw_svg(&this->background_features[2],
#include <Client/Assets/MapFeature/Fern1.h>
                         , 0, 0);

    rr_renderer_init(&this->background_features[3]);
    rr_renderer_set_dimensions(&this->background_features[3], 200, 200);
    rr_renderer_draw_svg(&this->background_features[3],
#include <Client/Assets/MapFeature/Fern2.h>
                         , 0, 0);

    rr_renderer_init(&this->background_features[4]);
    rr_renderer_set_dimensions(&this->background_features[4], 200, 200);
    rr_renderer_draw_svg(&this->background_features[4],
#include <Client/Assets/MapFeature/Fern3.h>
                         , 0, 0);

    rr_renderer_init(&this->background_features[5]);
    rr_renderer_set_dimensions(&this->background_features[5], 200, 200);
    rr_renderer_draw_svg(&this->background_features[5],
#include <Client/Assets/MapFeature/Fern4.h>
                         , 0, 0);

    rr_renderer_init(&this->background_features[6]);
    rr_renderer_set_dimensions(&this->background_features[6], 200, 200);
    rr_renderer_draw_svg(&this->background_features[6],
#include <Client/Assets/MapFeature/Fern5.h>
                         , 0, 0);

    rr_renderer_init(&this->background_features[7]);
    rr_renderer_set_dimensions(&this->background_features[7], 200, 200);
    rr_renderer_draw_svg(&this->background_features[7],
#include <Client/Assets/MapFeature/PalmTree.h>
                         , 0, 0);

    rr_renderer_init(&this->background_features[8]);
    rr_renderer_set_dimensions(&this->background_features[8], 450, 450);
    rr_renderer_draw_svg(&this->background_features[8],
#include <Client/Assets/MapFeature/BeechTree.h>
                         , 0, 0);
}

void rr_game_websocket_on_event_function(enum rr_websocket_event_type type,
                                         void *data, void *captures,
                                         uint64_t size)
{
    struct rr_game *this = captures;
    switch (type)
    {
    case rr_websocket_event_type_open:
        this->socket_pending = 0;
        puts("websocket opened");
        break;
    case rr_websocket_event_type_close:
        // memset the clients
        puts("websocket closed");
        break;
    case rr_websocket_event_type_data:
    {
        struct proto_bug encoder;
        proto_bug_init(&encoder, data);

        if (!this->socket.recieved_first_packet)
        {
            this->socket.recieved_first_packet = 1;
            rr_decrypt(data, 1024, 21094093777837637ull);
            rr_decrypt(data, 8, 1);
            rr_decrypt(data, 1024, 59731158950470853ull);
            rr_decrypt(data, 1024, 64709235936361169ull);
            rr_decrypt(data, 1024, 59013169977270713ull);
            uint64_t verification =
                proto_bug_read_uint64(&encoder, "verification");
            proto_bug_read_uint32(&encoder, "useless bytes");
            this->socket.clientbound_encryption_key =
                proto_bug_read_uint64(&encoder, "c encryption key");
            this->socket.serverbound_encryption_key =
                proto_bug_read_uint64(&encoder, "s encryption key");
            struct proto_bug verify_encoder;
            proto_bug_init(&verify_encoder, &output_packet[0]);
            proto_bug_write_uint64(&verify_encoder, rr_get_rand(),
                                   "useless bytes");
            proto_bug_write_uint64(&verify_encoder, verification,
                                   "verification");
#ifdef RIVET_BUILD
            uint64_t token_size = strlen(this->socket.rivet_player_token
                                             ? this->socket.rivet_player_token
                                             : strdup(""));
            proto_bug_write_varuint(&verify_encoder, token_size,
                                    "rivet token size");
            proto_bug_write_string(&verify_encoder,
                                   this->socket.rivet_player_token, token_size,
                                   "rivet token");
#endif
            rr_websocket_send(&this->socket, verify_encoder.start,
                              verify_encoder.current);
            this->socket_ready = 1;
            return;
        }

        this->socket.clientbound_encryption_key =
            rr_get_hash(this->socket.clientbound_encryption_key);
        rr_decrypt(data, size, this->socket.clientbound_encryption_key);
        switch (proto_bug_read_uint8(&encoder, "header"))
        {
        case 0:
        {
            this->simulation_ready = 1;
            rr_simulation_read_binary(this, &encoder);
            struct proto_bug encoder2;
            proto_bug_init(&encoder2, output_packet);
            proto_bug_write_uint8(&encoder2, 0, "header");
            proto_bug_write_uint8(&encoder2, 0, "movement type");
            uint8_t movement_flags = 0;
            movement_flags |=
                (rr_bitset_get(this->input_data->keys_pressed, 87) ||
                 rr_bitset_get(this->input_data->keys_pressed, 38))
                << 0;
            movement_flags |=
                (rr_bitset_get(this->input_data->keys_pressed, 65) ||
                 rr_bitset_get(this->input_data->keys_pressed, 37))
                << 1;
            movement_flags |=
                (rr_bitset_get(this->input_data->keys_pressed, 83) ||
                 rr_bitset_get(this->input_data->keys_pressed, 40))
                << 2;
            movement_flags |=
                (rr_bitset_get(this->input_data->keys_pressed, 68) ||
                 rr_bitset_get(this->input_data->keys_pressed, 39))
                << 3;
            movement_flags |= this->input_data->mouse_buttons << 4;
            movement_flags |= rr_bitset_get(this->input_data->keys_pressed, 32)
                              << 4;
            movement_flags |= rr_bitset_get(this->input_data->keys_pressed, 16)
                              << 5;
            proto_bug_write_uint8(&encoder2, movement_flags,
                                  "movement kb flags");
            rr_websocket_send(&this->socket, encoder2.start, encoder2.current);
            break;
        }
        case 69:
        {
            this->ticks_until_game_start =
                proto_bug_read_uint8(&encoder, "countdown");
            for (uint32_t i = 0; i < 4; ++i)
            {
                this->squad_members[i].in_use =
                    proto_bug_read_uint8(&encoder, "bitbit");
                this->squad_members[i].ready =
                    proto_bug_read_uint8(&encoder, "ready");
                for (uint32_t j = 0; j < 20; ++j)
                {
                    this->squad_members[i].loadout[j].id =
                        proto_bug_read_uint8(&encoder, "id");
                    this->squad_members[i].loadout[j].rarity =
                        proto_bug_read_uint8(&encoder, "rar");
                }
            }
            struct proto_bug encoder2;
            proto_bug_init(&encoder2, output_packet);
            proto_bug_write_uint8(&encoder2, 70, "header");
            for (uint32_t i = 0; i < 20; ++i)
            {
                if (this->protocol_state & (1 << i))
                {
                    proto_bug_write_uint8(&encoder2, i + 1, "pos");
                    proto_bug_write_uint8(&encoder2, this->loadout[i].id, "id");
                    proto_bug_write_uint8(&encoder2, this->loadout[i].rarity,
                                          "rar");
                }
            }
            proto_bug_write_uint8(&encoder2, 0, "pos");
            rr_websocket_send(&this->socket, encoder2.start, encoder2.current);
            this->protocol_state = 0;
            break;
        }
        default:
            RR_UNREACHABLE("how'd this happen");
        }
        break;
    }
    default:
        RR_UNREACHABLE("non exhaustive switch expression");
    }
}

void render_drop_component(EntityIdx entity, void *_captures)
{
    struct rr_game *this = _captures;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this->simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_drop_render(entity, this);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_health_component(EntityIdx entity, void *_captures)
{
    struct rr_game *this = _captures;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this->simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x,
                          physical->lerp_y + physical->radius + 30);
    rr_component_health_render(entity, this);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_mob_component(EntityIdx entity, void *_captures)
{
    struct rr_game *this = _captures;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this->simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_mob_render(entity, this);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_petal_component(EntityIdx entity, void *_captures)
{
    struct rr_game *this = _captures;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this->simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_petal_render(entity, this);
    rr_renderer_context_state_free(this->renderer, &state);
}

void render_flower_component(EntityIdx entity, void *_captures)
{
    struct rr_game *this = _captures;
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(this->renderer, &state);
    struct rr_component_physical *physical =
        rr_simulation_get_physical(this->simulation, entity);
    rr_renderer_translate(this->renderer, physical->lerp_x, physical->lerp_y);
    rr_component_flower_render(entity, this);
    rr_renderer_context_state_free(this->renderer, &state);
}

void player_info_finder(struct rr_game *this)
{
    struct rr_simulation *simulation = this->simulation;
    uint8_t counter = 1;
    memset(&this->player_infos, 0, sizeof this->player_infos);
    this->player_infos[0] = this->player_info->parent_id;
    for (EntityIdx i = 1; i < RR_MAX_ENTITY_COUNT; ++i)
        if (rr_bitset_get(simulation->player_info_tracker, i) &&
            i != this->player_info->parent_id)
            this->player_infos[counter++] = i;
}

static void render_background(struct rr_component_player_info *player_info,
                              struct rr_game *this, uint32_t prop_amount)
{
    double scale = player_info->lerp_camera_fov * this->renderer->scale;
    double leftX =
        player_info->lerp_camera_x - this->renderer->width / (2 * scale);
    double rightX =
        player_info->lerp_camera_x + this->renderer->width / (2 * scale);
    double topY =
        player_info->lerp_camera_y - this->renderer->height / (2 * scale);
    double bottomY =
        player_info->lerp_camera_y + this->renderer->height / (2 * scale);

#define GRID_SIZE (256)
    double newLeftX = floorf(leftX / GRID_SIZE) * GRID_SIZE;
    double newTopY = floorf(topY / GRID_SIZE) * GRID_SIZE;
    for (; newLeftX < rightX; newLeftX += GRID_SIZE)
    {
        for (double currY = newTopY; currY < bottomY; currY += GRID_SIZE)
        {
            uint32_t tile_index = (uint32_t)((newLeftX / GRID_SIZE + 1) *
                                             (currY / GRID_SIZE + 2)) %
                                  this->tiles_size;
            struct rr_renderer_context_state state;
            rr_renderer_context_state_init(this->renderer, &state);
            rr_renderer_translate(this->renderer, newLeftX + GRID_SIZE / 2,
                                  currY + GRID_SIZE / 2);
            rr_renderer_draw_image(this->renderer, this->tiles + tile_index);
            rr_renderer_context_state_free(this->renderer, &state);
        }
    }

#define render_map_feature                                                     \
    struct rr_renderer_context_state state;                                    \
    float theta = ((double)(uint32_t)(rr_get_hash(i + 200000))) /              \
                  ((double)UINT32_MAX) * (M_PI * 2);                           \
    float distance = sqrtf(((double)(uint32_t)(rr_get_hash(i + 300000))) /     \
                           ((double)UINT32_MAX)) *                             \
                     1600.0;                                                   \
    float rotation = ((double)(uint32_t)(rr_get_hash(i + 400000))) /           \
                     ((double)UINT32_MAX) * (M_PI * 2);                        \
    float x = distance * sinf(theta);                                          \
    float y = distance * cosf(theta);                                          \
    if (x < leftX - (selected_feature == 8 ? 400 : 100))                       \
        continue;                                                              \
    if (x > rightX + (selected_feature == 8 ? 400 : 100))                      \
        continue;                                                              \
    if (y < topY - (selected_feature == 8 ? 400 : 100))                        \
        continue;                                                              \
    if (y > bottomY + (selected_feature == 8 ? 400 : 100))                     \
        continue;                                                              \
    rr_renderer_context_state_init(this->renderer, &state);                    \
    rr_renderer_translate(this->renderer, x, y);                               \
    rr_renderer_rotate(this->renderer, rotation);                              \
    rr_renderer_draw_image(this->renderer,                                     \
                           &this->background_features[selected_feature]);      \
    rr_renderer_context_state_free(this->renderer, &state);

    rr_renderer_set_global_alpha(this->renderer, 0.75f);

    // draw background features
    for (uint64_t i = 0; i < prop_amount;)
    {
        uint64_t selected_feature;
        // any number between 0-8 and is not 1. 1 is water lettuce
        // which is disabled for now
        do
        {
            selected_feature = rr_get_hash(i) % 8;
            i++;
        } while (selected_feature == 1);

        render_map_feature
    }
    // trees over everything
    for (uint64_t i = 0; i < prop_amount / 50; i++)
    {
        uint64_t selected_feature = 8;
        render_map_feature
    }

#undef GRID_SIZE
#undef render_map_feature
}

void rr_game_tick(struct rr_game *this, float delta)
{
    struct timeval start;
    struct timeval end;

    gettimeofday(&start, NULL);
    double time = start.tv_sec * 1000000 + start.tv_usec;
    rr_renderer_set_transform(this->renderer, 1, 0, 0, 0, 1, 0);
    struct rr_renderer_context_state grand_state;
    rr_renderer_context_state_init(this->renderer, &grand_state);
    // render off-game elements

    if (this->simulation_ready)
    {
        rr_simulation_tick(this->simulation, delta);

        this->renderer->state.filter.amount = 0;
        struct rr_renderer_context_state state1;
        struct rr_renderer_context_state state2;
        // rr_simulation_for_each_entity(this->simulation, this->simulation,
        // player_info_finder);
        if (this->player_info != 0)
        {
            // screen shake
            rr_renderer_context_state_init(this->renderer, &state1);
            struct rr_component_player_info *player_info = this->player_info;
            rr_renderer_translate(this->renderer, this->renderer->width / 2,
                                  this->renderer->height / 2);
            rr_renderer_scale(this->renderer, player_info->lerp_camera_fov *
                                                  this->renderer->scale);
            rr_renderer_translate(this->renderer, -player_info->lerp_camera_x,
                                  -player_info->lerp_camera_y);
#ifdef RIVET_BUILD
            if (player_info->flower_id != RR_NULL_ENTITY)
            {
                if (rr_simulation_get_physical(this->simulation,
                                               player_info->flower_id)
                        ->server_animation_tick != 0)
                    rr_renderer_translate(this->renderer, rr_frand() * 5.0f,
                                          rr_frand() * 5.0f);
            }
#endif

            uint32_t alpha = (uint32_t)(player_info->lerp_camera_fov * 51)
                             << 24;
            rr_renderer_context_state_init(this->renderer, &state2);
            rr_renderer_set_transform(this->renderer, 1, 0, 0, 0, 1, 0);
            rr_renderer_set_fill(this->renderer, 0xff45230a);
            rr_renderer_fill_rect(this->renderer, 0, 0, this->renderer->width,
                                  this->renderer->height);
            rr_renderer_set_fill(this->renderer, alpha);
            rr_renderer_fill_rect(this->renderer, 0, 0, this->renderer->width,
                                  this->renderer->height);
            rr_renderer_context_state_free(this->renderer, &state2);
            rr_renderer_context_state_init(this->renderer, &state2);

            struct rr_component_arena *arena =
                rr_simulation_get_arena(this->simulation, 1);
            rr_renderer_begin_path(this->renderer);
            rr_renderer_arc(this->renderer, 0, 0, arena->radius);
            rr_renderer_set_fill(this->renderer, 0xff45230a);
            rr_renderer_fill(this->renderer);
            rr_renderer_clip(this->renderer);

            rr_renderer_set_line_width(this->renderer, 1.0f);
            rr_renderer_set_stroke(this->renderer, alpha);
            rr_renderer_set_global_alpha(this->renderer, 1);

            render_background(player_info, this, 200);

            rr_renderer_context_state_free(this->renderer, &state2);

            rr_simulation_for_each_health(this->simulation, this,
                                          render_health_component);
            rr_simulation_for_each_drop(this->simulation, this,
                                        render_drop_component);
            rr_simulation_for_each_mob(this->simulation, this,
                                       render_mob_component);
            rr_simulation_for_each_petal(this->simulation, this,
                                         render_petal_component);
            rr_simulation_for_each_flower(this->simulation, this,
                                          render_flower_component);
            rr_renderer_context_state_free(this->renderer, &state1);
        }
    }
    else
    {
        // render background but different
        struct rr_component_player_info custom_player_info;
        rr_component_player_info_init(&custom_player_info, 0);
        custom_player_info.lerp_camera_fov = 0.9;
        struct rr_renderer_context_state state;
        rr_renderer_context_state_init(this->renderer, &state);
        rr_renderer_translate(this->renderer, this->renderer->width * 0.5f,
                              this->renderer->height * 0.5f);
        render_background(&custom_player_info, this, 400);
        rr_renderer_context_state_free(this->renderer, &state);
    }
    // ui
    this->simulation_not_ready = this->simulation_ready == 0;
    this->prev_focused = this->focused;
    rr_ui_container_refactor(this->window);
    rr_ui_render_element(this->window, this);
    this->window->poll_events(this->window, this);

    if (this->focused != NULL)
        this->focused->on_event(this->focused, this);
    else
        this->window->on_event(this->window, this);
    if (this->prev_focused != this->focused && this->prev_focused != NULL)
        this->prev_focused->on_event(this->prev_focused, this);
#ifndef EMSCRIPTEN
    lws_service(this->socket.socket_context, -1);
#endif
    if (this->socket_ready)
    {
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick,
                              186 /* ; */))
            this->displaying_debug_information ^= 1;
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick,
                              75 /* k */))
        {
            struct proto_bug encoder;
            proto_bug_init(&encoder, output_packet);
            proto_bug_write_uint8(&encoder, 3, "header");
            proto_bug_write_uint8(&encoder, 2, "cheat type");
            rr_websocket_send(&this->socket, encoder.start, encoder.current);
        }
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick,
                              76 /* l */))
        {
            struct proto_bug encoder;
            proto_bug_init(&encoder, output_packet);
            proto_bug_write_uint8(&encoder, 3, "header");
            proto_bug_write_uint8(&encoder, 1, "cheat type");
            rr_websocket_send(&this->socket, encoder.start, encoder.current);
        }
        if (rr_bitset_get_bit(this->input_data->keys_pressed_this_tick,
                              86 /* v */))
        {
            struct proto_bug encoder;
            proto_bug_init(&encoder, output_packet);
            proto_bug_write_uint8(&encoder, 3, "header");
            proto_bug_write_uint8(&encoder, 3, "cheat type");
            rr_websocket_send(&this->socket, encoder.start, encoder.current);
        }
        if (this->simulation_ready &&
            rr_bitset_get_bit(this->input_data->keys_pressed_this_tick,
                              13 /* enter */))
        {
            struct proto_bug encoder;
            proto_bug_init(&encoder, output_packet);
            proto_bug_write_uint8(&encoder, 1, "header");
            rr_websocket_send(&this->socket, encoder.start, encoder.current);
        }
    }

    gettimeofday(&end, NULL);
    long time_elapsed =
        (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);

    if (this->displaying_debug_information)
    {
        struct rr_renderer_context_state state;
        rr_renderer_context_state_init(this->renderer, &state);
        rr_renderer_scale(this->renderer, 5);
        static char debug_mspt[100];
        debug_mspt[sprintf(debug_mspt, "%f mspt",
                           (float)time_elapsed / 1000.0f)] = 0;
        rr_renderer_fill_text(this->renderer, debug_mspt, 0, 8);
        rr_renderer_context_state_free(this->renderer, &state);
        // rr_renderer_stroke_text
    }
    rr_renderer_context_state_free(this->renderer, &grand_state);

    memset(this->input_data->keys_pressed_this_tick, 0, RR_BITSET_ROUND(256));
    memset(this->input_data->keys_released_this_tick, 0, RR_BITSET_ROUND(256));
    this->input_data->mouse_buttons_this_tick = 0;
    this->input_data->mouse_state_this_tick = 0;
}

void rr_game_connect_socket(struct rr_game *this)
{
    memset(this->simulation, 0, sizeof *this->simulation);
    this->socket_ready = 0;
    this->simulation_ready = 0;
    this->socket_pending = 1;
    rr_websocket_init(&this->socket);
    this->socket.user_data = this;
    this->socket.on_event = rr_game_websocket_on_event_function;
#ifdef RIVET_BUILD
    rr_rivet_lobbies_find(&this->socket);
#else
#ifdef RR_WINDOWS
    rr_websocket_connect_to(&this->socket, "127.0.0.1", 1234, 0);
#else
    rr_websocket_connect_to(&this->socket, "127.0.0.1", 1234, 0);
    // rr_websocket_connect_to(&this->socket, "45.79.197.197", 1234, 0);
#endif
#endif
}

void rr_rivet_lobby_on_find(char *s, char *token, uint16_t port, void *captures)
{
    struct rr_websocket *socket = captures;
    if (port == 443)
        rr_websocket_connect_to(socket, s, port, 1);
    else
        rr_websocket_connect_to(socket, s, port, 0);
    free(s);
// socket->rivet_player_token = strdup(token);
// free(token);
#ifdef RIVET_BUILD
    socket->rivet_player_token = token;
#endif
}
