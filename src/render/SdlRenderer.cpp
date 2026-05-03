#define SDL_MAIN_HANDLED

#include "render/SdlRenderer.hpp"

#include <SDL.h>
#include <SDL_ttf.h>

#include <array>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>

#include "render/ConsoleBuffer.hpp"

namespace {

SDL_Color toSdlColor(rl::Color color)
{
    return SDL_Color{color.r, color.g, color.b, color.a};
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
    case SDLK_w:
        return rl::Key::W;
    case SDLK_a:
        return rl::Key::A;
    case SDLK_s:
        return rl::Key::S;
    case SDLK_d:
        return rl::Key::D;
    default:
        return rl::Key::Unknown;
    }
}

} // namespace

namespace rl {

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
    if (const auto envPath = environmentVariable("ROGUELIKE_FONT_PATH")) {
        const std::filesystem::path candidate{*envPath};
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
        throw std::runtime_error(
            "ROGUELIKE_FONT_PATH points to a missing font: " +
            candidate.string());
    }

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

    const auto text = toUtf8(cell.glyph);
    auto* rawSurface = TTF_RenderUTF8_Blended(
        font_.get(), text.c_str(), toSdlColor(cell.foreground));
    if (rawSurface == nullptr) {
        return;
    }

    std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> surface{
        rawSurface, SDL_FreeSurface};
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture{
        SDL_CreateTextureFromSurface(renderer_.get(), surface.get()),
        SDL_DestroyTexture};
    if (!texture) {
        return;
    }

    SDL_Rect glyphDestination{destination.x + ((cellWidth_ - surface->w) / 2),
                              destination.y + ((cellHeight_ - surface->h) / 2),
                              surface->w,
                              surface->h};
    SDL_RenderCopy(renderer_.get(), texture.get(), nullptr, &glyphDestination);
}

} // namespace rl
