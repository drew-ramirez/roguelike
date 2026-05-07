/**
 * **************************************************************************
 * @file     SdlRenderer.hpp
 * @brief    Declares the SDL2 renderer behind the console renderer API
 * **************************************************************************
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "render/IRenderer.hpp"

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;
struct TTF_Font;

namespace rl {

struct Cell;

class SdlRenderer final : public IRenderer {
  public:
    /**
     * Creates a console-sized SDL window using the project font lookup rules
     *
     * @param consoleWidth Width of the logical console in cells
     * @param consoleHeight Height of the logical console in cells
     */
    SdlRenderer(int consoleWidth, int consoleHeight);
    ~SdlRenderer() override;

    SdlRenderer(const SdlRenderer&) = delete;
    SdlRenderer& operator=(const SdlRenderer&) = delete;
    SdlRenderer(SdlRenderer&&) = delete;
    SdlRenderer& operator=(SdlRenderer&&) = delete;

    [[nodiscard]] std::vector<InputEvent> pollEvents() override;
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

    struct TextureDeleter {
        void operator()(SDL_Texture* texture) const noexcept;
    };

    struct GlyphCacheKey {
        char32_t glyph = U' ';
        std::uint32_t foregroundRgba = 0;

        [[nodiscard]] bool
        operator==(const GlyphCacheKey& other) const noexcept;
    };

    struct GlyphCacheKeyHash {
        [[nodiscard]] std::size_t
        operator()(const GlyphCacheKey& key) const noexcept;
    };

    struct CachedGlyph {
        std::unique_ptr<SDL_Texture, TextureDeleter> texture;
        int width = 0;
        int height = 0;
    };

    using FontPtr = std::unique_ptr<TTF_Font, FontDeleter>;
    using WindowPtr = std::unique_ptr<SDL_Window, WindowDeleter>;
    using RendererPtr = std::unique_ptr<SDL_Renderer, RendererDeleter>;
    using GlyphCache =
        std::unordered_map<GlyphCacheKey, CachedGlyph, GlyphCacheKeyHash>;

    [[nodiscard]] static std::filesystem::path findFontPath();
    [[nodiscard]] static std::string toUtf8(char32_t glyph);

    void drawCell(int x, int y, const Cell& cell);

    SdlContext sdl_;
    TtfContext ttf_;
    FontPtr font_;
    WindowPtr window_;
    RendererPtr renderer_;
    // Glyph textures are retained between frames: creating
    // SDL_ttf surfaces & SDL textures per cell bloats memory during
    // longer sessions
    GlyphCache glyphCache_;
    int cellWidth_ = 12;
    int cellHeight_ = 16;
    bool shouldQuit_ = false;
};

} // namespace rl
