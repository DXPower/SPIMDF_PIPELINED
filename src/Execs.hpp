#pragma once

#include "ISA.hpp"
#include "Instruction.hpp"

namespace SPIMDF {
    class CPU;

    struct Executor {
        CPU* cpu;
        Instruction curInstr;


        Executor(CPU& cpu): cpu(&cpu) { };
        virtual ~Executor() { };

        virtual void Consume() = 0;
        virtual void Produce() = 0;
    };

    struct FetchExec final : Executor {
        Instruction curInstr2;
        Instruction staller = Instruction::Create<ISA::NOP>(0);
        Instruction executed = Instruction::Create<ISA::NOP>(0);

        bool isBroken = false;

        FetchExec(CPU& cpu) : Executor(cpu) { };

        void Consume() override;
        void Produce() override;
        
        bool IsStalled() const;
        bool IsExecuted() const;

        private:
        void SetStaller(const Instruction& instr);
    };

    struct IssueExec final : Executor {
        IssueExec(CPU& cpu) : Executor(cpu) { };

        void Consume() override;
        void Produce() override;
    };

    struct ALUExec final : Executor {
        ALUExec(CPU& cpu) : Executor(cpu) { };

        void Consume() override;
        void Produce() override;
    };

    struct MemALUExec final : Executor {
        MemALUExec(CPU& cpu) : Executor(cpu) { };

        void Consume() override;
        void Produce() override;
    };

    struct MemExec final : Executor {
        MemExec(CPU& cpu) : Executor(cpu) { };

        void Consume() override;
        void Produce() override;
    };
}