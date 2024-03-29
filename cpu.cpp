#include "cpu.hpp"
#include <algorithm>


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

uint8_t i8080::read_byte(uint16_t address)
{
    return memory[address]; 
}

uint16_t i8080::read_word(uint16_t address)
{
    return read_byte(address) | (read_byte(address + 1) << 8); 
}

void i8080::write_byte(uint16_t address, uint8_t val)
{
    if (address >= 0x2000 && address <= 0x4000)
    {
        memory[address] = val; 
    }
}

void i8080::write_word(uint16_t address, uint16_t val)
{
    write_byte(address, val & 0xff); 
    write_byte(address + 1, val >> 8); 
}

void i8080::generate_interrupt(uint8_t id)
{
    sp -= 1;
    memory[sp] = (pc & 0xff00) << 8;
    sp -= 1;
    memory[sp] = pc & 0xff;
    pc = 8 * id; 
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
    sp -= 2; 
    write_word(sp, return_address); 
    pc = (opcode[2] << 8) | opcode[1];
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
    a = read_byte(address); 
}

void i8080::LDAX(uint8_t *reg1, uint8_t *reg2)
{
    uint16_t address = (*reg1 << 8) | *reg2;
    a = read_byte(address); 
}

void i8080::LHLD()
{
    uint16_t address = (opcode[2] << 8) | opcode[1];
    l = read_byte(address); 
    h = read_byte(address + 1); 
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
    *reg2 = read_byte(sp); 
    *reg1 = read_byte(sp + 1); 
    sp += 2;
}

void i8080::POP_PSW()
{
    a = read_byte(sp + 1); 
    uint8_t psw = read_byte(sp); 
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
    write_byte(sp, *reg1); 
    sp -= 1;
    write_byte(sp, *reg2);
}

void i8080::PUSH_PSW()
{
    uint8_t psw = (s << 7) | (z << 6) | (ac << 4) | (p << 2) | (1 << 1) | cy;
    sp -= 2; 
    write_byte(sp, a); 
    write_byte(sp, psw); 
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
    pc = (read_byte(sp + 1) << 8) | read_byte(sp);
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
    write_byte(address, l); 
    write_byte(address + 1, h); 
}

