#pragma once

#include "Instruction.hpp"
#include "opt_array.hpp"
#include <sstream>

namespace SPIMDF {
    namespace BufferEntry {
        struct PreIssue {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
        };

        struct PreALU {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
        };

        struct PostALU {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
            uint32_t result = 0;
        };

        struct PreMemALU {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
        };

        struct PreMem {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
            uint32_t result = 0;
        };

        struct PostMem {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
            uint32_t result = 0;
        };
    };

    template<typename Entry_t, int N>
    struct Buffer {
        opt_array<Entry_t, N> entries;

        std::string ToPrintingString() const {
            std::stringstream ss;
            
            std::size_t i = 1;
            for (const auto& entry : entries) {
                ss << '\t' << "Entry " << i++ << ": ";

                if (entry.has_value())
                    ss << "[" << entry.value().instruction.ToString() << "]";

                ss << '\n';
            }

            return ss.str();
        }
    };

    using PreIssueQueue  = Buffer<BufferEntry::PreIssue, 4>;
 
    using PreALUQueue    = Buffer<BufferEntry::PreALU, 2>;
    using PostALUQueue   = Buffer<BufferEntry::PostALU, 2>;

    using PreMemALUQueue = Buffer<BufferEntry::PreMemALU, 2>;
    using PreMemQueue    = Buffer<BufferEntry::PreMem, 2>;
    using PostMemQueue   = Buffer<BufferEntry::PostMem, 2>;
}