#pragma once

/// SDL2-based display window for 1-bit bitmap rendering.
/// Owns the SDL window and texture, provides a single update_screen() method
/// to blit a macflim::bitmap to the display. Handles SDL init/teardown via RAII.

#include "bitmap.hpp"

#include <SDL.h>

#include <cstddef>
#include <format>
#include <stdexcept>
#include <string_view>

namespace macflim
{

class sdl_display
{
  public:
    sdl_display(size_t width, size_t height, size_t scale, std::string_view title, bool vsync = true)
        : width_{width}, height_{height}, scale_{scale}
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0)
            throw std::runtime_error(std::format("SDL_Init failed: {}", SDL_GetError()));

        window_ = SDL_CreateWindow(std::string(title).c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   static_cast<int>(width * scale), static_cast<int>(height * scale), 0);
        if (!window_)
            throw std::runtime_error(std::format("SDL_CreateWindow failed: {}", SDL_GetError()));

        Uint32 renderer_flags = SDL_RENDERER_ACCELERATED;
        if (vsync)
            renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
        renderer_ = SDL_CreateRenderer(window_, -1, renderer_flags);
        if (!renderer_)
            throw std::runtime_error(std::format("SDL_CreateRenderer failed: {}", SDL_GetError()));

        //  1-bit bitmap → 8-bit grayscale texture, scaled by the renderer
        texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGB332, SDL_TEXTUREACCESS_STREAMING,
                                     static_cast<int>(width), static_cast<int>(height));
        if (!texture_)
            throw std::runtime_error(std::format("SDL_CreateTexture failed: {}", SDL_GetError()));
    }

    ~sdl_display()
    {
        if (texture_)
            SDL_DestroyTexture(texture_);
        if (renderer_)
            SDL_DestroyRenderer(renderer_);
        if (window_)
            SDL_DestroyWindow(window_);
        SDL_Quit();
    }

    sdl_display(const sdl_display &) = delete;
    sdl_display &operator=(const sdl_display &) = delete;

    /// Blit a 1-bit bitmap to the SDL window. Bit set = black, bit clear = white (Mac convention).
    void update_screen(const bitmap &bm)
    {
        uint8_t *pixels = nullptr;
        int pitch = 0;
        SDL_LockTexture(texture_, nullptr, reinterpret_cast<void **>(&pixels), &pitch);

        //  The bitmap's data_ is row-major, 1 bit per pixel, MSB=leftmost, set=black
        auto image = bm.as_image();
        for (size_t y = 0; y < height_; ++y)
            for (size_t x = 0; x < width_; ++x)
                pixels[y * pitch + x] = image.at(x, y) ? 0xFF : 0x00;

        SDL_UnlockTexture(texture_);
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
        SDL_RenderPresent(renderer_);
    }

    /// Poll for quit event. Returns true if the user closed the window or pressed Escape.
    [[nodiscard]] bool should_quit() const
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                return true;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                return true;
        }
        return false;
    }

  private:
    size_t width_;
    size_t height_;
    size_t scale_;
    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    SDL_Texture *texture_ = nullptr;
};

} // namespace macflim