void i8080::STA()
{
    uint16_t address = (opcode[2] << 8) | opcode[1];
    write_byte(address, a); 
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
        clock_count += 4; break;
    // lxi b, word
    case 0x01:
        LXI(&b, &c); pc += 2; clock_count += 10; break;
    // stax, b
    case 0x02:
        uint16_t bc = (b << 8) | c; write_byte(bc, a); clock_count += 7; break;
    // inx, b
    case 0x03:
        INX(&b, &c); clock_count += 5; break;
    // inr, b
    case 0x04:
        INR(&b); clock_count += 5; break;
    // dcr, b
    case 0x05:
        DCR(&b); clock_count += 5; break;
    // mvi, b, d8
    case 0x06:
        b = opcode[1]; pc += 1; clock_count += 7; break;
    // rlc
    case 0x07:
        RLC(); clock_count += 4; break;
    // nop
    case 0x08:
        clock_count += 4; break;
    // dad b
    case 0x09:
        DAD(&b, &c); clock_count += 10; break;
    // ldax b
    case 0xa:
        LDAX(&b, &c); clock_count += 7; break;
    // dcx b
    case 0xb:
        DCX(&b, &c); clock_count += 5; break;
    // inr c
    case 0xc:
        INR(&c); clock_count += 5; break;
    // dcr, c
    case 0xd:
        DCR(&c); clock_count += 5; break;
    // mvi, c, d8
    case 0xe:
        uint16_t value = opcode[1]; c = value; clock_count += 7; break;
    // rrc
    case 0xf:
        RRC(); clock_count += 4; break;
    // nop
    case 0x10:
        clock_count += 4; break;
    // lxi d, d16
    case 0x11:
        LXI(&d, &e); clock_count += 10; break;
    // stax d
    case 0x12:
        uint16_t address = (d << 8) | e; write_byte(address, a); clock_count += 7; break;
    // inx d
    case 0x13:
        INX(&d, &e); clock_count += 5; break;
    // inr, d
    case 0x14:
        INR(&d); clock_count += 5; break;
    // dcr, d
    case 0x15:
        DCR(&d); clock_count += 5; break;
    // mvi d, d8
    case 0x16:
        d = opcode[1]; pc += 2; clock_count += 7; break;
    // ral
    case 0x17:
        RAL(); clock_count += 4; break;
    // nop
    case 0x18:
        clock_count += 4; break;
    // dad d
    case 0x19:
        DAD(&d, &e); clock_count += 10; break;
    // ldax, d
    case 0x1a:
        LDAX(&d, &e); clock_count += 7; break;
    // dcx, d
    case 0x1b:
        DCX(&d, &e); clock_count += 5; break;
    // inr e
    case 0x1c:
        INR(&e); clock_count += 5; break;
    // dcr, e
    case 0x1d:
        DCR(&e); clock_count += 5; break;
    // mvi, e, d8
    case 0x1e:
        e = opcode[1]; pc += 1; clock_count += 7; break;
    // rar
    case 0x1f:
        RAR(); clock_count += 4; break;
    // nop
    case 0x20:
        clock_count += 4; break;
    // lxi h, d16
    case 0x21:
        LXI(&h, &l); clock_count += 10; break;
    // shld a16
    case 0x22:
        SHLD(); clock_count += 16; break;
    // inx, h
    case 0x23:
        INX(&h, &l); clock_count += 5; break;
    // inr h
    case 0x24:
        INR(&h); clock_count += 5; break;
    // dcr, h
    case 0x25:
        DCR(&h); clock_count += 5; break;
    // mvi h, d8
    case 0x26:
        h = opcode[1]; pc += 1; clock_count += 7; break;
    // daa
    case 0x27:
        DAA(); clock_count += 4; break;
    // nop
    case 0x28:
        clock_count += 4; break;
    // dad h
    case 0x29:
        DAD(&h, &l); clock_count += 10; break;
    // lhld a16
    case 0x2a:
        LHLD(); clock_count += 16; break;
    // dcx h
    case 0x2b:
        DCX(&h, &l); clock_count += 5; break;
    // inr l
    case 0x2c:
        INR(&l); clock_count += 5; break;
    // dcr l
    case 0x2d:
        DCR(&l); clock_count += 5;
    // mvi l, d8
    case 0x2e:
        l = opcode[1]; pc += 1; clock_count += 7; break;
    // cma
    case 0x2f:
        clock_count += 4; CMA();
    // nop
    case 0x30:
        clock_count += 4; break;
    // lxi sp, d16
    case 0x31:
        uint8_t second = opcode[1]; uint8_t third = opcode[2]; sp = (third << 8) | second; clock_count += 10; break;
    // sta a16
    case 0x32:
        STA(); clock_count += 13; break;
    // inx sp
    case 0x33:
        ++sp; clock_count += 5;
    // inr m
    case 0x34:
        uint16_t address = (h << 8) | l; memory[address]++; handle_without_carry(memory[address]); clock_count += 10; break;
    // dcr m
    case 0x35:
        uint16_t address = (h << 8) | l; memory[address]--; handle_without_carry(memory[address]); clock_count += 10; break;
    // mvi m, d8
    case 0x36:
        uint16_t address = (h << 8) | l; write_byte(address, opcode[1]); clock_count += 10; break;
    // stc
    case 0x37:
        cy = 0x1; clock_count += 4;
    // nop
    case 0x38:
        clock_count += 4; break;
    // dad sp
    case 0x39:
        uint16_t pair = (h << 8) | l; uint32_t result = pair + sp; cy = ((result & 0xffff0000) > 0); h = (result >> 8) & 0xff; l = result * 0xff; clock_count += 10; break;
    // lda a16
    case 0x3a:
        LDA(); clock_count += 16; break;
    // dcx sp
    case 0x3b:
        --sp; clock_count += 5; break;
    // inr a
    case 0x3c:
        INR(&a); clock_count += 5; break;
    // dcr a
    case 0x3d:
        DCR(&a); clock_count += 5; break;
    // mvi a, d8
    case 0x3e:
        a = opcode[1]; pc += 1; clock_count += 7; break;
    // cmc
    case 0x3f:
        cy = ~cy; clock_count += 4; break;
    // mov b, b
    case 0x40:
        clock_count += 5; break;
    // mov b, c
    case 0x41:
        b = c; clock_count += 5; break; 
    // mov b d
    case 0x42:
        b = d; clock_count += 5; break; 
    // mov b e
    case 0x43:
        b = e; clock_count += 5; break; 
    // mov b h
    case 0x44:
        b = h; clock_count += 5; break; 
    // mov b l
    case 0x45:
        b = l; clock_count += 5; break; 
    // mov b m
    case 0x46:
        uint16_t address = (h << 8) | l; b = read_byte(address); clock_count += 4; break; 
    // mov b a
    case 0x47:
        b = a; clock_count += 5; break; 
    // mov c, b
    case 0x48:
        c = b; clock_count += 5; break; 
    // mov c, c,
    case 0x49:
        clock_count += 5; break;
    // mov c, d
    case 0x4a:
        c = d; clock_count += 5; break; 
    // mov c,e
    case 0x4b:
        c = e; clock_count += 5; break; 
    // mov c, h
    case 0x4c:
        c = h; clock_count += 5; break; 
    // mov c, l
    case 0x4d:
        c = l; clock_count += 5; break; 
    // mov c, m
    case 0x4e:
        uint16_t address = (h << 8) | l; c = read_byte(address); clock_count += 7; break;
    // mov c, a
    case 0x4f:
        c = a; clock_count += 5; break;
    // mov d, b
    case 0x50:
        d = b; clock_count += 5; break;
    // mov d, c
    case 0x51:
        d = c; clock_count += 5; break;
    // mov d, d
    case 0x52:
        clock_count += 5; break;
    // mov d, e
    case 0x53:
        d = e; clock_count += 5; break;
    // mov d, h
    case 0x54:
        d = h; clock_count += 5; break;
    // mov d l
    case 0x55:
        d = l; clock_count += 5; break;
    // mov d, m
    case 0x56:
        uint16_t address = (h << 8) | l; d = read_byte(address); clock_count += 7; break;
    // mov d, a
    case 0x57:
        d = a; clock_count += 5; break;
    // mov e, b
    case 0x58:
        e = b; clock_count += 5; break;
    // mov e, c
    case 0x59:
        e = c; clock_count += 5; break;
    // mov e, d
    case 0x5a:
        e = d; clock_count += 5; break;
    // mov e, e
    case 0x5b:
        clock_count += 5; break;
    // mov e, h
    case 0x5c:
        e = h; clock_count += 5; break;
    // mov e l
    case 0x5d:
        e = l; clock_count += 5; break;
    // mov e, m
    case 0x5e:
        uint16_t address = (h << 8) | l; e = read_byte(address); clock_count += 7; break;
    // mov e, a
    case 0x5f:
        e = a; clock_count += 5; break;
    // mov h, b
    case 0x60:
        h = b; clock_count += 5; break;
    // mov h, c
    case 0x61:
        h = c; clock_count += 5; break;
    // mov h, d
    case 0x62:
        h = d; clock_count += 5; break;
    // mov h e
    case 0x63:
        h = e; clock_count += 5; break;
    // mov h, h
    case 0x64:
        clock_count += 5; break;
    // mov h, l
    case 0x65:
        h = l; clock_count += 5; break;
    // mov h, m
    case 0x66:
        uint16_t address = (h << 8) | l; h = read_byte(address); clock_count += 7; break;
    // mov h a
    case 0x67:
        h = a; clock_count += 5; break;
    // mov l b
    case 0x68:
        l = b;  clock_count += 5; break;
    // mov l, c
    case 0x69:
        l = c; clock_count += 5; break;
    // mov l, d
    case 0x6a:
        l = d; clock_count += 5; break;
    // mov l e
    case 0x6b:
        l = e; clock_count += 5; break;
    // mov l , h
    case 0x6c:
        l = h; clock_count += 5; break;
    // mov l, l
    case 0x6d:
        clock_count += 5; break;
    // mov l, m
    case 0x6e:
        uint16_t address = (h << 8) | l; l = read_byte(address); clock_count += 7; break;
    // mov l a
    case 0x6f:
        l = a; clock_count += 5; break;
    // mov m, b
    case 0x70:
        uint16_t address = (h << 8) | l; write_byte(address, b); clock_count += 7; break;
    // mov m, c
    case 0x71:
        uint16_t address = (h << 8) | l; write_byte(address, c); clock_count += 7; break;
    // mov m, d
    case 0x72:
        uint16_t address = (h << 8) | l; write_byte(address, d); clock_count += 7; break;
    // mov m, e
    case 0x73:
        uint16_t address = (h << 8) | l; write_byte(address, e); clock_count += 7; break;
    // mov m, h
    case 0x74:
        uint16_t address = (h << 8) | l; write_byte(address, h); clock_count += 7; break;
    // mov m, l
    case 0x75:
        uint16_t address = (h << 8) | l; write_byte(address, l); clock_count += 7; break;
    // hlt
    case 0x76:
        clock_count += 7; exit(0); 
    // move m, a
    case 0x77:
        uint16_t address = (h << 8) | l; write_byte(address, a); clock_count += 7; break;
    // mov a, b
    case 0x78:
        a = b; clock_count += 5; break;
    // mov a, c
    case 0x79:
        a = c; clock_count += 5; break;
    // mov a, d
    case 0x7a:
        a = d; clock_count += 5; break;
    // mov a, e
    case 0x7b:
        a = e; clock_count += 5; break;
    // move a, h
    case 0x7c:
        a = h; clock_count += 5; break;
    // mov a, l
    case 0x7d:
        a = l; clock_count += 5; break;
    // mov a, m
    case 0x7e:
        uint16_t address = (h << 8) | l; a = read_byte(address); clock_count += 7; break;
    // mov a, e
    case 0x7f:
        clock_count += 5; break;
    // add b
    case 0x80:
        ADD(&b); clock_count += 4; break;
    // add c
    case 0x81:
        ADD(&c); clock_count += 4; break;
    // add d
    case 0x82:
        ADD(&d); clock_count += 4; break;
    // add e
    case 0x83:
        ADD(&e); clock_count += 4; break;
    // add h
    case 0x84:
        ADD(&h); clock_count += 4; break;
    // add l
    case 0x85:
        ADD(&l); clock_count += 4; break;
    // add m
    case 0x86:
        uint16_t address = (h << 8) | l; uint16_t result = a + memory[address]; handle_arith_flag(result); a = result & 0xff; clock_count += 7; break;
    // add a
    case 0x87:
        ADD(&a); clock_count += 4; break;
    // adc b
    case 0x88:
        ADC(&b); clock_count += 4; break;
    // adc c
    case 0x89:
        ADC(&c); clock_count += 4; break;
    // adc d
    case 0x8a:
        ADC(&d); clock_count += 4; break;
    // adc e
    case 0x8b:
        ADC(&e); clock_count += 4; break;
    // adc h
    case 0x8c:
        ADC(&h); clock_count += 4; break;
    // adc l
    case 0x8d:
        ADC(&l); clock_count += 4; break;
    // adc m
    case 0x8e:
        uint16_t address = (h << 8) | l; uint16_t result = a + memory[address] + c; handle_arith_flag(result); a = result & 0xff; clock_count += 7; break;
    // adc a
    case 0x8f:
        ADC(&a); clock_count += 4; break;
    // sub b
    case 0x90:
        SUB(&b); clock_count += 4; break;
    // sub c
    case 0x91:
        SUB(&c); clock_count += 4; break;
    // sub d
    case 0x92:
        SUB(&d); clock_count += 4; break;
    // sub e
    case 0x93:
        SUB(&e); clock_count += 4; break;
    // sub h
    case 0x94:
        SUB(&h); clock_count += 4; break;
    // sub l
    case 0x95:
        SUB(&l); clock_count += 4; break;
    // sub m
    case 0x96:
        uint16_t address = (h << 8) | l; uint16_t result = a - memory[address]; handle_without_carry(result); cy = (!result > 0xff); clock_count += 7; break;
    // sub a
    case 0x97:
        SUB(&a); clock_count += 4; break;
    // sbb b
    case 0x98:
        SBB(&b); clock_count += 4; break;
    // sbb c
    case 0x99:
        SBB(&c); clock_count += 4; break;
    // sbb d
    case 0x9a:
        SBB(&d); clock_count += 4; break;
    // sbb e
    case 0x9b:
        SBB(&e); clock_count += 4; break;
    // sbb h
    case 0x9c:
        SBB(&h); clock_count += 4; break;
    // sbb l
    case 0x9d:
        SBB(&l); clock_count += 4; break;
    // sbb m
    case 0x9e:
        uint16_t address = (h << 8) | l; memory[address] += cy; uint16_t result = a - memory[address]; handle_arith_flag(result); a = result & 0xff; clock_count += 7; break;
    // sbb a
    case 0x9f:
        SBB(&a); clock_count += 4; break;
    // ana b
    case 0xa0:
        ANA(&b); clock_count += 4; break;
    // ana c
    case 0xa1:
        ANA(&c); clock_count += 4; break;
    // ana d
    case 0xa2:
        ANA(&d); clock_count += 4; break;
    // ana e
    case 0xa3:
        ANA(&e); clock_count += 4; break;
    // ana h
    case 0xa4:
        ANA(&h); clock_count += 4; break;
    // ana l
    case 0xa5:
        ANA(&l); clock_count += 4; break;
    // ana m
    case 0xa6:
        uint16_t address = (h << 8) | l; uint16_t result = a & memory[address]; handle_arith_flag(result); a = result & 0xff; clock_count += 7; break;
    // ana a
    case 0xa7:
        ANA(&a); clock_count += 4; break;
    // xra b
    case 0xa8:
        XRA(&b); clock_count += 4; break;
    // xra c
    case 0xa9:
        XRA(&c); clock_count += 4; break;
    // xra d
    case 0xaa:
        XRA(&d); clock_count += 4; break;
    // xra e
    case 0xab:
        XRA(&e); clock_count += 4; break;
    // xra h
    case 0xac:
        XRA(&c); clock_count += 4; break;
    // xra l
    case 0xad:
        XRA(&d); clock_count += 4; break;
    // xra m
    case 0xae:
        uint16_t address = (h << 8) | l; uint16_t result = a ^ memory[address]; handle_without_carry(result); cy = 0; ac = 0; a = result; clock_count += 7; break;
    // xra a
    case 0xaf:
        XRA(&a); clock_count += 4; break;
    // ora b
    case 0xb0:
        ORA(&b); clock_count += 4; break;
    // ora c
    case 0xb1:
        ORA(&c); clock_count += 4; break;
    // ora d
    case 0xb2:
        ORA(&d); clock_count += 4; break;
    // ora e
    case 0xb3:
        ORA(&e); clock_count += 4; break;
    // ora h
    case 0xb4:
        ORA(&h); clock_count += 4; break;
    // ora l
    case 0xb5:
        ORA(&l); clock_count += 4; break;
    // ora M
    case 0xb6:
        uint16_t address = (h << 8) | l; uint16_t result = a | memory[address]; cy = 0; z = (result == 0); s = ((result & 0x80) != 0); p = parity(result & 0xff); a = result & 0xff; clock_count += 7; break;
    // ora a
    case 0xb7:
        ORA(&a); clock_count += 4; break;
    // cmp b
    case 0xb8:
        CMP(&b); clock_count += 4; break;
    // cmp c
    case 0xb9:
        CMP(&c); clock_count += 4; break;
    // cmp d
    case 0xba:
        CMP(&d); clock_count += 4; break;
    // cmp e
    case 0xbb:
        CMP(&e); clock_count += 4; break;
    // cmp h
    case 0xbc:
        CMP(&h); clock_count += 4; break;
    // cmp l
    case 0xbd:
        CMP(&l); clock_count += 4; break;
    // cmp m
    case 0xbe:
        uint16_t address = (h << 8) | l; uint16_t result = a - memory[address]; cy = (a < memory[address]); z = (result == 0); s = ((result & 0x80) != 0); p = parity(result & 0xff); clock_count += 7; break;
    // cmp a
    case 0xbf:
        CMP(&a); clock_count += 4; break;
    // rnz
    case 0xc0:
        if (z != 0x0) RET(); clock_count += 5; break; 
    // pop b
    case 0xc1:
        POP(&b, &c); clock_count += 10; break; 
    // jnz a16
    case 0xc2:
        if (z != 0x0) JMP(); clock_count += 10; break; 
    // jmp a16
    case 0xc3:
        JMP(); clock_count += 10; break; 
    // cnz a16
    case 0xc4:
        if (z != 0x0) CALL(); clock_count += 11; break; 
    // push b
    case 0xc5:
        PUSH(&b, &c); clock_count += 11; break; 
    // adi d8
    case 0xc6:
        uint8_t byte = opcode[1]; uint16_t result = a + byte; handle_arith_flag(result); a = result & 0xff; clock_count += 7; break; 
    // rst 0
    case 0xc7:
        unsigned char temp[3]; temp[1] = 0; temp[2] = 0; uint16_t return_address = pc + 2; write_byte(sp - 1, (return_address >> 8) & 0xff); write_byte(sp - 2, return_address & 0xff); sp = sp - 2; pc = temp[2] << 8 | temp[1]; clock_count += 11; break; 
    // rz
    case 0xc8:
        if (z == 0x0) RET(); clock_count += 5; break; 
    // ret
    case 0xc9:
        RET(); clock_count += 10; break; 
    // jz a16
    case 0xca:
        if (z == 0x0) JMP(); clock_count += 10; break; 
    // jmp a16 illegal opcode
    case 0xcb:
        clock_count += 10; break;
    // cz a16
    case 0xcc:
        if (z == 0x0) CALL(); clock_count += 11; break; 
    // call a16
    case 0xcd:
        CALL(); clock_count += 11; break; 
    // aci d8
    case 0xce:
        uint8_t byte = opcode[1]; uint16_t result = a + byte + cy; handle_arith_flag(result); a = result & 0xff; clock_count += 7; break; 
    // rst 1
    case 0xcf:
        unsigned char temp[3]; temp[1] = 8; temp[2] = 0; uint16_t return_address = pc + 2; write_byte(sp - 1, (return_address >> 8) & 0xff); write_byte(sp - 2, return_address & 0xff); sp = sp - 2; pc = temp[2] << 8 | temp[1]; clock_count += 11; break; 
    // rnc
    case 0xd0:
        if (cy != 0x0) RET(); clock_count += 5; break;
    // pop d
    case 0xd1:
        POP(&d, &e); clock_count += 10; break;
    // jnc a16
    case 0xd2:
        if (cy != 0x0) JMP(); clock_count += 10; break;
    // out d8
    case 0xd3:
        clock_count += 10; break;
    // cnc a16
    case 0xd4:
        if (cy != 0x0) CALL(); clock_count += 11; break;
    // push d
    case 0xd5:
        PUSH(&d, &e); clock_count += 11; break; 
    // sui d8
    case 0xd6:
        uint8_t byte = opcode[1]; uint16_t result = a - byte; handle_arith_flag(result); a = result & 0xff; clock_count += 7; break; 
    // rst 2
    case 0xd7:
        unsigned char temp[3]; temp[1] = 16; temp[2] = 0; uint16_t return_address = pc + 2; write_byte(sp - 1, (return_address >> 8) & 0xff); write_byte(sp - 2, return_address & 0xff); sp = sp - 2; pc = temp[2] << 8 | temp[1]; clock_count += 11; break; 
    // rc
    case 0xd8:
        if (cy != 0x0) RET(); clock_count += 5; break; 
    // ret 1 illegal opcode
    case 0xd9:
        clock_count += 10; break;
    // jc a16
    case 0xda:
        if (cy != 0x0) JMP(); clock_count += 10; break; 
    // in d8
    case 0xdb:
        clock_count += 10; break;
    // cc a16
    case 0xdc:
        if (cy != 0x0) CALL(); clock_count += 11; break;
    // call a16 illegal opcode
    case 0xdd:
        clock_count += 11; break;
    // sbi d8
    case 0xde:
        uint8_t byte = opcode[1]; uint16_t result = a - byte - cy; handle_arith_flag(result); a = result & 0xff; clock_count += 7; break; 
    // rst 3
    case 0xdf:
        unsigned char temp[3]; temp[1] = 24; temp[2] = 0; uint16_t return_address = pc + 2; write_byte(sp - 1, (return_address >> 8) & 0xff); write_byte(sp - 2, return_address & 0xff); sp = sp - 2;pc = temp[2] << 8 | temp[1]; clock_count += 11; break; 
    // rpo
    case 0xe0:
        if (p == 0) RET(); clock_count += 5; break; 
    // pop h
    case 0xe1:
        POP(&h, &l); clock_count += 10; break; 
    // JPO a16
    case 0xe2:
        if (p == 0) JMP(); clock_count += 10; break; 
    // xthl illegal opcode
    case 0xe3:
        clock_count += 18; break;
    // cpo a16
    case 0xe4:
        if (p == 0) CALL(); else pc += 2; clock_count += 11; break; 
    // push h
    case 0xe5:
        PUSH(&h, &l); clock_count += 11; break; 
    // ani d8
    case 0xe6:
        uint8_t byte = opcode[1]; uint16_t result = a & byte; handle_arith_flag(result); a = result & 0xff; clock_count += 7; break; 
    // rst 4
    case 0xe7:
        unsigned char temp[3]; temp[1] = 32; temp[2] = 0; uint16_t return_address = pc + 2; write_byte(sp - 1, (return_address >> 8) & 0xff); write_byte(sp - 2, return_address & 0xff); sp = sp - 2; pc = temp[2] << 8 | temp[1]; clock_count += 11; break; 
    // rpe
    case 0xe8:
        if (p == 0x1) RET(); clock_count += 5; break;
    // pchl
    case 0xe9:
        uint16_t address = (h << 8) | l; pc = address; clock_count += 5; break; 
    // jpe a16
    case 0xea:
        if (p == 0x1) JMP(); clock_count += 10; break; 
    // xchg
    case 0xeb:
        XCHG(); clock_count += 5; break; 
    // cpe a16
    case 0xec:
        if (p == 0x1) CALL(); clock_count += 11; break; 
    // call a16 illegal opcode
    case 0xed:
        clock_count += 11; break;
    // xri d8
    case 0xee:
        uint8_t byte = opcode[1]; uint16_t result = a ^ byte; handle_without_carry(result); cy = 0; ac = 0;a = result; clock_count += 7; break; 
    // rst 5
    case 0xef:
        unsigned char temp[3]; temp[1] = 40; temp[2] = 0; uint16_t return_address = pc + 2; write_byte(sp - 1, (return_address >> 8) & 0xff); write_byte(sp - 2, return_address & 0xff); sp = sp - 2; pc = temp[2] << 8 | temp[1]; clock_count += 11; break; 
    // rp
    case 0xf0:
        if (s == 0) RET(); clock_count += 5; break; 
    // pop psw
    case 0xf1:
        POP_PSW(); clock_count += 10; break; 
    // jp a16
    case 0xf2:
        if (s == 0) JMP(); clock_count += 10; break; 
    // di
    case 0xf3:
        clock_count += 4; interrupts_enabled = 0; break;
    // cp a16
    case 0xf4:
        if (s == 0) CALL(); clock_count += 11; break;
    // push psw
    case 0xf5:
        PUSH_PSW(); clock_count += 11; break;
    // ori d8
    case 0xf6:
        uint8_t byte = opcode[1]; uint16_t result = a | byte; handle_without_carry(result); a = result & 0xff; clock_count += 7; break; 
    // rst 6
    case 0xf7:
        unsigned char temp[3]; temp[1] = 48; temp[2] = 0; uint16_t return_address = pc + 2; write_byte(sp - 1, (return_address >> 8) & 0xff); write_byte(sp - 2, return_address & 0xff); sp = sp - 2; pc = temp[2] << 8 | temp[1]; clock_count += 11; break; 
    // rm
    case 0xf8:
        if (s == 1) RET(); clock_count += 5; break; 
    // sphl
    case 0xf9:
        uint16_t address = (h << 8) | l; sp = address; clock_count += 5; break; 
    // jm a16
    case 0xfa:
        if (s == 1) JMP(); clock_count += 10; break; 
    // ei
    case 0xfb:
        clock_count += 4; interrupts_enabled = 1; break;
    // cm a16
    case 0xfc:
        if (s == 1) CALL(); clock_count += 11; break; 
    // call a16 illegal opcode
    case 0xfd:
        clock_count += 11; break;
    // cpi d8
    case 0xfe:
        uint8_t byte = opcode[1]; uint16_t result = a - byte; cy = (a < byte); z = (result == 0); s = ((result & 0x80) != 0); p = parity(result & 0xff); clock_count += 7;
    // rst 7 
    case 0xff: 
        unsigned char temp[3]; temp[1] = 56; temp[2] = 0; uint16_t return_address = pc + 2; write_byte(sp - 1, (return_address >> 8) & 0xff); write_byte(sp - 2, return_address & 0xff); sp = sp - 2; pc = temp[2] << 8 | temp[1]; clock_count += 11; break; 
    default: 
        unimplemented_instruction(); break; 
    }
    
};

