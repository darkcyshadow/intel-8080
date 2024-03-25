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

void i8080::ADD(uint8_t* reg)
{
    uint16_t result = a + *reg;
    handle_arith_flag(result);
    a = result & 0xff;
}

void i8080::ADC(uint8_t* reg)
{
    uint16_t result = a + c + *reg;
    handle_arith_flag(result);
    a = result & 0xff;
}

void i8080::ANA(uint8_t *reg)
{
    uint16_t result = a & *reg;
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

void i8080::DAD(uint8_t *reg1, uint8_t *reg2)
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

void i8080::LDAX(uint8_t *reg1, uint8_t *reg2)
{
    uint16_t address = *reg1 << 8 | *reg2;
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

void i8080::PUSH(uint8_t* reg1, uint8_t* reg2)
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
    // mov d, b
    case 0x50:
        d = b;
    // mov d, c
    case 0x51:
        d = c;
    // mov d, d
    case 0x52:
        break;
    // mov d, e
    case 0x53:
        d = e;
    // mov d, h
    case 0x54:
        d = h;
    // mov d l
    case 0x55:
        d = l;
    // mov d, m
    case 0x56:
        uint16_t address = (h << 8) | l;
        d = memory[address];
    // mov d, a
    case 0x57:
        d = a;
    // mov e, b
    case 0x58:
        e = b;
    // mov e, c
    case 0x59:
        e = c;
    // mov e, d
    case 0x5a:
        e = d;
    // mov e, e
    case 0x5b:
        break;
    // mov e, h
    case 0x5c:
        e = h;
    // mov e l
    case 0x5d:
        e = l;
    // mov e, m
    case 0x5e:
        uint16_t address = (h << 8) | l;
        e = memory[address];
    // mov e, a
    case 0x5f:
        e = a;
    // mov h, b
    case 0x60:
        h = b;
    // mov h, c
    case 0x61:
        h = c;
    // mov h, d
    case 0x62:
        h = d;
    // mov h e
    case 0x63:
        h = e;
    // mov h, h
    case 0x64:
        break;
    // mov h, l
    case 0x65:
        h = l;
    // mov h, m
    case 0x66:
        uint16_t address = (h << 8) | l;
        h = memory[address];
    // mov h a
    case 0x67:
        h = a;
    // mov l b
    case 0x68:
        l = b;
    // mov l, c
    case 0x69:
        l = c;
    // mov l, d
    case 0x6a:
        l = d;
    // mov l e
    case 0x6b:
        l = e;
    // mov l , h
    case 0x6c:
        l = h;
    // mov l, l
    case 0x6d:
        break;
    // mov l, m
    case 0x6e:
        uint16_t address = (h << 8) | l;
        l = memory[address];
    // mov l a
    case 0x6f:
        l = a;
    // mov m, b
    case 0x70:
        uint16_t address = (h << 8) | l;
        memory[address] = b;
    // mov m, c
    case 0x71:
        uint16_t address = (h << 8) | l;
        memory[address] = c;
    // mov m, d
    case 0x72:
        uint16_t address = (h << 8) | l;
        memory[address] = d;
    // mov m, e
    case 0x73:
        uint16_t address = (h << 8) | l;
        memory[address] = e;
    // mov m, h
    case 0x74:
        uint16_t address = (h << 8) | l;
        memory[address] = h;
    // mov m, l
    case 0x75:
        uint16_t address = (h << 8) | l;
        memory[address] = l;
    // hlt
    case 0x76:
        exit(0);
    // move m, a
    case 0x77:
        uint16_t address = (h << 8) | l;
        memory[address] = a;
    // mov a, b
    case 0x78:
        a = b;
    // mov a, c
    case 0x79:
        a = c;
    // mov a, d
    case 0x7a:
        a = d;
    // mov a, e
    case 0x7b:
        a = e;
    // move a, h
    case 0x7c:
        a = h;
    // mov a, l
    case 0x7d:
        a = l;
    // mov a, m
    case 0x7e:
        uint16_t address = (h << 8) | l;
        a = memory[address];
    // mov a, e
    case 0x7f:
        break;
    // add b
    case 0x80:
        ADD(&b);
        break;
    // add c
    case 0x81:
        ADD(&c);
        break;
    // add d
    case 0x82:
        ADD(&d);
        break;
    // add e
    case 0x83:
        ADD(&e);
        break;
    // add h
    case 0x84:
        ADD(&h);
    // add l
    case 0x85:
        ADD(&l);
    // add m
    case 0x86:
        uint16_t address = (h << 8) | l;
        uint16_t result = a + memory[address];
        handle_arith_flag(result);
        a = result & 0xff;
    // add a
    case 0x87:
        ADD(&a);
    // adc b
    case 0x88:
        ADC(&b);
    // adc c
    case 0x89:
        ADC(&c);
    // adc d
    case 0x8a:
        ADC(&d);
    // adc e
    case 0x8b:
        ADC(&e);
    // adc h
    case 0x8c:
        ADC(&h);
    // adc l
    case 0x8d:
        ADC(&l);
    // adc m
    case 0x8e:
        uint16_t address = (h << 8) | l;
        uint16_t result = a + memory[address] + c;
        handle_arith_flag(result);
        a = result & 0xff;
    // adc a
    case 0x8f:
        ADC(&a);
    // sub b
    case 0x90:
        SUB(&b);
    // sub c
    case 0x91:
        SUB(&c);
    // sub d
    case 0x92:
        SUB(&d);
    // sub e
    case 0x93:
        SUB(&e);
    // sub h
    case 0x94:
        SUB(&h);
    // sub l
    case 0x95:
        SUB(&l);
    // sub m
    case 0x96:
        uint16_t address = (h << 8) | l;
        uint16_t result = a - memory[address];
        handle_without_carry(result);
        cy = (!result > 0xff);
    // sub a
    case 0x97:
        SUB(&a);
    // sbb b
    case 0x98:
        SBB(&b);
    // sbb c
    case 0x99:
        SBB(&c);
    // sbb d
    case 0x9a:
        SBB(&d);
    // sbb e
    case 0x9b:
        SBB(&e);
    // sbb h
    case 0x9c:
        SBB(&h);
    // sbb l
    case 0x9d:
        SBB(&l);
    // sbb m
    case 0x9e:
        uint16_t address = (h << 8) | l;
        memory[address] += cy;
        uint16_t result = a - memory[address];
        handle_arith_flag(result);
        a = result & 0xff;
    // sbb a
    case 0x9f:
        SBB(&a);
    // ana b
    case 0xa0:
        ANA(&b);
    // ana c
    case 0xa1:
        ANA(&c);
    // ana d
    case 0xa2:
        ANA(&d);
    // ana e
    case 0xa3:
        ANA(&e);
    // ana h
    case 0xa4:
        ANA(&h);
    // ana l
    case 0xa5:
        ANA(&l);
    // ana m
    case 0xa6:
        uint16_t address = (h << 8) | l;
        uint16_t result = a & memory[address];
        handle_arith_flag(result);
        a = result & 0xff;
    // ana a
    case 0xa7:
        ANA(&a);
    // xra b
    case 0xa8:
        XRA(&b);
    // xra c
    case 0xa9:
        XRA(&c);
    // xra d
    case 0xaa:
        XRA(&d);
    // xra e
    case 0xab:
        XRA(&e);
    // xra h
    case 0xac:
        XRA(&c);
    // xra l
    case 0xad:
        XRA(&d);
    // xra m
    case 0xae:
        uint16_t address = (h << 8) | l;
        uint16_t result = a ^ memory[address];
        handle_without_carry(result);
        cy = 0;
        ac = 0;
        a = result;
    // xra a
    case 0xaf:
        XRA(&a);
    // ora b
    case 0xb0:
        ORA(&b);
    // ora c
    case 0xb1:
        ORA(&c);
    // ora d
    case 0xb2:
        ORA(&d);
    // ora e
    case 0xb3:
        ORA(&e);
    // ora h
    case 0xb4:
        ORA(&h);
    // ora l
    case 0xb5:
        ORA(&l);
    // ora M
    case 0xb6:
        uint16_t address = (h << 8) | l;
        uint16_t result = a | memory[address];
        cy = 0;
        z = (result == 0);
        s = ((result & 0x80) != 0);
        p = parity(result & 0xff);
        a = result & 0xff;
    // ora a
    case 0xb7:
        ORA(&a);
    // cmp b
    case 0xb8:
        CMP(&b);
    // cmp c
    case 0xb9:
        CMP(&c);
    // cmp d
    case 0xba:
        CMP(&d);
    // cmp e
    case 0xbb:
        CMP(&e);
    // cmp h
    case 0xbc:
        CMP(&h);
    // cmp l
    case 0xbd:
        CMP(&l);
    // cmp m
    case 0xbe:
        uint16_t address = (h << 8) | l;
        uint16_t result = a - memory[address];
        cy = (a < memory[address]);
        z = (result == 0);
        s = ((result & 0x80) != 0);
        p = parity(result & 0xff);
    // cmp a 
    case 0xbf: 
        CMP(&a); 
    // rnz 
    case 0xc0: 
        if (z != 0x0)
        {
            RET(); 
        }
    // pop b 
    case 0xc1: 
        POP(&b, &c); 
    // jnz a16 
    case 0xc2: 
        if (z != 0x0)
        {
            JMP(); 
        }
    // jmp a16 
    case 0xc3: 
        JMP(); 
    // cnz a16 
    case 0xc4: 
        if (z != 0x0)
        {
            CALL(); 
        }
    // push b 
    case 0xc5: 
        PUSH(&b, &c); 
    // adi d8 
    case 0xc6: 
        uint8_t byte = opcode[1]; 
        uint16_t result = a + byte; 
        handle_arith_flag(result); 
        a = result & 0xff; 
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
    // rz 
    case 0xc8: 
        if (z == 0x0)
        {
            RET(); 
        }
    // ret 
    case 0xc9: 
        RET(); 
    // jz a16 
    case 0xca: 
        if (z == 0x0)
        {
            JMP(); 
        }
    // jmp a16 illegal opcode 
    case 0xcb: 
        break; 
    // cz a16 
    case 0xcc: 
        if (z == 0x0)
        {
            CALL(); 
        }
    // call a16 
    case 0xcd: 
        CALL(); 
    // aci d8 
    case 0xce: 

    }
}
