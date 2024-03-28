#include "cpu.hpp"
#include <algorithm>



// From https://github.com/superzazu/8080
const uint8_t instruction_cycles[] = {
//  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7,  4,  // 0
	4,  10, 7,  5,  5,  5,  7,  4,  4,  10, 7,  5,  5,  5,  7,  4,  // 1
	4,  10, 16, 5,  5,  5,  7,  4,  4,  10, 16, 5,  5,  5,  7,  4,  // 2
	4,  10, 13, 5,  10, 10, 10, 4,  4,  10, 13, 5,  5,  5,  7,  4,  // 3
	5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 4
	5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 5
	5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,  // 6
	7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  5,  5,  7,  5,  // 7
	4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // 8
	4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // 9
	4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // A
	4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,  // B
	5,  10, 10, 10, 11, 11, 7,  11, 5,  10, 10, 10, 11, 11, 7,  11, // C
	5,  10, 10, 10, 11, 11, 7,  11, 5,  10, 10, 10, 11, 11, 7,  11, // D
	5,  10, 10, 18, 11, 11, 7,  11, 5,  5,  10, 5,  11, 11, 7,  11, // E
	5,  10, 10, 4,  11, 11, 7,  11, 5,  5,  10, 4,  11, 11, 7,  11  // F
};

i8080::i8080()
{
    interrupts_enabled = 0; 
    halt = 0; 
    clock_count = 0; 

    std::fill(in_port, in_port + 4, 0);
    std::fill(in_port, in_port + 7, 0);

    in_port[0] |= 1 << 1;
	in_port[0] |= 1 << 2;
	in_port[0] |= 1 << 3;
	in_port[1] |= 1 << 3;
    a = 0; 
    b = 0; 
    c = 0; 
    d = 0; 
    e = 0; 
    h = 0; 
    l = 0; 

    sp = 0; 
    pc = 0; 

    s = 0; 
    z = 0;
    ac = 0;
    cy = 0; 
    p = 0;  
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
    ac = (result > 0x09);
}

void i8080::handle_without_carry(uint16_t result)
{
    // zero flag - set to 1 when result == 0
    z = ((result & 0xff) == 0);
    // sign flag - set to 1 when msb is set(negative)
    s = ((result & 0x80) != 0);
    // parity flag - set to 1 if result is even
    p = parity(result & 0xff);
    ac = (result > 0x09);
}

