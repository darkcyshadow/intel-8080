#include "graphics.hpp"


Graphics::Graphics(const char* _title, u_int16_t _width, u_int16_t _height, u_int16_t _pixel_size); 
{
    pixel_size = _pixel_size; 
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
    for (int x = 0; x < _width; ++x)
    {
        for (int y = 0; y < _height; ++y)
        {
            uint8_t vram_byte = vram[x * (_height >> 3) + (y >> 3)];
            for (int bit = 0; bit < 8; bit++)
            {
                uint8_t x_coord = x ; 
                uint8_t y_coord = (_height - 1 - (y + bit)); 
                pixels[y_coord * _width + x_coord] = 0xffff; 
            }
        }
    }
    SDL_UpdateTexture(main_texture, NULL, pixels, 2 * _width); 
    SDL_RenderCopy (main_renderer, main_texture, NULL, NULL); 
    SDL_RenderPresent(main_renderer); 
}