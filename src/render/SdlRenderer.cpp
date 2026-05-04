/**
 * **************************************************************************
 * @file     SdlRenderer.cpp
 * @brief    Owns the SDL2/SDL_ttf renderer behind the IRenderer boundary
 * **************************************************************************
 */

#define SDL_MAIN_HANDLED

#include "render/SdlRenderer.hpp"

#include <SDL.h>
#include <SDL_ttf.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "render/ConsoleBuffer.hpp"

namespace {

/**
 * **************************************************************************
 * LOCAL SDL HELPERS
 * **************************************************************************
 */

SDL_Color toSdlColor(rl::Color color)
{
    return SDL_Color{color.r, color.g, color.b, color.a};
}

std::uint32_t packColor(rl::Color color) noexcept
{
    return (std::uint32_t{color.r} << 24U) |
           (std::uint32_t{color.g} << 16U) |
           (std::uint32_t{color.b} << 8U) | std::uint32_t{color.a};
}

std::string sdlError(const char* prefix)
{
    return std::string(prefix) + ": " + SDL_GetError();
}

std::string ttfError(const char* prefix)
{
    return std::string(prefix) + ": " + TTF_GetError();
}

std::optional<std::string> environmentVariable(const char* name)
{
#if defined(_WIN32)
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return std::nullopt;
    }

    std::string result{value};
    // _dupenv_s hands ownership to us, so free it the CRT way
    std::free(value);
    return result;
#else
    if (const char* value = std::getenv(name)) {
        return std::string{value};
    }
    return std::nullopt;
#endif
}

rl::Key mapSdlKey(SDL_Keycode key)
{
    switch (key) {
    case SDLK_UP:
        return rl::Key::Up;
    case SDLK_DOWN:
        return rl::Key::Down;
    case SDLK_LEFT:
        return rl::Key::Left;
    case SDLK_RIGHT:
        return rl::Key::Right;
    case SDLK_KP_1:
        return rl::Key::Numpad1;
    case SDLK_KP_2:
        return rl::Key::Numpad2;
    case SDLK_KP_3:
        return rl::Key::Numpad3;
    case SDLK_KP_4:
        return rl::Key::Numpad4;
    case SDLK_KP_6:
        return rl::Key::Numpad6;
    case SDLK_KP_7:
        return rl::Key::Numpad7;
    case SDLK_KP_8:
        return rl::Key::Numpad8;
    case SDLK_KP_9:
        return rl::Key::Numpad9;
    case SDLK_a:
        return rl::Key::A;
    case SDLK_b:
        return rl::Key::B;
    case SDLK_c:
        return rl::Key::C;
    case SDLK_d:
        return rl::Key::D;
    case SDLK_e:
        return rl::Key::E;
    case SDLK_f:
        return rl::Key::F;
    case SDLK_g:
        return rl::Key::G;
    case SDLK_h:
        return rl::Key::H;
    case SDLK_i:
        return rl::Key::I;
    case SDLK_j:
        return rl::Key::J;
    case SDLK_k:
        return rl::Key::K;
    case SDLK_l:
        return rl::Key::L;
    case SDLK_m:
        return rl::Key::M;
    case SDLK_n:
        return rl::Key::N;
    case SDLK_o:
        return rl::Key::O;
    case SDLK_p:
        return rl::Key::P;
    case SDLK_q:
        return rl::Key::Q;
    case SDLK_r:
        return rl::Key::R;
    case SDLK_s:
        return rl::Key::S;
    case SDLK_t:
        return rl::Key::T;
    case SDLK_u:
        return rl::Key::U;
    case SDLK_v:
        return rl::Key::V;
    case SDLK_w:
        return rl::Key::W;
    case SDLK_x:
        return rl::Key::X;
    case SDLK_y:
        return rl::Key::Y;
    case SDLK_z:
        return rl::Key::Z;
    case SDLK_ESCAPE:
        return rl::Key::Escape;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        return rl::Key::Enter;
    case SDLK_F5:
        return rl::Key::F5;
    case SDLK_PERIOD:
    case SDLK_GREATER:
        return rl::Key::Descend;
    default:
        return rl::Key::Unknown;
    }
}

} // namespace

namespace rl {

/**
 * **************************************************************************
 * SDL RESOURCE LIFETIME
 * **************************************************************************
 */

SdlRenderer::SdlContext::SdlContext()
{
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(sdlError("SDL_Init failed"));
    }
}