void i8080::unimplemented_instruction()
{
    --pc; 
    exit(1); 
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

void i8080::ADD(uint8_t *reg)
{
    uint16_t result = a + *reg;
    handle_arith_flag(result);
    a = result & 0xff;
}

void i8080::ADC(uint8_t *reg)
{
    uint16_t result = a + c + *reg;
    handle_arith_flag(result);
    a = result & 0xff;
}

void i8080::ANA(uint8_t *reg)
{
    uint16_t result = a & *reg;
    handle_arith_flag(result);
    cy = 0; 
    a = result & 0xff;
}

// the call routine works by first saving the return address, we increment pc by 2 as the address that is being called is 2 bytes
// save return address onto stack
// set pc to the address that is being called
void i8080::CALL()
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

void i8080::CMP(uint8_t *reg)
{
    uint16_t result = a - *reg;
    cy = (a < *reg);
    z = (result == 0);
    s = ((result & 0x80) != 0);
    p = parity(result & 0xff);
}

void i8080::DAA()
{
    uint8_t lsb = (a << 4) >> 4;
    if (lsb > 0x09 || ac == 1)
    {
        uint16_t result = lsb + 0x06;
        handle_arith_flag(result);
        a = a + 0x06;
    }
    uint8_t msb = (a >> 4);
    if (msb > 0x09 || cy == 1)
    {
        uint16_t result = msb + 0x06;
        handle_arith_flag(result);
        a = (a & 0x0f) | ((msb + 0x06) << 4);
    }
}

void i8080::DAD(uint8_t *reg1, uint8_t *reg2)
{
    uint32_t pair = (*reg1 << 8) | *reg2;
    uint32_t hl = (h << 8) | l;
    uint32_t result = hl + pair;
    cy = ((result & 0xffff0000) > 0); // check the upper 16 bits to see if there is an overflow beyond the 16 bits reg pair
    h = (result >> 8) & 0xff;
    l = result & 0xff;
}

void i8080::DCR(uint8_t *reg)
{
    uint16_t result = --*reg;
    z = ((result & 0xff) == 0);
    s = ((result & 0x80) != 0);
    p = parity(result & 0xff);
    *reg = result & 0xff;
}

void i8080::DCX(uint8_t *reg1, uint8_t *reg2)
{
    --*reg2;
    if (*reg2 == 0xff)
    {
        --*reg1;
    }
}

void i8080::INR(uint8_t *reg)
{
    uint16_t result = ++*reg;
    handle_without_carry(result);
    *reg = result & 0xff;
}

void i8080::INX(uint8_t *reg1, uint8_t *reg2)
{
    ++reg2;
    if (reg2 == 0)
    {
        ++reg1;
    }
}

void i8080::JMP()
{
    pc = (opcode[2] << 8) | opcode[1];
}

void i8080::LDA()
{
    uint16_t address = (opcode[2] << 8) | opcode[1];
    a = memory[address];
}

void i8080::LDAX(uint8_t *reg1, uint8_t *reg2)
{
    uint16_t address = (*reg1 << 8) | *reg2;
    a = memory[address];
}

void i8080::LHLD()
{
    uint16_t address = (opcode[2] << 8) | opcode[1];
    l = memory[address];
    h = memory[address + 1];
}

void i8080::LXI(uint8_t *reg1, uint8_t *reg2)
{
    *reg1 = opcode[2];
    *reg2 = opcode[1];
}

void i8080::ORA(uint8_t *reg)
{
    uint16_t result = a | *reg;
    cy = 0;
    z = (result == 0);
    s = ((result & 0x80) != 0);
    p = parity(result & 0xff);
    a = result & 0xff;
}

void i8080::POP(uint8_t *reg1, uint8_t *reg2)
{
    *reg2 = memory[sp];
    *reg1 = memory[sp + 1];
    sp += 2;
}

void i8080::POP_PSW()
{
    a = memory[sp + 1];
    uint8_t psw = memory[sp];
    s = (psw >> 7 & 0x1);
    z = (psw >> 6 & 0x1);
    ac = (psw >> 4 & 0x1);
    p = (psw >> 2 & 0x1);
    cy = (psw & 0x1);
    sp += 2;
}

void i8080::PUSH(uint8_t *reg1, uint8_t *reg2)
{
    sp -= 1;
    memory[sp] = *reg1;
    sp -= 1;
    memory[sp] = *reg2;
}

void i8080::PUSH_PSW()
{
    uint8_t psw = (s << 7) | (z << 6) | (ac << 4) | (p << 2) | (1 << 1) | cy;
    memory[sp - 1] = a;
    memory[sp - 2] = psw;
    sp -= 2;
}

void i8080::RAL()
{
    uint8_t high_bit = a >> 7;
    uint8_t temp = a;
    a = (temp << 1) | cy;
    cy = high_bit;
}

void i8080::RAR()
{
    uint8_t low_bit = a & 0x1;
    uint8_t temp = cy;
    cy = low_bit;
    a = (a >> 1) | (temp << 7);
}

void i8080::RET()
{
    // values are stored in opposite order on the stack, thus the first part of address will be lower on the stack
    pc = (memory[sp + 1] << 8) | memory[sp];
    sp += 2;
}

void i8080::RLC()
{
    uint8_t high_bit = a >> 7;
    cy = high_bit;
    a = (a << 1) | high_bit;
}

void i8080::RRC()
{
    uint8_t low_bit = a & 0x1;
    cy = low_bit;
    a = (a >> 1) | (low_bit << 7);
}

void i8080::SHLD()
{
    uint16_t address = (opcode[2] << 8) | opcode[1];
    memory[address] = l;
    memory[address + 1] = h;
}

void i8080::STA()
{
    uint16_t address = (opcode[2] << 8) | opcode[1];
    memory[address] = a;
}

void i8080::SBB(uint8_t *reg)
{
    *reg += cy;
    uint16_t result = a - *reg;
    handle_arith_flag(result);
    a = result & 0xff;
}

void i8080::SUB(uint8_t *reg)
{
    uint16_t result = a - *reg;
    handle_without_carry(result);
    cy = (!result > 0xff);
}

void i8080::XCHG()
{
    uint8_t temp_d = d;
    uint8_t temp_e = e;
    d = h;
    e = l;
    h = temp_d;
    l = temp_e;
}

void i8080::XRA(uint8_t *reg)
{
    uint16_t result = a ^ *reg;
    handle_without_carry(result);
    cy = 0;
    ac = 0;
    a = result;
}

void i8080::XTHL()
{
    uint8_t temp_l = l;
    uint8_t temp_h = h;
    l = memory[sp];
    memory[sp] = temp_l;
    sp += 1;
    memory[sp] = temp_h;
    h = memory[sp];
}

int i8080::emulate()
{
    instruction_count++; 
    // opcode is a pointer to the location in memory where the instruction is stored
    opcode = &memory[pc];
   // clock_count = instruction_cycles[opcode]; 

    pc += 1; 
    switch (*opcode)
    {
    // nop
    case 0x00:
        clock_count += 4;
        break;
    // lxi b, word
    case 0x01:
        LXI(&b, &c);
        pc += 2; 
        clock_count += 10;
        break;
    // stax, b
    case 0x02:
        uint16_t bc = (b << 8) | c;
        memory[bc] = a;
        clock_count += 7; 
        break;
    // inx, b
    case 0x03:
        INX(&b, &c);
        clock_count += 5; 
        break;
    // inr, b
    case 0x04:
        INR(&b);
        clock_count += 5; 
        break;
    // dcr, b
    case 0x05:
        DCR(&b);
        clock_count += 5;
        break;
    // mvi, b, d8
    case 0x06:
        b = opcode[1]; 
        pc += 1; 
        clock_count += 7;
        break;
    // rlc
    case 0x07:
        RLC();
        clock_count += 4;
        break;
    // nop
    case 0x08:
        clock_count += 4;
        break;
    // dad b
    case 0x09:
        DAD(&b, &c);
        clock_count += 10;
        break;
    // ldax b
    case 0xa:
        LDAX(&b, &c);
        clock_count += 7;
        break;
    // dcx b
    case 0xb:
        DCX(&b, &c);
        clock_count += 5;
        break;
    // inr c
    case 0xc:
        INR(&c);
        clock_count += 5;
        break;
    // dcr, c
    case 0xd:
        DCR(&c);
        clock_count += 5;
        break;
    // mvi, c, d8
    case 0xe:
        uint16_t value = opcode[1];
        c = value;
        clock_count += 7;
        break;
    // rrc
    case 0xf:
        RRC();
        clock_count += 4;
        break;
    // nop
    case 0x10:
        clock_count += 4;
        break;
    // lxi d, d16
    case 0x11:
        LXI(&d, &e);
        clock_count += 10;
        break;
    // stax d
    case 0x12:
        uint16_t address = (d << 8) | e;
        memory[address] = a;
        clock_count += 7;
        break;
    // inx d
    case 0x13:
        INX(&d, &e);
        clock_count += 5;
        break;
    // inr, d
    case 0x14:
        INR(&d);
        clock_count += 5;
        break;
    // dcr, d
    case 0x15:
        DCR(&d);
        clock_count += 5;
        break;
    // mvi d, d8
    case 0x16:
        d = opcode[1]; 
        pc += 2; 
        clock_count += 7;
        break;
    // ral
    case 0x17:
        RAL();
        clock_count += 4;
        break;
    // nop
    case 0x18:
        clock_count += 4;
        break;
    // dad d
    case 0x19:
        DAD(&d, &e);
        clock_count += 10;
        break;
    // ldax, d
    case 0x1a:
        LDAX(&d, &e);
        clock_count += 7;
        break;
    // dcx, d
    case 0x1b:
        DCX(&d, &e);
        clock_count += 5;
        break;
    // inr e
    case 0x1c:
        INR(&e);
        clock_count += 5;
        break;
    // dcr, e
    case 0x1d:
        DCR(&e);
        clock_count += 5;
        break;
    // mvi, e, d8
    case 0x1e:
        e = opcode[1]; 
        pc += 1; 
        clock_count += 7;
        break;
    // rar
    case 0x1f:
        RAR();
        clock_count += 4;
        break;
    // nop
    case 0x20:
        clock_count += 4;
        break;
    // lxi h, d16
    case 0x21:
        LXI(&h, &l);
        clock_count += 10;
        break;
    // shld a16
    case 0x22:
        SHLD();
        clock_count += 16;
        break;
    // inx, h
    case 0x23:
        INX(&h, &l);
        clock_count += 5;
        break;
    // inr h
    case 0x24:
        INR(&h);
        clock_count += 5;
        break;
    // dcr, h
    case 0x25:
        DCR(&h);
        clock_count += 5;
        break;
    // mvi h, d8
    case 0x26:
        h = opcode[1]; 
        pc += 1; 
        clock_count += 7;
        break;
    // daa
    case 0x27:
        DAA();
        clock_count += 4;
        break;
    // nop
    case 0x28:
        clock_count += 4;
        break;
    // dad h
    case 0x29:
        DAD(&h, &l);
        clock_count += 10;
        break;
    // lhld a16
    case 0x2a:
        LHLD();
        clock_count += 16;
        break;
    // dcx h
    case 0x2b:
        DCX(&h, &l);
        clock_count += 5;
        break;
    // inr l
    case 0x2c:
        INR(&l);
        clock_count += 5;
        break;
    // dcr l
    case 0x2d:
        DCR(&l);
        clock_count += 5;
    // mvi l, d8
    case 0x2e:
        l = opcode[1]; 
        pc += 1; 
        clock_count += 7;
        break;
    // cma
    case 0x2f:
        clock_count += 4;
        CMA();
    // nop
    case 0x30:
        clock_count += 4;
        break;
    // lxi sp, d16
    case 0x31:
        uint8_t second = opcode[1];
        uint8_t third = opcode[2];
        sp = (third << 8) | second;
        clock_count += 10;
        break;
    // sta a16
    case 0x32:
        STA();
        clock_count += 13;
        break;
    // inx sp
    case 0x33:
        ++sp;
        clock_count += 5;
    // inr m
    case 0x34:
        uint16_t address = (h << 8) | l;
        memory[address]++;
        handle_without_carry(memory[address]);
        clock_count += 10;
        break;
    // dcr m
    case 0x35:
        uint16_t address = (h << 8) | l;
        memory[address]--;
        handle_without_carry(memory[address]);
        clock_count += 10;
        break;
    // mvi m, d8
    case 0x36:
        uint16_t address = (h << 8) | l;
        memory[address] = opcode[1];
        clock_count += 10;
        break;
    // stc
    case 0x37:
        cy = 0x1;
        clock_count += 4;
    // nop
    case 0x38:
        clock_count += 4;
        break;
    // dad sp
    case 0x39:
        uint16_t pair = (h << 8) | l;
        uint32_t result = pair + sp;
        cy = ((result & 0xffff0000) > 0);
        h = (result >> 8) & 0xff;
        l = result * 0xff;
        clock_count += 10;
        break;
    // lda a16
    case 0x3a:
        LDA();
        clock_count += 16;
        break;
    // dcx sp
    case 0x3b:
        --sp;
        clock_count += 5;
        break;
    // inr a
    case 0x3c:
        INR(&a);
        clock_count += 5;
        break;
    // dcr a
    case 0x3d:
        DCR(&a);
        clock_count += 5;
        break;
    // mvi a, d8
    case 0x3e:
        a = opcode[1]; 
        pc += 1; 
        clock_count += 7;
        break;
    // cmc
    case 0x3f:
        cy = ~cy;
        clock_count += 4;
        break;
    // mov b, b
    case 0x40:
        clock_count += 5;
        break;
    // mov b, c
    case 0x41:
        b = c;
        clock_count += 5;
    // mov b d
    case 0x42:
        b = d;
        clock_count += 5;
    // mov b e
    case 0x43:
        b = e;
        clock_count += 5;
    // mov b h
    case 0x44:
        b = h;
        clock_count += 5;
    // mov b l
    case 0x45:
        b = l;
        clock_count += 5;
    // mov b m
    case 0x46:
        uint16_t address = (h << 8) | l;
        b = memory[address];
        clock_count += 4;
    // mov b a
    case 0x47:
        b = a;
        clock_count += 5;
    // mov c, b
    case 0x48:
        c = b;
        clock_count += 5;
    // mov c, c,
    case 0x49:
        clock_count += 5;
        break;
    // mov c, d
    case 0x4a:
        c = d;
        clock_count += 5;
    // mov c,e
    case 0x4b:
        c = e;
        clock_count += 5;
    // mov c, h
    case 0x4c:
        c = h;
        clock_count += 5;
    // mov c, l
    case 0x4d:
        c = l;
        clock_count += 5;
    // mov c, m
    case 0x4e:
        uint16_t address = (h << 8) | l;
        c = memory[address];
        clock_count += 7;
    // mov c, a
    case 0x4f:
        c = a;
        clock_count += 5;
    // mov d, b
    case 0x50:
        d = b;
        clock_count += 5;
    // mov d, c
    case 0x51:
        d = c;
        clock_count += 5;
    // mov d, d
    case 0x52:
        clock_count += 5;
        break;
    // mov d, e
    case 0x53:
        d = e;
        clock_count += 5;
    // mov d, h
    case 0x54:
        d = h;
        clock_count += 5;
    // mov d l
    case 0x55:
        d = l;
        clock_count += 5;
    // mov d, m
    case 0x56:
        uint16_t address = (h << 8) | l;
        d = memory[address];
        clock_count += 7;
    // mov d, a
    case 0x57:
        d = a;
        clock_count += 5;
    // mov e, b
    case 0x58:
        e = b;
        clock_count += 5;
    // mov e, c
    case 0x59:
        e = c;
        clock_count += 5;
    // mov e, d
    case 0x5a:
        e = d;
        clock_count += 5;
    // mov e, e
    case 0x5b:
        clock_count += 5;
        break;
    // mov e, h
    case 0x5c:
        e = h;
        clock_count += 5;
    // mov e l
    case 0x5d:
        e = l;
        clock_count += 5;
    // mov e, m
    case 0x5e:
        uint16_t address = (h << 8) | l;
        e = memory[address];
        clock_count += 7;
    // mov e, a
    case 0x5f:
        e = a;
        clock_count += 5;
    // mov h, b
    case 0x60:
        h = b;
        clock_count += 5;
    // mov h, c
    case 0x61:
        h = c;
        clock_count += 5;
    // mov h, d
    case 0x62:
        h = d;
        clock_count += 5;
    // mov h e
    case 0x63:
        h = e;
        clock_count += 5;
    // mov h, h
    case 0x64:
        clock_count += 5;
        break;
    // mov h, l
    case 0x65:
        h = l;
        clock_count += 5;
    // mov h, m
    case 0x66:
        uint16_t address = (h << 8) | l;
        h = memory[address];
        clock_count += 7;
    // mov h a
    case 0x67:
        h = a;
        clock_count += 5;
    // mov l b
    case 0x68:
        l = b;
        clock_count += 5;
    // mov l, c
    case 0x69:
        l = c;
        clock_count += 5;
    // mov l, d
    case 0x6a:
        l = d;
        clock_count += 5;
    // mov l e
    case 0x6b:
        l = e;
        clock_count += 5;
    // mov l , h
    case 0x6c:
        l = h;
        clock_count += 5;
    // mov l, l
    case 0x6d:
        clock_count += 5;
        break;
    // mov l, m
    case 0x6e:
        uint16_t address = (h << 8) | l;
        l = memory[address];
        clock_count += 7;
    // mov l a
    case 0x6f:
        l = a;
        clock_count += 5;
    // mov m, b
    case 0x70:
        uint16_t address = (h << 8) | l;
        memory[address] = b;
        clock_count += 7;
    // mov m, c
    case 0x71:
        uint16_t address = (h << 8) | l;
        memory[address] = c;
        clock_count += 7;
    // mov m, d
    case 0x72:
        uint16_t address = (h << 8) | l;
        memory[address] = d;
        clock_count += 7;
    // mov m, e
    case 0x73:
        uint16_t address = (h << 8) | l;
        memory[address] = e;
        clock_count += 7;
    // mov m, h
    case 0x74:
        uint16_t address = (h << 8) | l;
        memory[address] = h;
        clock_count += 7;
    // mov m, l
    case 0x75:
        uint16_t address = (h << 8) | l;
        memory[address] = l;
        clock_count += 7;
    // hlt
    case 0x76:
        clock_count += 7;
        exit(0);
    // move m, a
    case 0x77:
        uint16_t address = (h << 8) | l;
        memory[address] = a;
        clock_count += 7;
    // mov a, b
    case 0x78:
        a = b;
        clock_count += 5;
    // mov a, c
    case 0x79:
        a = c;
        clock_count += 5;
    // mov a, d
    case 0x7a:
        a = d;
        clock_count += 5;
    // mov a, e
    case 0x7b:
        a = e;
        clock_count += 5;
    // move a, h
    case 0x7c:
        a = h;
        clock_count += 5;
    // mov a, l
    case 0x7d:
        a = l;
        clock_count += 5;
    // mov a, m
    case 0x7e:
        uint16_t address = (h << 8) | l;
        a = memory[address];
        clock_count += 7;
    // mov a, e
    case 0x7f:
        clock_count += 5;
        break;
    // add b
    case 0x80:
        ADD(&b);
        clock_count += 4;
        break;
    // add c
    case 0x81:
        ADD(&c);
        clock_count += 4;
        break;
    // add d
    case 0x82:
        ADD(&d);
        clock_count += 4;
        break;
    // add e
    case 0x83:
        ADD(&e);
        clock_count += 4;
        break;
    // add h
    case 0x84:
        ADD(&h);
        clock_count += 4;
    // add l
    case 0x85:
        ADD(&l);
        clock_count += 4;
    // add m
    case 0x86:
        uint16_t address = (h << 8) | l;
        uint16_t result = a + memory[address];
        handle_arith_flag(result);
        a = result & 0xff;
        clock_count += 7;
    // add a
    case 0x87:
        ADD(&a);
        clock_count += 4;
    // adc b
    case 0x88:
        ADC(&b);
        clock_count += 4;
    // adc c
    case 0x89:
        ADC(&c);
        clock_count += 4;
    // adc d
    case 0x8a:
        ADC(&d);
        clock_count += 4;
    // adc e
    case 0x8b:
        ADC(&e);
        clock_count += 4;
    // adc h
    case 0x8c:
        ADC(&h);
        clock_count += 4;
    // adc l
    case 0x8d:
        ADC(&l);
        clock_count += 4;
    // adc m
    case 0x8e:
        uint16_t address = (h << 8) | l;
        uint16_t result = a + memory[address] + c;
        handle_arith_flag(result);
        a = result & 0xff;
        clock_count += 7;
    // adc a
    case 0x8f:
        ADC(&a);
        clock_count += 4;
    // sub b
    case 0x90:
        SUB(&b);
        clock_count += 4;
    // sub c
    case 0x91:
        SUB(&c);
        clock_count += 4;
    // sub d
    case 0x92:
        SUB(&d);
        clock_count += 4;
    // sub e
    case 0x93:
        SUB(&e);
        clock_count += 4;
    // sub h
    case 0x94:
        SUB(&h);
        clock_count += 4;
    // sub l
    case 0x95:
        SUB(&l);
        clock_count += 4;
    // sub m
    case 0x96:
        uint16_t address = (h << 8) | l;
        uint16_t result = a - memory[address];
        handle_without_carry(result);
        cy = (!result > 0xff);
        clock_count += 7;
    // sub a
    case 0x97:
        SUB(&a);
        clock_count += 4;
    // sbb b
    case 0x98:
        SBB(&b);
        clock_count += 4;
    // sbb c
    case 0x99:
        SBB(&c);
        clock_count += 4;
    // sbb d
    case 0x9a:
        SBB(&d);
        clock_count += 4;
    // sbb e
    case 0x9b:
        SBB(&e);
        clock_count += 4;
    // sbb h
    case 0x9c:
        SBB(&h);
        clock_count += 4;
    // sbb l
    case 0x9d:
        SBB(&l);
        clock_count += 4;
    // sbb m
    case 0x9e:
        uint16_t address = (h << 8) | l;
        memory[address] += cy;
        uint16_t result = a - memory[address];
        handle_arith_flag(result);
        a = result & 0xff;
        clock_count += 7;
    // sbb a
    case 0x9f:
        SBB(&a);
        clock_count += 4;
    // ana b
    case 0xa0:
        ANA(&b);
        clock_count += 4;
    // ana c
    case 0xa1:
        ANA(&c);
        clock_count += 4;
    // ana d
    case 0xa2:
        ANA(&d);
        clock_count += 4;
    // ana e
    case 0xa3:
        ANA(&e);
        clock_count += 4;
    // ana h
    case 0xa4:
        ANA(&h);
        clock_count += 4;
    // ana l
    case 0xa5:
        ANA(&l);
        clock_count += 4;
    // ana m
    case 0xa6:
        uint16_t address = (h << 8) | l;
        uint16_t result = a & memory[address];
        handle_arith_flag(result);
        a = result & 0xff;
        clock_count += 7;
    // ana a
    case 0xa7:
        ANA(&a);
        clock_count += 4;
    // xra b
    case 0xa8:
        XRA(&b);
        clock_count += 4;
    // xra c
    case 0xa9:
        XRA(&c);
        clock_count += 4;
    // xra d
    case 0xaa:
        XRA(&d);
        clock_count += 4;
    // xra e
    case 0xab:
        XRA(&e);
        clock_count += 4;
    // xra h
    case 0xac:
        XRA(&c);
        clock_count += 4;
    // xra l
    case 0xad:
        XRA(&d);
        clock_count += 4;
    // xra m
    case 0xae:
        uint16_t address = (h << 8) | l;
        uint16_t result = a ^ memory[address];
        handle_without_carry(result);
        cy = 0;
        ac = 0;
        a = result;
        clock_count += 7;
    // xra a
    case 0xaf:
        XRA(&a);
        clock_count += 4;
    // ora b
    case 0xb0:
        ORA(&b);
        clock_count += 4;
    // ora c
    case 0xb1:
        ORA(&c);
        clock_count += 4;
    // ora d
    case 0xb2:
        ORA(&d);
        clock_count += 4;
    // ora e
    case 0xb3:
        ORA(&e);
        clock_count += 4;
    // ora h
    case 0xb4:
        ORA(&h);
        clock_count += 4;
    // ora l
    case 0xb5:
        ORA(&l);
        clock_count += 4;
    // ora M
    case 0xb6:
        uint16_t address = (h << 8) | l;
        uint16_t result = a | memory[address];
        cy = 0;
        z = (result == 0);
        s = ((result & 0x80) != 0);
        p = parity(result & 0xff);
        a = result & 0xff;
        clock_count += 7;
    // ora a
    case 0xb7:
        ORA(&a);
        clock_count += 4;
    // cmp b
    case 0xb8:
        CMP(&b);
        clock_count += 4;
    // cmp c
    case 0xb9:
        CMP(&c);
        clock_count += 4;
    // cmp d
    case 0xba:
        CMP(&d);
        clock_count += 4;
    // cmp e
    case 0xbb:
        CMP(&e);
        clock_count += 4;
    // cmp h
    case 0xbc:
        CMP(&h);
        clock_count += 4;
    // cmp l
    case 0xbd:
        CMP(&l);
        clock_count += 4;
    // cmp m
    case 0xbe:
        uint16_t address = (h << 8) | l;
        uint16_t result = a - memory[address];
        cy = (a < memory[address]);
        z = (result == 0);
        s = ((result & 0x80) != 0);
        p = parity(result & 0xff);
        clock_count += 7;
    // cmp a
    case 0xbf:
        CMP(&a);
        clock_count += 4;
    // rnz
    case 0xc0:
        if (z != 0x0)
        {
            RET();
        }
        clock_count += 5;
    // pop b
    case 0xc1:
        POP(&b, &c);
        clock_count += 10;
    // jnz a16
    case 0xc2:
        if (z != 0x0)
        {
            JMP();
        }
        clock_count += 10;
    // jmp a16
    case 0xc3:
        JMP();
        clock_count += 10;
    // cnz a16
    case 0xc4:
        if (z != 0x0)
        {
            CALL();
        }
        clock_count += 11;
    // push b
    case 0xc5:
        PUSH(&b, &c);
        clock_count += 11;
    // adi d8
    case 0xc6:
        uint8_t byte = opcode[1];
        uint16_t result = a + byte;
        handle_arith_flag(result);
        a = result & 0xff;
        clock_count += 7;
    // rst 0
    case 0xc7:
        unsigned char temp[3];
        temp[1] = 0;
        temp[2] = 0;
        uint16_t return_address = pc + 2;
        memory[sp - 1] = (return_address >> 8) & 0xff;
        memory[sp - 2] = return_address & 0xff;
        sp = sp - 2;
        pc = temp[2] << 8 | temp[1];
        clock_count += 11;
    // rz
    case 0xc8:
        if (z == 0x0)
        {
            RET();
        }
        clock_count += 5;
    // ret
    case 0xc9:
        RET();
        clock_count += 10;
    // jz a16
    case 0xca:
        if (z == 0x0)
        {
            JMP();
        }
        clock_count += 10;
    // jmp a16 illegal opcode
    case 0xcb:
        clock_count += 10;
        break;
    // cz a16
    case 0xcc:
        if (z == 0x0)
        {
            CALL();
        }
        clock_count += 11;
    // call a16
    case 0xcd:
        CALL();
        clock_count += 11;
    // aci d8
    case 0xce:
        uint8_t byte = opcode[1];
        uint16_t result = a + byte + cy;
        handle_arith_flag(result);
        a = result & 0xff;
        clock_count += 7;
    // rst 1
    case 0xcf:
        unsigned char temp[3];
        temp[1] = 8;
        temp[2] = 0;
        uint16_t return_address = pc + 2;
        memory[sp - 1] = (return_address >> 8) & 0xff;
        memory[sp - 2] = return_address & 0xff;
        sp = sp - 2;
        pc = temp[2] << 8 | temp[1];
        clock_count += 11;
    // rnc
    case 0xd0:
        if (cy != 0x0)
        {
            RET();
        }
    // pop d
    case 0xd1:
        POP(&d, &e);
    // jnc a16
    case 0xd2:
        if (cy != 0x0)
        {
            JMP();
        }
    // out d8
    case 0xd3:
        break;
    // cnc a16
    case 0xd4:
        if (cy != 0x0)
        {
            CALL();
        }
    // push d
    case 0xd5:
        PUSH(&d, &e);
    // sui d8
    case 0xd6:
        uint8_t byte = opcode[1];
        uint16_t result = a - byte;
        handle_arith_flag(result);
        a = result & 0xff;
    // rst 2
    case 0xd7:
        unsigned char temp[3];
        temp[1] = 16;
        temp[2] = 0;
        uint16_t return_address = pc + 2;
        memory[sp - 1] = (return_address >> 8) & 0xff;
        memory[sp - 2] = return_address & 0xff;
        sp = sp - 2;
        pc = temp[2] << 8 | temp[1];
    // rc
    case 0xd8:
        if (cy != 0x0)
        {
            RET();
        }
    // ret 1 illegal opcode
    case 0xd9:
        break;
    // jc a16
    case 0xda:
        if (cy != 0x0)
        {
            JMP();
        }
    // in d8
    case 0xdb:
        break;
    // cc a16
    case 0xdc:
        if (cy != 0x0)
        {
            CALL();
        }
    // call a16 illegal opcode
    case 0xdd:
        break;
    // sbi d8
    case 0xde:
        uint8_t byte = opcode[1];
        uint16_t result = a - byte - cy;
        handle_arith_flag(result);
        a = result & 0xff;
    // rst 3
    case 0xdf:
        unsigned char temp[3];
        temp[1] = 24;
        temp[2] = 0;
        uint16_t return_address = pc + 2;
        memory[sp - 1] = (return_address >> 8) & 0xff;
        memory[sp - 2] = return_address & 0xff;
        sp = sp - 2;
        pc = temp[2] << 8 | temp[1];
    // rpo
    case 0xe0:
        if (p == 0)
        {
            RET();
        }
    // pop h
    case 0xe1:
        POP(&h, &l);
    // JPO a16
    case 0xe2:
        if (p == 0)
        {
            JMP();
        }
    // xthl illegal opcode
    case 0xe3:
        break;
    // cpo a16
    case 0xe4:
        if (p == 0)
        {
            CALL();
        } 
        else 
        {
            pc += 2; 
        }
    // push h
    case 0xe5:
        PUSH(&h, &l);
    // ani d8
    case 0xe6:
        uint8_t byte = opcode[1];
        uint16_t result = a & byte;
        handle_arith_flag(result);
        a = result & 0xff;
    // rst 4
    case 0xe7:
        unsigned char temp[3];
        temp[1] = 32;
        temp[2] = 0;
        uint16_t return_address = pc + 2;
        memory[sp - 1] = (return_address >> 8) & 0xff;
        memory[sp - 2] = return_address & 0xff;
        sp = sp - 2;
        pc = temp[2] << 8 | temp[1];
    // rpe
    case 0xe8:
        if (p == 0x1)
        {
            RET();
        }
    // pchl
    case 0xe9:
        uint16_t address = (h << 8) | l;
        pc = address;
    // jpe a16
    case 0xea:
        if (p == 0x1)
        {
            JMP();
        }
    // xchg
    case 0xeb:
        XCHG();
    // cpe a16
    case 0xec:
        if (p == 0x1)
        {
            CALL();
        }
    // call a16 illegal opcode
    case 0xed:
        break;
    // xri d8
    case 0xee:
        uint8_t byte = opcode[1];
        uint16_t result = a ^ byte;
        handle_without_carry(result);
        cy = 0;
        ac = 0;
        a = result;
    // rst 5
    case 0xef:
        unsigned char temp[3];
        temp[1] = 40;
        temp[2] = 0;
        uint16_t return_address = pc + 2;
        memory[sp - 1] = (return_address >> 8) & 0xff;
        memory[sp - 2] = return_address & 0xff;
        sp = sp - 2;
        pc = temp[2] << 8 | temp[1];
    // rp
    case 0xf0:
        if (s == 0)
        {
            RET();
        }
    // pop psw
    case 0xf1:
        POP_PSW();
    // jp a16
    case 0xf2:
        if (s == 0)
        {
            JMP();
        }
    // di
    case 0xf3:
        break;
    // cp a16
    case 0xf4:
        if (s == 0)
        {
            CALL();
        }
    // push psw
    case 0xf5:
        PUSH_PSW();
    // ori d8
    case 0xf6:
        uint8_t byte = opcode[1];
        uint16_t result = a | byte;
        handle_without_carry(result);
        a = result & 0xff;
    // rst 6
    case 0xf7:
        unsigned char temp[3];
        temp[1] = 48;
        temp[2] = 0;
        uint16_t return_address = pc + 2;
        memory[sp - 1] = (return_address >> 8) & 0xff;
        memory[sp - 2] = return_address & 0xff;
        sp = sp - 2;
        pc = temp[2] << 8 | temp[1];
    // rm
    case 0xf8:
        if (s == 1)
        {
            RET();
        }
    // sphl
    case 0xf9:
        uint16_t address = (h << 8) | l;
        sp = address;
    // jm a16
    case 0xfa:
        if (s == 1)
        {
            JMP();
        }
    // ei
    case 0xfb:
        break;
    // cm a16
    case 0xfc:
        if (s == 1)
        {
            CALL();
        }
    // call a16 illegal opcode
    case 0xfd:
        break;
    // cpi d8
    case 0xfe:
        uint8_t byte = opcode[1];
        uint16_t result = a - byte;
        cy = (a < byte);
        z = (result == 0);
        s = ((result & 0x80) != 0);
        p = parity(result & 0xff);
    // rst 7 
    case 0xff: 
        unsigned char temp[3];
        temp[1] = 56;
        temp[2] = 0;
        uint16_t return_address = pc + 2;
        memory[sp - 1] = (return_address >> 8) & 0xff;
        memory[sp - 2] =   return_address & 0xff;
        sp = sp - 2;
        pc = temp[2] << 8 | temp[1];
        break; 

    default: 
        unimplemented_instruction(); 
        break; 
    }
    
};

