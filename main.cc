/*
   Copyright (c) 2025, e-soul.org
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification, are permitted
   provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of conditions
      and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
      and the following disclaimer in the documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <format>
#include <memory>
#include <stdexcept>
#include <string>

template<auto InitFunc, auto QuitFunc>
struct SdlLibrary {
    template<typename... Args>
    explicit SdlLibrary(Args &&... args) {
        if (!InitFunc(std::forward<Args>(args)...)) {
#ifdef _MSC_VER
            constexpr std::string_view func = __FUNCSIG__;
#else
            constexpr std::string_view func = __PRETTY_FUNCTION__;
#endif
            throw std::runtime_error(std::format("Initialization error: {} - {}", SDL_GetError(), func));
        }
    }

    ~SdlLibrary() {
        QuitFunc();
    }

    SdlLibrary(const SdlLibrary &) = delete;
    SdlLibrary(SdlLibrary &&) = delete;
    SdlLibrary &operator=(const SdlLibrary &) = delete;
    SdlLibrary &operator=(SdlLibrary &&) = delete;
};
using SdlCore = SdlLibrary<SDL_Init, SDL_Quit>;
using SdlTtf = SdlLibrary<TTF_Init, TTF_Quit>;

template<class T, void(*DeleterFunc)(T*)>
using SdlUniquePtr = std::unique_ptr<std::remove_pointer_t<T>, decltype(DeleterFunc)>;
using SdlWindowPtr = SdlUniquePtr<SDL_Window, SDL_DestroyWindow>;
using SdlTrayPtr = SdlUniquePtr<SDL_Tray, SDL_DestroyTray>;
using TtfFontPtr = SdlUniquePtr<TTF_Font, TTF_CloseFont>;
using SdlRendererPtr = SdlUniquePtr<SDL_Renderer, SDL_DestroyRenderer>;
using SdlTexturePtr = SdlUniquePtr<SDL_Texture, SDL_DestroyTexture>;

#if defined(SDL_PLATFORM_WIN32)
// cl /EHs /std:c++20 main.cc /I D:\dev\lib\SDL3-3.2.10\include /I D:\dev\lib\SDL3_ttf-3.2.2\include /link /Core:WINDOWS /LIBPATH:D:\dev\lib\SDL3-3.2.10\lib\x86 /LIBPATH:D:\dev\lib\SDL3_ttf-3.2.2\lib\x86 SDL3.lib SDL3_ttf.lib user32.lib
#include <windows.h>

void setClickThrough(SDL_Window* window) {
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (hwnd) {
        LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
        style |= WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT;
        SetWindowLong(hwnd, GWL_EXSTYLE, style);
    } else {
        SDL_Log("Failed to get HWND: %s", SDL_GetError());
    }
}
#else
// deps: SDL3-devel SDL3_ttf-devel
// g++ main.cc -std=c++20 -o main -lX11 -lXfixes `pkg-config --cflags --libs sdl3 sdl3-ttf`
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>

void setClickThrough(SDL_Window* window) {
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        SDL_Log("X11 detected");
        Display *xdisplay = (Display *)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
        Window xwindow = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        if (!xdisplay || !xwindow) {
            SDL_Log("Failed to get X11 display and window!");
            return;
        }
        XserverRegion empty = XFixesCreateRegion(xdisplay, NULL, 0);
        XFixesSetWindowShapeRegion(xdisplay, xwindow, ShapeInput, 0, 0, empty);
        XFixesDestroyRegion(xdisplay, empty);
        XFlush(xdisplay);
    } else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
        SDL_Log("Wayland detected");
        struct wl_display *display = (struct wl_display *)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
        struct wl_surface *surface = (struct wl_surface *)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
        if (!display || !surface) {
            SDL_Log("Failed to get Wayland display and surface!");
        }
        SDL_Log("Mouse event transparency (click-through) is not supported on Wayland.");
    } else {
        SDL_Log("Unsupported video driver: %s", SDL_GetCurrentVideoDriver());
    }
}
#endif

void onQuit(void *userdata, SDL_TrayEntry *invoker)
{
    SDL_Event e;
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
}

int main(int argc, char **argv)
{
    SdlCore sdlCore(SDL_INIT_VIDEO);
    SdlTtf sdlTtf;

    SdlTrayPtr tray(SDL_CreateTray(NULL, "Generic OverLay"), SDL_DestroyTray);
    if (!tray) {
        SDL_Log("Tray creation failed: %s", SDL_GetError());
        return 1;
    }

    SDL_TrayMenu *menu = SDL_CreateTrayMenu(tray.get());
    if (!menu) {
        SDL_Log("Tray menu creation failed: %s", SDL_GetError());
        return 1;
    }

    SDL_TrayEntry *entry = SDL_InsertTrayEntryAt(menu, -1, "Quit", SDL_TRAYENTRY_BUTTON);
    if (!entry) {
        SDL_Log("Tray entry creation failed: %s", SDL_GetError());
        return 1;
    }
    SDL_SetTrayEntryCallback(entry, onQuit, NULL);

    SdlWindowPtr window(SDL_CreateWindow("Generic OverLay", 500, 400, SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_UTILITY | SDL_WINDOW_BORDERLESS | SDL_WINDOW_TRANSPARENT),
                        SDL_DestroyWindow);
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        return 1;
    }

    SdlRendererPtr renderer(SDL_CreateRenderer(window.get(), NULL), SDL_DestroyRenderer);
    if (!renderer) {
        SDL_Log("Renderer creation failed: %s", SDL_GetError());
        return 1;
    }

    setClickThrough(window.get());

    SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);

    TtfFontPtr font(TTF_OpenFont("IntelOneMono-Medium.ttf", 24), TTF_CloseFont);
    if (!font) {
        SDL_Log("Font load failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Color color = {255, 0, 0, 255};
    const char *text = "Generic Overlay";
    SDL_Surface *surface = TTF_RenderText_Blended(font.get(), text, SDL_strlen(text), color);
    if (!surface) {
        SDL_Log("TTF_RenderText failed: %s", SDL_GetError());
        return 1;
    }

    SdlTexturePtr texture(SDL_CreateTextureFromSurface(renderer.get(), surface), SDL_DestroyTexture);
    if (!texture) {
        SDL_Log("Texture creation failed: %s", SDL_GetError());
        return 1;
    }
    SDL_DestroySurface(surface);

    float texW = 0, texH = 0;
    SDL_GetTextureSize(texture.get(), &texW, &texH);

    SDL_FRect dstRect = {
        (500.0f - texW) / 2.0f,
        (400.0f - texH) / 2.0f,
        texW,
        texH};

    SDL_Event event;

    bool running = true;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running = false;
            }
        }

        SDL_SetRenderDrawColor(renderer.get(), 0, 144, 0, 100);
        SDL_RenderClear(renderer.get());

        SDL_RenderTexture(renderer.get(), texture.get(), NULL, &dstRect);
        SDL_RenderPresent(renderer.get());
    }

    return 0;
}
