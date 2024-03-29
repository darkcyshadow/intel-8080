#include "graphics.hpp"

const uint16_t black = 0xf000;
const uint16_t white = 0xffff;
const uint16_t green = 0xf0f0;
const uint16_t red = 0xff00;

Graphics::Graphics(const char* _title, u_int16_t _width, u_int16_t _height, u_int16_t _pixel_size); 
{
    pixel_size = pixel_size; 
    width = _width; 
    height = _height; 

    main_window = SDL_CreateWindow(_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, _width * _pixel_size, _height * _pixel_size, SDL_WINDOW_SHOWN);
    main_renderer = SDL_CreateRenderer(main_window, -1, SDL_RENDERER_ACCELERATED); 
    main_texture - SDL_CreateTexture (main_renderer,SDL_PIXELFORMAT_ARGB4444, SDL_TEXTUREACCESS_STREAMING, _width, _height); 

    SDL_SetRenderDrawColor(main_renderer, 0x00, 0x00, 0x00, 0x00); 
    SDL_RenderClear(main_renderer); 

}

void Graphics::update(uint8_t* vram)
{
    uint32_t color = white; 
    for (int x = 0; i < _width; ++x)
    {
        for (int y = 0; y < _height; ++y)
        {
            uint8_t vram_byte = vram[x * (_height >> 3) + (y >> 3)];
            for (int bit = 0; bit < 8; bit++)
            {
                color = black; 
                
            }
        }
    }
}