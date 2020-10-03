#include "CPU.hpp"
#include <cstdio>
#include "Disassembler.hpp"
#include "ISA.hpp"
#include "Instruction.hpp"
#include <iostream>
#include <fstream>

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

    std::ofstream output("simulation.txt", std::ios::binary);
    char buffer[200];

    bool hitBreak = false;

    while (true) {
        output << "--------------------\n";
        
        sprintf(buffer, "Cycle %lu:\t%u\t%s\n\n", cpu.GetCycle(), cpu.GetPC(), cpu.CurInstr().ToString().c_str());
        output << buffer;

        hitBreak = cpu.CurInstr().opcode == ISA::Opcode::BRK;

        cpu.Clock();

        // Print registers
        uint8_t base = 0;
        output << "Registers\n";

        for (uint8_t row = 0; row < 4; row++) {
            sprintf(
                buffer
                , "R%02u:\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\n"
                , base
                , cpu.Reg(base + 0), cpu.Reg(base + 1), cpu.Reg(base + 2), cpu.Reg(base + 3)
                , cpu.Reg(base + 4), cpu.Reg(base + 5), cpu.Reg(base + 6), cpu.Reg(base + 7)
            );

            output << buffer;

            base += 8;
        }

        // Print memory
        uint8_t word = 0;
        output << "\nData\n";

        for (auto [addr, datum] : cpu.GetAllMem()) {
            if (word == 0)
                output << addr << ":\t";
            
            output << datum;

            if (word++ != 7)
                output << "\t";
            else {
                word = 0;
                output << "\n";
            }
        }

        output << '\n' << std::flush;

        if (hitBreak) break;
    }

    output.close();
}