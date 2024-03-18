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
    pc += 1;
}

void i8080::unimplemented_instruction()
{
    pc -= 1;
    printf("error, unimplemented instruction");
    exit(1);
}

int i8080::parity(uint16_t result)
{
    int count = 0;
    while (result != 0)
    {
        result = result & (result - 1);
        ++count;
    }
    if (count % 2 == 0)
    {
        return 1;
    }
    else
        return 0;
}

// 0x80 = 1000 0000
// 0xff = 1111 1111

// we bitmask before storing to ensure it fits in an 8 bit register 
void i8080::handle_arith_flag(uint16_t result)
{
    // zero flag - set to 1 when result == 0
    z = ((result & 0xff) == 0); 
    // sign flag - set to 1 when msb is set(negative)
    s = ((result & 0x80) != 0); 
    // cy flag - set to 1 when instruction resultd in a carry 
    cy = (result > 0xff); 
    // parity flag - set to 1 if result is even
    p = parity(result & 0xff); 
    
}


/* there are diff formms of each type of instruction 
arithmetic: 
    register - where the values to be operated are stored in registers
    immediate - where the source of the addend is the byte after the instruction,
    memory - the addend is the byte pointed to by the address stored in the HL register pair 
branch:
    JNZ - jump if the zero flag is not set
    JZ - jump if zero flag is set 
    JNC - jump if carry flag is not set
    JC - jump if carry flag is set 
    JPO - jump if parity if odd 
    JPE - jump if parity is even (set) 
    JP - jump if sign flag is not set (+)
    JM - jump if sign flag is set (-)
CALL/RET: 
    CNZ/RNZ - call/ret if zero flag not set 
    CZ/RZ - call/ret if zero flag is set 
    CNC/RNC - call/ret if carry flag is not set 
    CC/RC - call/ret if carry flag is set 
    CPO/RPO - call/ret if partiy is odd
    CPE/RPE - call/ret if parity is even (set)
    CP/RP - call/ret if sign is not set (+)
    CM/RM - call/ret if sign is set (-)
logical: 

*/

int i8080::emulate()
{
    // opcode is a pointer to the location in memory where the instruction is stored 
    unsigned char* opcode = &memory[pc];
    switch (*opcode)
    {
        //nop
        case 0x00: break; 
        // lxi b, word
        case 0x01: 
            c = opcode[1]; 
            b = opcode[2]; 
            pc +=2; 
            break; 
        // stax, b 
        case 0x02: 
            uint16_t bc = b << 8 | c; 
            memory[bc] = a; 
        // inx, b
        case 0x03: 
            ++c; 
            if (c == 0)
            {
                ++b;
            }
        // inr, b
        case 0x04: 




    }
}







