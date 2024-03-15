#include <cstdlib>
#include <ctime> 
#include <stdint.h>

class intel_cpu_8080
{
    private: 
        uint8_t memory[0xFFFF]; 
        uint16_t pc; 
        uint16_t sp; 
        uint8_t a; 
        uint8_t b; 
        uint8_t c; 
        uint8_t d; 
        uint8_t e; 
        uint8_t h; 
        uint8_t l; 
        uint8_t flag; 
        // status flags 
        uint8_t z:1; 
        uint8_t s:1; 
        uint8_t p:1; 
        uint8_t cy:1; 
        uint8_t ac:1; 
        uint8_t pad:3;


    public: 
        intel_cpu_8080(); 
        ~intel_cpu_8080(); 

        void load_program(const uint8_t* program, size_t size, uint16_t address); 
        void run();
        void execute_intruction(uint8_t opcode); 
        

};