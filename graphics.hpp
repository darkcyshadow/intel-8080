#include <stdint.h> 
#include <SDL2/SDL.h> 
#include <stdio.h> 
#include <cpu.hpp>

#ifndef GRAPHICS_H 
#define GRAPHICS_H

class Graphics
{
    public:    
        Graphics(const char* title, u_int16_t width, u_int16_t height, u_int16_t pixel_size); 
        void update(uint8_t* vram); 
    private: 
        u_int16_t pixel_size; 
        uint16_t width; 
        u_int16_t height; 
        u_int16_t pixels[224 * 256]; 
        SDL_Window* main_window; 
        SDL_Renderer* main_renderer; 
        SDL_Texture* main_texture; 
}
#endif