#include <cpu.hpp> 
#include <algorithm> 

i8080::i8080()
: pc(0), sp(0), a(0), b(0), c(0), d(0), e(0), h(0), l(0), flag(0), z(0), s(0), p(0), cy(0), ac(0), pad(0)
{
    std::fill(std::begin(memory), std::end(memory), 0x00);
    sp = sizeof(memory) - 1; 
}

void i8080::execute_nop()
{
    pc +=1; 
}

void i8080::unimplemented_instruction()
{
    pc -=1; 
    printf("error, unimplemented instruction"); 
    exit(1); 
}

int i8080::emulate()
{
    unsigned char opcode = memory[pc]; 
    switch(opcode)
    {
        
    }
}