SdlRenderer::SdlContext::~SdlContext()
{
    SDL_Quit();
}

SdlRenderer::TtfContext::TtfContext()
{
    if (TTF_Init() != 0) {
        throw std::runtime_error(ttfError("TTF_Init failed"));
    }
}

SdlRenderer::TtfContext::~TtfContext()
{
    TTF_Quit();
}

void SdlRenderer::FontDeleter::operator()(TTF_Font* font) const noexcept
{
    if (font != nullptr) {
        TTF_CloseFont(font);
    }
}

void SdlRenderer::WindowDeleter::operator()(SDL_Window* window) const noexcept
{
    if (window != nullptr) {
        SDL_DestroyWindow(window);
    }
}

void SdlRenderer::RendererDeleter::operator()(
    SDL_Renderer* renderer) const noexcept
{
    if (renderer != nullptr) {
        SDL_DestroyRenderer(renderer);
    }
}

void SdlRenderer::TextureDeleter::operator()(
    SDL_Texture* texture) const noexcept
{
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
    }
}

bool SdlRenderer::GlyphCacheKey::operator==(
    const GlyphCacheKey& other) const noexcept
{
    return glyph == other.glyph && foregroundRgba == other.foregroundRgba;
}

std::size_t SdlRenderer::GlyphCacheKeyHash::operator()(
    const GlyphCacheKey& key) const noexcept
{
    const auto glyphHash = std::hash<char32_t>{}(key.glyph);
    const auto colorHash = std::hash<std::uint32_t>{}(key.foregroundRgba);
    return glyphHash ^ (colorHash + static_cast<std::size_t>(0x9E3779B9U) +
                        (glyphHash << 6U) + (glyphHash >> 2U));
}

/**
 * **************************************************************************
 * RENDERER SETUP
 * **************************************************************************
 */

SdlRenderer::SdlRenderer(int consoleWidth, int consoleHeight)
{
    if (consoleWidth <= 0 || consoleHeight <= 0) {
        throw std::invalid_argument(
            "Renderer console dimensions must be positive");
    }

    const auto fontPath = findFontPath();
    font_.reset(TTF_OpenFont(fontPath.string().c_str(), 16));
    if (!font_) {
        throw std::runtime_error(
            ttfError(("Failed to load font " + fontPath.string()).c_str()));
    }

    int measuredWidth = 0;
    int measuredHeight = 0;
    if (TTF_SizeUTF8(font_.get(), "M", &measuredWidth, &measuredHeight) == 0) {
        cellWidth_ = measuredWidth > 0 ? measuredWidth : cellWidth_;
    }
    cellHeight_ = TTF_FontHeight(font_.get()) > 0 ? TTF_FontHeight(font_.get())
                                                  : cellHeight_;

    window_.reset(SDL_CreateWindow("Roguelike",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   consoleWidth * cellWidth_,
                                   consoleHeight * cellHeight_,
                                   SDL_WINDOW_SHOWN));
    if (!window_) {
        throw std::runtime_error(sdlError("SDL_CreateWindow failed"));
    }

    renderer_.reset(SDL_CreateRenderer(window_.get(),
                                       -1,
                                       SDL_RENDERER_ACCELERATED |
                                           SDL_RENDERER_PRESENTVSYNC));
    if (!renderer_) {
        renderer_.reset(
            SDL_CreateRenderer(window_.get(), -1, SDL_RENDERER_SOFTWARE));
    }
    if (!renderer_) {
        throw std::runtime_error(sdlError("SDL_CreateRenderer failed"));
    }
}

SdlRenderer::~SdlRenderer() = default;

/**
 * **************************************************************************
 * FRAME FLOW
 * **************************************************************************
 */

std::vector<InputEvent> SdlRenderer::pollEvents()
{
    std::vector<InputEvent> events;
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            shouldQuit_ = true;
            events.push_back(InputEvent{InputEventType::Quit});
        } else if (event.type == SDL_KEYDOWN) {
            events.push_back(InputEvent{InputEventType::KeyDown,
                                        mapSdlKey(event.key.keysym.sym)});
        }
    }
    return events;
}

