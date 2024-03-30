#include "cpu.cpp"
#include "graphics.hpp"

void load_rom(const char* file_name)
{
    std::ifstream file(file_name, std::ios::binary | std::ios::ate)
    std::streampos size = file.tellg(); 
    size_t memory_size = sizeof(memory) - 0xe0ff; 

    if (size > memory_size) { return; }

    file.seekg(0, std::ios::beg); 
    file.reaad((char *)memory , size); 
    file.close(); 
}

int main(int argc, char* argv[])
{
    i8080 cpu; 

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        SDL_Quit(); 
        exit(1); 
    }

    Graphics graphics; 

    load_rom(argv[2]); 
    graphics = new Graphics("intel 8080", 224, 256, 2); 

    SDL_Event e; 


    
}


