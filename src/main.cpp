#include "CPU.hpp"
#include <cstdio>
#include "Disassembler.hpp"
#include "ISA.hpp"
#include "Instruction.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

#include "opt_array.hpp"

using namespace SPIMDF;


int main(int argc, const char** argv) {
    CPU cpu(256);
    SPIMDF::Disassemble("sample.txt", cpu);
    // cpu.Mem(200) = 44;

    // uint32_t ia = 252;
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(0, 0, 1);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(1, 1, 1);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(2, 2, 400);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(3, 3, 10);

    // // Store initial values
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::SW>(2, 0, 0);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::SW>(2, 1, 4);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(2, 2, 8);
    
    // // Begin of loop
    // // cpu.Instr(ia += 4) = Instruction::Create<ISA::XOR>(0, 1, 0);
    // // cpu.Instr(ia += 4) = Instruction::Create<ISA::XOR>(0, 1, 1);
    // // cpu.Instr(ia += 4) = Instruction::Create<ISA::XOR>(0, 1, 0);

    // // cpu.Instr(ia += 4) = Instruction::Create<ISA::LW>(2, 0, -4);
    // // cpu.Instr(ia += 4) = Instruction::Create<ISA::LW>(2, 1, -8);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(3, 3, -1);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADD>(0, 1, 10);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::SW>(2, 10, 0);

    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(1, 0, 0);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(10, 1, 0);
    // // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADD>(0, 1, 1);

    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(2, 2, 4);

    // cpu.Instr(ia += 4) = Instruction::Create<ISA::BEQ>(3, 20, 1);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::J>(71);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::BRK>(0);


    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ORI>(0, 8, 200);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::SW>(0, 8, 200);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::LW>(10, 20, 200);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(1, 2, 15);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADDI>(1, 3, -5);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ADD>(2, 3, 1);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::ORI>(0, 7, 37);
    // cpu.Instr(ia += 4) = Instruction::Create<ISA::BRK>(0);

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


    std::ofstream output("simulation.txt", std::ios::binary);
    // auto& output = std::cout;

    char buffer[200];
    std::string temp;

    bool hitBreak = false;

    while (true) {
        output << "--------------------\n";
        
        // sprintf(buffer, "Cycle %lu:\t%u\t%s\n\n", cpu.GetCycle(), cpu.GetPC(), cpu.CurInstr().ToString().c_str());
        sprintf(buffer, "Cycle %lu:\n\n", cpu.GetCycle());
        output << buffer;

        // hitBreak = cpu.CurInstr().opcode == ISA::Opcode::BRK;

        cpu.Clock();

        // Execution Units
        output << "IF Unit:\n";
        if (cpu.executors.fetch.staller.IsNop())
            output << "\tWaiting Instruction:\n"; 
        else
            output << "\tWaiting Instruction: [" << cpu.executors.fetch.staller.ToString() << "]\n";

        if (cpu.executors.fetch.executed.IsNop())
            output << "\tExecuted Instruction:\n";
        else
            output << "\tExecuted Instruction: [" << cpu.executors.fetch.executed.ToString() << "]\n";

        // Pre-Issue Queue
        output << "Pre-Issue Queue:\n";
        output << cpu.queues.preIssue.ToPrintingString();

        // Pre-MemALU Queue
        output << "Pre-ALU1 Queue:\n";
        output << cpu.queues.preMemALU.ToPrintingString();

        // Pre-Mem Queue
        output << "Pre-MEM Queue:";
        output << cpu.queues.preMem.ToPrintingString() << '\n';

        // Post-Mem Queue
        output << "Post-MEM Queue:";
        output << cpu.queues.postMem.ToPrintingString() << '\n';

        // Pre-ALU Queue
        output << "Pre-ALU2 Queue:\n";
        output << cpu.queues.preALU.ToPrintingString();

        // Post-ALU Queue
        output << "Post-ALU2 Queue:";
        output << cpu.queues.postALU.ToPrintingString() << '\n';

        // Print registers
        uint8_t base = 0;
        output << "\nRegisters\n";

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

        output << std::flush;

        if (cpu.executors.fetch.isBroken) break;

        // if (argc != 2 || strcmp(argv[1], "DEBUG") != 0)
        //     std::cin.ignore();
    }

    output.close();
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