void SdlRenderer::present(const ConsoleBuffer& console)
{
    SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
    SDL_RenderClear(renderer_.get());

    for (int y = 0; y < console.height(); ++y) {
        for (int x = 0; x < console.width(); ++x) {
            drawCell(x, y, console.cellAt(x, y));
        }
    }

    SDL_RenderPresent(renderer_.get());
}

bool SdlRenderer::shouldQuit() const noexcept
{
    return shouldQuit_;
}

std::filesystem::path SdlRenderer::findFontPath()
{
    // Let local dev override font lookup without changing committed settings
    if (const auto envPath = environmentVariable("ROGUELIKE_FONT_PATH")) {
        const std::filesystem::path candidate{*envPath};
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
        throw std::runtime_error(
            "ROGUELIKE_FONT_PATH points to a missing font: " +
            candidate.string());
    }

    // Fall back to common monospace fonts on the usual desktop targets
    const std::array<std::filesystem::path, 8> candidates{
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/lucon.ttf",
        "C:/Windows/Fonts/cour.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/dejavu/DejaVuSansMono.ttf",
        "/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Supplemental/Courier New.ttf"};

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    throw std::runtime_error("No usable monospace font found. Set "
                             "ROGUELIKE_FONT_PATH to a .ttf or .ttc file.");
}

std::string SdlRenderer::toUtf8(char32_t glyph)
{
    // SDL_ttf takes UTF-8 text, while ConsoleBuffer stores code points
    if (glyph <= 0x7F) {
        return std::string(1, static_cast<char>(glyph));
    }
    if (glyph <= 0x7FF) {
        return {static_cast<char>(0xC0 | ((glyph >> 6) & 0x1F)),
                static_cast<char>(0x80 | (glyph & 0x3F))};
    }
    if (glyph <= 0xFFFF) {
        return {static_cast<char>(0xE0 | ((glyph >> 12) & 0x0F)),
                static_cast<char>(0x80 | ((glyph >> 6) & 0x3F)),
                static_cast<char>(0x80 | (glyph & 0x3F))};
    }

    return {static_cast<char>(0xF0 | ((glyph >> 18) & 0x07)),
            static_cast<char>(0x80 | ((glyph >> 12) & 0x3F)),
            static_cast<char>(0x80 | ((glyph >> 6) & 0x3F)),
            static_cast<char>(0x80 | (glyph & 0x3F))};
}

void SdlRenderer::drawCell(int x, int y, const Cell& cell)
{
    const SDL_Rect destination{
        x * cellWidth_, y * cellHeight_, cellWidth_, cellHeight_};
    SDL_SetRenderDrawColor(renderer_.get(),
                           cell.background.r,
                           cell.background.g,
                           cell.background.b,
                           cell.background.a);
    SDL_RenderFillRect(renderer_.get(), &destination);

    if (cell.glyph == U' ') {
        return;
    }

    // Rendering a glyph through SDL_ttf allocates transient surface/texture
    // resources, so we have to cache by glyph & foreground color instead 
    // of repeating that for every visible cell on every frame
    const GlyphCacheKey key{cell.glyph, packColor(cell.foreground)};
    auto cachedGlyph = glyphCache_.find(key);
    if (cachedGlyph == glyphCache_.end()) {
        const auto text = toUtf8(cell.glyph);
        auto* rawSurface = TTF_RenderUTF8_Blended(
            font_.get(), text.c_str(), toSdlColor(cell.foreground));
        if (rawSurface == nullptr) {
            return;
        }

        std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> surface{
            rawSurface, SDL_FreeSurface};
        std::unique_ptr<SDL_Texture, TextureDeleter> texture{
            SDL_CreateTextureFromSurface(renderer_.get(), surface.get()),
            TextureDeleter{}};
        if (!texture) {
            return;
        }

        SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND);

        CachedGlyph glyph{std::move(texture), surface->w, surface->h};
        cachedGlyph = glyphCache_.emplace(key, std::move(glyph)).first;
    }

    SDL_Rect glyphDestination{destination.x +
                                  ((cellWidth_ - cachedGlyph->second.width) /
                                   2),
                              destination.y +
                                  ((cellHeight_ - cachedGlyph->second.height) /
                                   2),
                              cachedGlyph->second.width,
                              cachedGlyph->second.height};
    SDL_RenderCopy(renderer_.get(),
                   cachedGlyph->second.texture.get(),
                   nullptr,
                   &glyphDestination);
}

} // namespace rl
