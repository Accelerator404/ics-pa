#include <isa.h>
#include "local-include/reg.h"
#include <string.h>
// All registers are integer.
// While ra,sp,gp,tp are pointer/address registers.
const char *regsl[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
    unsigned n = sizeof(regsl)/sizeof(regsl[0]);
    printf("Infos of the registers:\n");
    printf("PC\t0x%x\t%u\n",cpu.pc,cpu.pc);
    for (int i = 0; i < n; ++i) {
        // The registers' type rtlreg_t is an alias for unit32_t.
        printf("%s\t0x%x\t%u\n",regsl[i],cpu.gpr[i]._32,cpu.gpr[i]._32);
    }
}

word_t isa_reg_str2val(const char *s, bool *success) {
    unsigned n = sizeof(regsl)/sizeof(regsl[0]);
    if (strcmp(s,"pc") == 0) {
        *success = true;
        return (word_t)cpu.pc;
    }
    else{
        for (int i = 0; i < n; ++i) {
            if (strcmp(s,regsl[i]) == 0){
                *success = true;
                return cpu.gpr[i]._32;
            }
        }
    }
    *success=false;
    return 0;
}
