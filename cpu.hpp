#include <cstdlib>
#include <ctime>
#include <stdint.h>

class i8080
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
    /*opcode flags
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
    uint8_t z : 1;
    uint8_t s : 1;
    uint8_t p : 1;
    uint8_t cy : 1;
    uint8_t ac : 1;
    uint8_t pad : 3;

public:
    i8080();
    ~i8080();

    void execute_nop();
    int emulate(); 
    void unimplemented_instruction(); 
    void handle_arith_flag(uint16_t result); 
    void handle_logical_flag(uint16_t result); 
    int parity(int x, int size); 

};
