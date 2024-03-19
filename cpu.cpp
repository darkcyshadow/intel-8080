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

void i8080::ADD(uint8_t reg)
{
    uint16_t result = a + reg; 
    handle_arith_flag(result); 
    a = result & 0xff;

}

void i8080:: ADC(uint8_t reg)
{
    uint16_t result = a + c + reg; 
    handle_arith_flag(result); 
    a = result & 0xff; 
}

void i8080:: ANA(uint8_t reg)
{
    uint16_t result = a & reg; 
    handle_arith_flag(result); 
    a = result & 0xff; 
}

// the call routine works by first saving the return address, we increment pc by 2 as the address that is being called is 2 bytes 
// save return address onto stack 
// set pc to the address that is being called
void i8080:: CALL()
{
    uint16_t return_address = pc + 2; 
    memory[sp - 1] = (return_address >> 8) & 0xff; 
    memory[sp - 2] = return_address & 0xff;
    sp = sp - 2; 
    pc = opcode[2] << 8 | opcode[1]; 
}

void i8080::CMA()
{
    a = ~a; 
}

void i8080::CMC()
{
    c = ~c; 
}

void i8080::CMP(uint8_t reg)
{   
    uint16_t result = a - reg; 
    c = (a < reg); 
    z = (result == 0); 
    s = ((result & 0x80) != 0); 
    p = parity(result & 0xff); 
}





int i8080::emulate()
{
    // opcode is a pointer to the location in memory where the instruction is stored 
    opcode = &memory[pc];
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
            uint16_t result = ++b; 
            handle_arith_flag(result); 
        // dcr, b 
        case 0x05: 
            uint16_t result = --b;
        // mvi, b, d8
        case 0x06: 
            uint16_t byte = opcode[1]; 
            b = byte;
        // rlc 
        case 0x07: 
            uint16_t temp = a; 
            c = temp >> 7;
            a = temp << 1 | temp >> 7;  
        // nop 
        case 0x08: 
            break; 
        // dad b 
        case 0x09: 
            uint16_t hl = h << 8 | l; 
            uint16_t bc = b << 8 | c;
            hl += bc; 
            handle_arith_flag(hl); 
        // ldax b 
        case 0xa: 
           uint16_t bc = b << 8 | c; 
           a = memory[bc]; 
        // dcx b
        case 0xb: 
           uint16_t bc = b << 8 | c; 
           --bc; 
        // inr c 
        case 0xc: 
            uint16_t result = ++c; 
            handle_arith_flag(result); 
        // dcr, c 
        case 0xd: 
            uint16_t result = --c; 
            handle_arith_flag(result); 
        // mvi, c, d8 
        case 0xe: 
            uint16_t value = opcode[1]; 
            c = value; 
        // rrc
        case 0xf: 
            uint16_t temp = a; 
            a = temp >> 1 | temp << 7;
            c = a >> 7; 
        // nop
        case 0x10: 
            break; 
        // lxi d, d16 
        case 0x11: 
            e = opcode[1]; 
            d = opcode[2]; 
            pc += 2; 
            break; 
        // stax d 
        case 0x12: 
            uint16_t de = d << 8 | c; 
            memory[de] = a; 
        // inx d 
        case 0x13: 
            ++e; 
            if (e == 0)
            {
                ++d; 
            }
        // inr, d 
        case 0x14: 
            uint16_t result = ++d; 
            handle_arith_flag(result); 
        // dcr, d 
        case 0x15: 
            uint16_t result = --d; 
            handle_arith_flag(result);
        // mvi d, d8 
        case 0x16: 
            uint16_t result = opcode[1]; 
            d = result; 
        // ral 
        case 0x17: 
            uint16_t temp = a; 
            a = temp << 1 | c; 
            c = temp >> 7; 
        // nop 
        case 0x18: 
            break; 
        // dad d 
        case 0x19: 
            uint16_t hl = h << 8 | l; 
            uint16_t de = h << 8 | e; 
            uint32_t result = hl + de;
            c = ((result & 0xffff) > 0); 
            h = (result & 0xff) >> 8; 
            l = result & 0xff; 



    }
}







