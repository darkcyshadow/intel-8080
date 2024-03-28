#include <cstdlib>
#include <ctime>
#include <stdint.h>



/*
Memory map:
    ROM
    $0000-$07ff:    invaders.h
    $0800-$0fff:    invadersg.
    $1000-$17ff:    invaders.f
    $1800-$1fff:    invaders.e

    RAM
    $2000-$23ff:    work RAM
    $2400-$3fff:    video RAM

    $4000-:     RAM mirror
  
opcode flag:s
    z   - zero
    set to 1 when result equals 0

    s   - sign
    set to 1 when bit 7 (MSB) is set

    p   - parity
    set whenever answer has even parity

    cy  - carry
    set to 1 when instruction resulted in carry out

    ac  - auxiliary carry
    indicates when a carry has been generated out of the
    least significant four bits of the accumulator register
    following an instruction. It is primarily used in decimal (BCD)
    arithmetic instructions, it is adding 6 to adjust BCD arithmetic.
*/



class i8080
{
private:
  uint8_t memory[0xFFFF];
  uint16_t pc = 0; 
  uint16_t sp = 0; 
  unsigned char* opcode; 
  
  // registers 
  uint8_t a = 0; 
  uint8_t b = 0; 
  uint8_t c = 0; 
  uint8_t d = 0; 
  uint8_t e = 0; 
  uint8_t h = 0; 
  uint8_t l = 0; 
  
  // flags 
  uint8_t z : 1;
  uint8_t s : 1;
  uint8_t p : 1;
  uint8_t cy : 1;
  uint8_t ac : 1;
  uint8_t pad : 3;

  // custom hardware for space invaders 
  uint16_t reg_shift; 
  uint8_t shift_offset; 

public:
  i8080();
  ~i8080();

  
  void interrupt(uint8_t opcode); 

  //io
  uint8_t in_port[4];
  uint8_t out_port[7];
  // status 
  uint8_t halt; 
  uint8_t interrupts_enabled; 
  uint64_t clock_count; 
  uint64_t instruction_count; 

  


  
  int emulate();
  
  void handle_arith_flag(uint16_t result);
  void handle_without_carry(uint16_t result); 

  void unimplemented_instruction(); 

  int parity(uint16_t result); 

  void ADD(uint8_t* reg); 
  void ADC(uint8_t* reg); 
  void ANA(uint8_t* reg); 
  void CALL(); 
  void CMA(); 
  void CMC(); 
  void CMP(uint8_t* reg); 
  void DAA(); 
  void DAD(uint8_t* reg1, uint8_t* reg2); 
  void DCR(uint8_t *reg); 
  void DCX(uint8_t* reg1, uint8_t* reg2); 
  void INR(uint8_t *reg); 
  void INX(uint8_t *reg1, uint8_t *reg2); 
  void JMP();
  void LDA(); 
  void LDAX(uint8_t *reg1, uint8_t* reg2); 
  void LHLD(); 
  void LXI(uint8_t *reg1, uint8_t *reg2); 
  void ORA(uint8_t* reg); 
  void POP(uint8_t* reg1, uint8_t* reg2); 
  void POP_PSW(); 
  void PUSH(uint8_t* reg1, uint8_t* reg2); 
  void PUSH_PSW(); 
  void RAL(); 
  void RAR(); 
  void RET(); 
  void RLC(); 
  void RRC(); 
  void SHLD(); 
  void STA(); 
  void SBB(uint8_t *reg); 
  void SUB(uint8_t* reg); 
  void XCHG(); 
  void XRA(uint8_t* reg); 
  void XTHL(); 




};
