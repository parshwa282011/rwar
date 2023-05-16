#include <Client/Game.h>
#include <Client/InputData.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>
#include <Client/Socket.h>
#include <Shared/Bitset.h>

#include <stdio.h>
#include <stdint.h>

#ifndef EMSCRIPTEN
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef EMSCRIPTEN
#include <emscripten.h>
void rr_key_event(struct rr_game *this, uint8_t type, uint32_t key)
{
    /*
    if (type == 0)
        rr_bitset_set(&this->input_data->keys[0], key);
    else
        rr_bitset_unset(&this->input_data->keys[0], key);
        */
}
#else
#endif

void rr_main_renderer_initialize(struct rr_game *this)
{
#ifdef EMSCRIPTEN
    EM_ASM({
document.oncontextmenu = function() { return false; };
        Module.canvas = document.createElement("canvas");
        Module.canvas.id = "canvas";
        document.body.appendChild(Module.canvas);
        Module.ctxs = [Module.canvas.getContext('2d')];
        Module.availableCtxs = new Array(100).fill(0).map(function(_, i) { return i; });
        window.addEventListener(
            "keydown", function({which}) { Module._rr_key_event(1, which); });
        window.addEventListener(
            "keyup", function({which}) { Module._rr_key_event(0, which); });
        // window.addEventListener("mousedown", function({clientX, clientY, button}){Module.___Renderer_MouseEvent(clientX*devicePixelRatio, clientY*devicePixelRatio, 1, button)});
        // window.addEventListener("mousemove", function({clientX, clientY, button}){Module.___Renderer_MouseEvent(clientX*devicePixelRatio, clientY*devicePixelRatio, 2, button)});
        // window.addEventListener("mouseup", function({clientX, clientY, button}){Module.___Renderer_MouseEvent(clientX*devicePixelRatio, clientY*devicePixelRatio, 0, button)});
        Module.paths = [... Array(100)].fill(null);
        Module.availablePaths = new Array(100).fill(0).map(function(_, i) { return i; });
        Module.addPath = function()
        {
            if (Module.availablePaths.length)
            {
                const index = Module.availablePaths.pop();
                Module.paths[index] = new Path2D();
                return index;
            }
            throw new Error('Out of Paths: Can be fixed by allowing more paths');
            return -1;
        };
        Module.removePath = function(index)
        {
            Module.paths[index] = null;
            Module.availablePaths.push(index);
        };
        Module.addCtx = function()
        {
            if (Module.availableCtxs.length)
            {
                const index = Module.availableCtxs.shift();
                if (index == 0)
                    return 0; // used for the main ctx, because that has special behavior
                const ocanvas = new OffscreenCanvas(0, 0);
                Module.ctxs[index] = ocanvas.getContext('2d');
                return index;
            }
            throw new Error('Out of Contexts: Can be fixed by allowing more contexts');
            return -1;
        };
        Module.removeCtx = function(index)
        {
            if (index == 0)
                throw new Error('Tried to delete the main context');
            Module.ctxs[index] = null;
            Module.availableCtxs.push(index);
        };
        Module.ReadCstr = function(ptr)
        {
            let str = "";
            let char;
            while ((char = Module.HEAPU8[ptr++]))
                str += String.fromCharCode(char);
            return str;
        };
        function loop()
        {
            requestAnimationFrame(loop);
            Module.canvas.width = innerWidth * devicePixelRatio;
            Module.canvas.height = innerHeight * devicePixelRatio;
            Module._rr_renderer_main_loop($0, Module.canvas.width, Module.canvas.height, devicePixelRatio);
        };
        requestAnimationFrame(loop);        
    }, this);
#endif
}

void rr_renderer_main_loop(struct rr_game *this, float width, float height, float device_pixel_ratio)
{
    float a = height / 1080;
    float b = width / 1920;

    this->renderer->scale = b < a ? a : b;
    this->renderer->width = width;
    this->renderer->height = height;
    rr_game_tick(this);
}


int main()
{
    puts("client init");
    struct rr_game game;
    struct rr_websocket socket;
    struct rr_renderer renderer;
    struct rr_input_data input_data;
    struct rr_simulation simulation;

    rr_game_init(&game);
    rr_websocket_init(&socket);
    rr_renderer_init(&renderer);
    rr_input_data_init(&input_data);
    rr_simulation_init(&simulation);

    game.socket = &socket;
    game.renderer = &renderer;
    game.input_data = &input_data;
    game.simulation = &simulation;
    rr_main_renderer_initialize(&game);

    rr_websocket_connect_to(&socket, "localhost", 8000);

#ifndef EMSCRIPTEN
    while (1)
    {
        struct timeval start;
        struct timeval end;
        gettimeofday(&start, NULL);
        rr_game_tick(&game);
        gettimeofday(&end, NULL);
        long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
        if (elapsed_time > 1000)
            printf("tick took %ld microseconds\n", elapsed_time);
        long to_sleep = 16666 - elapsed_time;
        usleep(to_sleep > 0 ? to_sleep : 0);
    }
#endif

    puts("main end (shouldn't see this hopefully)");
    return 0;
}