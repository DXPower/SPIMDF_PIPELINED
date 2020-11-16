#pragma once

#include "ISA.hpp"
#include "Instruction.hpp"
#include <functional>
#include "Buffer.hpp"

namespace SPIMDF {
    class CPU;

    struct Executor {
        CPU* cpu;

        Executor(CPU& cpu): cpu(&cpu) { };
        virtual ~Executor() { };

        virtual void Consume() = 0;
        virtual void Produce() = 0;
    };

    struct FetchExec final : Executor {
        Instruction slot1;
        Instruction slot2;
        Instruction staller = Instruction::Create<ISA::NOP>(0);
        Instruction executed = Instruction::Create<ISA::NOP>(0);

        bool isBroken = false;

        FetchExec(CPU& cpu) : Executor(cpu) { };

        void Consume() override;
        void Produce() override;
        
        bool IsStalled() const;
        bool IsExecuted() const;

        bool HasStallerPreIssueHazard() const;

        private:
        void SetStaller(const Instruction& instr);
    };

    struct IssueExec final : Executor {
        Instruction slot1;
        Instruction slot2;

        IssueExec(CPU& cpu) : Executor(cpu) { };

        // bool MapActiveInstructions(const std::function<bool(const Instruction&)>& func) const;

        void Consume() override;
        void Produce() override;
    };

    struct ALUExec final : Executor {
        Instruction slot;

        ALUExec(CPU& cpu) : Executor(cpu) { };

        void Consume() override;
        void Produce() override;
    };

    struct MemALUExec final : Executor {
        Instruction slot;

        MemALUExec(CPU& cpu) : Executor(cpu) { };

        void Consume() override;
        void Produce() override;
    };

    struct MemExec final : Executor {
        std::optional<BufferEntry::PreMem> slot;

        MemExec(CPU& cpu) : Executor(cpu) { };

        void Consume() override;
        void Produce() override;
    };

    struct WritebackExec final : Executor {
        std::optional<BufferEntry::PostALU> slotALU;
        std::optional<BufferEntry::PostMem> slotMem;

        WritebackExec(CPU& cpu) : Executor(cpu) { };

        void Consume() override;
        void Produce() override;
    };
}