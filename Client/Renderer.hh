#pragma once

#include <cstdint>
#include <map>

class SkCanvas;

namespace app
{
    class Renderer
    {
#ifndef WASM_BUILD
        SkCanvas *m_Canvas;
#endif
        class Simulation &m_Simulation;

    public:
        int32_t m_Width;
        int32_t m_Height;
        std::map<uint8_t, uint8_t> m_KeysPressed{};

        class Paint
        {
        public:
            enum class Style : uint8_t
            {
                Fill,
                Stroke
            };

            enum class Cap : uint8_t
            {
                Butt,
                Round,
                Square
            };

            enum class Join : uint8_t
            {
                Mitter,
                Round,
                Bevel
            };

            Style m_Style = Style::Fill;
            Join m_Join = Join::Round;
            Cap m_Cap = Cap::Butt;
            bool m_AntiAliased = true;
            uint32_t m_Color = 0;
            float m_StrokeWidth = 1;
        };

        Renderer(Simulation &);

        void Initialize();
        void SetSize(int32_t width, int32_t height);
        void Translate(float, float);
        void Scale(float, float);
        void DrawCircle(float, float, float, Paint const &);
        void Render();
        void Save();
        void Restore();
        void Clear();
    };
}