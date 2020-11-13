#include "CPU.hpp"
#include <cstdio>
#include "Disassembler.hpp"
#include "ISA.hpp"
#include "Instruction.hpp"
#include <iostream>
#include <fstream>

#include "opt_array.hpp"

using namespace SPIMDF;

int main() {
    CPU cpu(256);
    SPIMDF::Disassemble("sample.txt", cpu);

    // uint32_t ia = 256;
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::SUB> (1, 2, 0);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::AND> (1, 2, 0);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::OR>  (1, 2, 0);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::XOR> (1, 2, 0);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::SLT> (2, 1, 0);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(1, 0, -50);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ANDI>(1, 0, 0x00F2);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ORI> (4, 0, 24);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::XORI>(1, 0, 0x00A4);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::SRL>(0, 0, 2);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::JR>(5);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::BRK> (0);

    // cpu.Instr(200) = Instruction::Create<ISA::ORI>(10, 10, 25);
    // cpu.Instr(204) = Instruction::Create<ISA::BRK>(10);

    // cpu.Reg(1) = 0x94;
    // cpu.Reg(2) = 24;
    // cpu.Reg(5) = 200;


    // std::ofstream output("simulation.txt", std::ios::binary);
    // auto& output = std::cout;

    char buffer[200];
    std::string temp;

    bool hitBreak = false;

    while (true) {
        std::cout << "--------------------\n";
        
        sprintf(buffer, "Cycle %lu:\t%u\t%s\n\n", cpu.GetCycle(), cpu.GetPC(), cpu.CurInstr().ToString().c_str());
        std::cout << buffer;

        // hitBreak = cpu.CurInstr().opcode == ISA::Opcode::BRK;

        cpu.Clock();

        // Execution Units
        std::cout << "IF Unit:\n";
        std::cout << "\tWaiting Instruction: " << cpu.executors.fetch.staller.ToString() << "\n";
        std::cout << "\tExecuted Instruction: " << cpu.executors.fetch.executed.ToString() << "\n";

        // Pre-Issue Queue
        std::cout << "Pre-Issue Queue:\n";
        std::size_t entryI = 0;

        for (const auto& entry : cpu.queues.preIssue.entries) {
            std::cout << "\tEntry " << entryI++ << ": ";

            if (entry.has_value())
                std::cout << "[" << entry.value().instruction.ToString() << "]";

            std::cout << "\n";
        }

        // Print registers
        uint8_t base = 0;
        std::cout << "Registers\n";

        for (uint8_t row = 0; row < 4; row++) {
            sprintf(
                buffer
                , "R%02u:\t%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i\n"
                , base
                , cpu.Reg(base + 0), cpu.Reg(base + 1), cpu.Reg(base + 2), cpu.Reg(base + 3)
                , cpu.Reg(base + 4), cpu.Reg(base + 5), cpu.Reg(base + 6), cpu.Reg(base + 7)
            );

            std::cout << buffer;

            base += 8;
        }

        // Print memory
        uint8_t word = 0;
        std::cout << "\nData\n";

        for (auto [addr, datum] : cpu.GetAllMem()) {
            if (word == 0)
                std::cout << addr << ":\t";
            
            std::cout << datum;

            if (word++ != 7)
                std::cout << "\t";
            else {
                word = 0;
                std::cout << "\n";
            }
        }

        std::cout << '\n' << std::flush;

        if (hitBreak) break;

        std::cin.ignore();
    }

    // output.close();
}

// if (a.is_empty()) printf("Is empty\n");
//     else printf("Not empty\n");

//     if (a.is_full()) printf("Is full\n");
//     else printf("Not full\n");

//     a.push_back(3);
//     a.push_back(4);
//     a.push_back(5);

//     if (a.is_empty()) printf("Is empty\n");
//     else printf("Not empty\n");

//     if (a.is_full()) printf("Is full\n");
//     else printf("Not full\n");

//     a.push_front(2);
//     a.push_front(1);

//     if (a.is_empty()) printf("Is empty\n");
//     else printf("Not empty\n");

//     if (a.is_full()) printf("Is full\n");
//     else printf("Not full\n");

//     printf("Popping back: %i\n", a.pop_back());
//     printf("Popping front: %i\n", a.pop_front());

//     if (a.is_empty()) printf("Is empty\n");
//     else printf("Not empty\n");

//     if (a.is_full()) printf("Is full\n");
//     else printf("Not full\n");

//     for (const auto& x : a) {
//         if (x.has_value())
//             printf("%i ", x.value());
//         else
//             printf("0 ");
//     }

//     printf("\n");
