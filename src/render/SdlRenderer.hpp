#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "render/IRenderer.hpp"

struct SDL_Renderer;
struct SDL_Window;
struct TTF_Font;

namespace rl {

struct Cell;

class SdlRenderer final : public IRenderer {
public:
    SdlRenderer(int consoleWidth, int consoleHeight);
    ~SdlRenderer() override;

    SdlRenderer(const SdlRenderer&) = delete;
    SdlRenderer& operator=(const SdlRenderer&) = delete;
    SdlRenderer(SdlRenderer&&) = delete;
    SdlRenderer& operator=(SdlRenderer&&) = delete;

    void pollEvents() override;
    void present(const ConsoleBuffer& console) override;
    [[nodiscard]] bool shouldQuit() const noexcept override;

private:
    struct SdlContext {
        SdlContext();
        ~SdlContext();
    };

    struct TtfContext {
        TtfContext();
        ~TtfContext();
    };

    struct FontDeleter {
        void operator()(TTF_Font* font) const noexcept;
    };

    struct WindowDeleter {
        void operator()(SDL_Window* window) const noexcept;
    };

    struct RendererDeleter {
        void operator()(SDL_Renderer* renderer) const noexcept;
    };

    using FontPtr = std::unique_ptr<TTF_Font, FontDeleter>;
    using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;
    using RendererPtr = std::unique_ptr<SDL_Renderer, RendererDeleter>;

    [[nodiscard]] static std::filesystem::path findFontPath();
    [[nodiscard]] static std::string toUtf8(char32_t glyph);

    void drawCell(int x, int y, const Cell& cell);

    SdlContext sdl_;
    TtfContext ttf_;
    FontPtr font_;
    WindowPtr window_;
    RendererPtr renderer_;
    int cellWidth_ = 12;
    int cellHeight_ = 16;
    bool shouldQuit_ = false;
};

} // namespace rl
