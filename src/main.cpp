#include "CPU.hpp"
#include <cstdio>
#include "Disassembler.hpp"
#include "ISA.hpp"
#include "Instruction.hpp"
#include <iostream>

using namespace SPIMDF;

using voidFunc = void();

struct Foo {
    int x;
    int y;
};

struct Foo foo = { .x = 2, .y = 3 };

int main() {
    CPU cpu(256);

    SPIMDF::Disassemble("sample.txt", cpu);
    

    while (std::cin.get() == '\n') {
        printf("Cycle %lu:\t%u\t%s\n\n", cpu.GetCycle(), cpu.GetPC(), cpu.CurInstr().ToString().c_str());
        cpu.Clock();

        printf("\nRegisters\n");
        uint8_t base = 0;

        for (uint8_t i = 0; i < 4; i++) {
            printf(
                "R%02u:\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\n"
                , base
                , cpu.Reg(base + 0), cpu.Reg(base + 1), cpu.Reg(base + 2), cpu.Reg(base + 3)
                , cpu.Reg(base + 4), cpu.Reg(base + 5), cpu.Reg(base + 6), cpu.Reg(base + 7)
            );

            base += 8;
        }
    }
}