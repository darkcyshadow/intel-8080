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

void i8080::ADC(uint8_t reg)
{
    uint16_t result = a + c + reg;
    handle_arith_flag(result);
    a = result & 0xff;
}

void i8080::ANA(uint8_t reg)
{
    uint16_t result = a & reg;
    handle_arith_flag(result);
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

void i8080::CMP(uint8_t reg)
{
    uint16_t result = a - reg;
    cy = (a < reg);
    z = (result == 0);
    s = ((result & 0x80) != 0);
    p = parity(result & 0xff);
}

void i8080::DAA()
{
    uint8_t lsb = (a << 4) >> 4;
    if (lsb > 0x09 || ac == 1)
    {
        uint16_t result = a + 0x06;
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

void i8080::DAD(uint8_t* reg1, uint8_t* reg2)
{
    uint32_t pair = *reg1 << 8 | *reg2;
    uint32_t hl = h << 8 | l;
    uint32_t result = hl + pair;
    cy = ((result & 0xffff0000) > 0); // check the upper 16 bits to see if there is an overflow beyond the 16 bits reg pair
    h = (result >> 8) & 0xff;
    l = result & 0xff;
}

void i8080::DCR(uint8_t *reg)
{
    uint16_t result = --*reg;
    handle_without_carry(result);
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
    pc = opcode[2] << 8 | opcode[1];
}

void i8080::LDA()
{
    uint16_t address = opcode[2] << 8 | opcode[1];
    a = memory[address];
}

void i8080::LDAX(uint8_t* reg1, uint8_t* reg2)
{
    uint16_t address = reg1 << 8 | reg2;
    a = memory[address];
}

void i8080::LHLD()
{
    uint16_t address = opcode[2] << 8 | opcode[1];
    l = memory[address];
    h = memory[address + 1];
}

void i8080::LXI(uint8_t *reg1, uint8_t *reg2)
{
    *reg1 = opcode[2];
    *reg2 = opcode[1];
}

void i8080::ORA(uint8_t reg)
{
    uint16_t result = a | reg;
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

void i8080::PUSH(uint8_t reg1, uint8_t reg2)
{
    sp -= 1;
    memory[sp] = reg1;
    sp -= 1;
    memory[sp] = reg2;
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

void i8080:: SBB(uint8_t* reg)
{
    *reg += cy; 
    uint16_t result = a - *reg; 
    handle_arith_flag(result); 
    a = result & 0xff; 
}

void i8080::SUB(uint8_t* reg)
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

void i8080::XRA(uint8_t* reg)
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
    // opcode is a pointer to the location in memory where the instruction is stored
    opcode = &memory[pc];
    switch (*opcode)
    {
    // nop
    case 0x00:
        break;
    // lxi b, word
    case 0x01:
        LXI(&b, &c); 
        break;
    // stax, b
    case 0x02:
        uint16_t bc = b << 8 | c;
        memory[bc] = a;
        break; 
    // inx, b
    case 0x03:
        INX(&b, &c); 
        break; 
    // inr, b
    case 0x04:
        INR(&b); 
        break; 
    // dcr, b
    case 0x05:
        DCR(&b); 
        break; 
    // mvi, b, d8
    case 0x06:
        uint16_t byte = opcode[1];
        b = byte;
        break;
    // rlc
    case 0x07:
        RLC(); 
        break; 
    // nop
    case 0x08:
        break;
    // dad b
    case 0x09:
        DAD(&b, &c); 
        break; 
    // ldax b
    case 0xa:
        LDAX(&b, &c); 
        break; 
    // dcx b
    case 0xb:
        DCX(&b, &c); 
        break; 
    // inr c
    case 0xc:
        INR(&c); 
        break; 
    // dcr, c
    case 0xd:
        DCR(&c); 
        break;
    // mvi, c, d8
    case 0xe:
        uint16_t value = opcode[1];
        c = value;
        break; 
    // rrc
    case 0xf:
        RRC(); 
        break; 
    // nop
    case 0x10:
        break;
    // lxi d, d16
    case 0x11:
        LXI(&d, &e); 
        break; 
    // stax d
    case 0x12:
        uint16_t address = (d << 8) | e; 
        memory[address] = a; 
        break; 
    // inx d
    case 0x13:
       INX(&d, &e); 
       break; 
    // inr, d
    case 0x14:
        INR(&d); 
        break; 
    // dcr, d
    case 0x15:
        DCR(&d); 
        break; 
    // mvi d, d8
    case 0x16:
        uint16_t result = opcode[1];
        d = result;
        break; 
    // ral
    case 0x17:
        RAL(); 
        break; 
    // nop
    case 0x18:
        break;
    // dad d
    case 0x19:
        DAD(&d, &e); 
        break; 
    // ldax, d 
    case 0x1a: 
        LDAX(&d, &e); 
        break;
    // dcx, d 
    case 0x1b: 
        DCX(&d, &e); 
        break; 
    // inr e
    case 0x1c: 
        INR(&e); 
        break; 
    // dcr, e 
    case 0x1d: 
        DCR(&e); 
        break; 
    // mvi, e, d8
    case 0x1e:
        uint8_t byte = opcode[1]; 
        e = byte; 
        break; 
    // rar
    case 0x1f: 
        RAR(); 
        break; 
    // nop 
    case 0x20: 
        break; 
    // lxi h, d16
    case 0x21: 
        LXI(&h, &l); 
        break; 
    // shld a16 
    case 0x22: 
        SHLD(); 
        break; 
    // inx, h 
    case 0x23: 
        INX(&h, &l); 
        break;
    // inr h 
    case 0x24: 
        INR(&h); 
        break; 
    // dcr, h
    case 0x25: 
        DCR(&h); 
        break; 
    // mvi h, d8
    case 0x26: 
        uint8_t byte = opcode[1]; 
        h = byte; 
        break; 
    // daa
    case 0x27: 
        DAA(); 
        break; 
    // nop
    case 0x28: 
        break; 
    // dad h 
    case 0x29: 
        DAD(&h, &l); 
        break; 
    // lhld a16
    case 0x2a: 
        LHLD(); 
        break; 
    // dcx h 
    case 0x2b: 
        DCX(&h, &l); 
        break; 
    // inr l 
    case 0x2c: 
        INR(&l); 
        break; 
    // dcr l 
    case 0x2d: 
        DCR(&l); 
    // mvi l, d8 
    case 0x2e: 
        uint8_t byte = opcode[1]; 
        l = byte;
        break; 
    // cma
    case 0x2f: 
        CMA(); 
    // nop 
    case 0x30: 
        break; 
    // lxi sp, d16 
    case 0x31: 
        uint8_t second = opcode[1]; 
        uint8_t third = opcode[2];
        sp = (third << 8) | second; 
        break; 
    // sta a16
    case 0x32: 
        STA(); 
        break;
    // inx sp 
    case 0x33: 
        ++sp; 
    // inr m
    case 0x34: 
        uint16_t address = (h << 8) | l; 
        memory[address]++; 
        handle_without_carry(memory[address]); 
        break; 
    // dcr m 
    case 0x35: 
        uint16_t address = (h << 8) | l; 
        memory[address]--; 
        handle_without_carry(memory[address]); 
        break; 
    // mvi m, d8 
    case 0x36: 
        uint16_t address = (h << 8) | l; 
        memory[address] = opcode[1]; 
        break; 
    // stc
    case 0x37: 
        cy = 0x1; 
    // nop 
    case 0x38: 
        break; 
    // dad sp 
    case 0x39: 
        uint16_t pair = (h << 8) | l; 
        uint32_t result = pair + sp; 
        cy = ((result & 0xffff0000) > 0); 
        h = (result >> 8) & 0xff; 
        l = result * 0xff; 
        break; 
    // lda a16 
    case 0x3a: 
        LDA(); 
        break;     
    // dcx sp 
    case 0x3b:
        --sp; 
        break; 
    // inr a 
    case 0x3c: 
        INR(&a);  
        break; 
    // dcr a 
    case 0x3d: 
        DCR(&a); 
        break; 
    // mvi a, d8 
    case 0x3e: 
        uint8_t byte = opcode[1]; 
        a = byte;
        break;  
    // cmc
    case 0x3f: 
        cy = ~cy; 
        break; 
    // mov b, b 
    case 0x40: 
        break; 
    // mov b, c 
    case 0x41: 
        b = c; 
    // mov b d 
    case 0x42: 
        b = d; 
    // mov b e 
    case 0x43: 
        b = e; 
    // mov b h
    case 0x44: 
        b = h; 
    // mov b l 
    case 0x45: 
        b = l; 
    // mov b m 
    case 0x46:
        uint16_t address = (h << 8) | l; 
        b = memory[address]; 
    // mov b a 
    case 0x47:  
        b = a; 
    // mov c, b 
    case 0x48: 
        c = b; 
    // mov c, c, 
    case 0x49: 
        break; 
    // mov c, d 
    case 0x4a: 
        c = d; 
    // mov c,e 
    case 0x4b: 
        c = e; 
    // mov c, h 
    case 0x4c: 
        c = h; 
    // mov c, l 
    case 0x4d: 
        c = l; 
    // mov c, m 
    case 0x4e: 
        uint16_t address = (h << 8) | l; 
        c = memory[address]; 
    // mov c, a
    case 0x4f: 
        c = a; 
    // 


}
