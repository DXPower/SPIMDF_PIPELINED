#pragma once

#include "ISA.hpp"
#include "Microcode.hpp"

#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace SPIMDF {
    namespace detail {
        template<auto FA, auto FB>
        constexpr std::enable_if_t<FA == FB || FA != FB, bool> SameFunctionPointer() {
            return FA == FB;
        }
        
        template<auto FA, auto FB>
        constexpr std::enable_if_t<!std::is_same_v<decltype(FA), decltype(FB)>, bool> SameFunctionPointer() {
            return false;
        }
    }

    class Instruction {
        public:
        using Executor_g = void(CPU& cpu, const Instruction&);
        using Printer_g = std::string(const Instruction&);
        using EP = std::pair<std::function<Executor_g>, std::function<Printer_g>>;

        ISA::Opcode opcode;

        private:
        std::function<Executor_g> executor;
        std::function<Printer_g> printer;
        std::variant<ISA::RType, ISA::IType, ISA::JType> format;

        public:
        template<class Format>
        Instruction(ISA::Opcode opcode, Format&& fm, const EP& ep)
            : opcode(opcode), format(fm), executor(ep.first), printer(ep.second)
        { }

        Instruction(const Instruction& copy)
            : opcode(copy.opcode)
            , executor(copy.executor)
            , printer(copy.printer)
            , format(copy.format)
        { }

        Instruction(Instruction&& other)
            : opcode(other.opcode)
            , executor(other.executor)
            , printer(other.printer)
            , format(other.format)
        {
            other.opcode = ISA::Opcode::NOP;
            other.format = (*ISA::NOP)(0);
            other.executor = Executors::NOP;
            other.printer = Printers::NOP;
        }

        Instruction& operator=(const Instruction& copy) {
            opcode = copy.opcode;
            executor = copy.executor;
            printer = copy.printer;
            format = copy.format;

            return *this;
        }

        Instruction& operator=(Instruction&& other) {
            opcode = other.opcode;
            executor = other.executor;
            printer = other.printer;
            format = other.format;

            other.opcode = ISA::Opcode::NOP;
            other.format = (*ISA::NOP)(0);
            other.executor = Executors::NOP;
            other.printer = Printers::NOP;

            return *this;
        }

        auto GetDeps() const {
            if (std::holds_alternative<ISA::RType>(format)) {
                return ISA::ParseFormatDeps(GetFormat<ISA::RType>());
            } else if (std::holds_alternative<ISA::IType>(format)) {
                return ISA::ParseFormatDeps(GetFormat<ISA::IType>());
            } else {
                return ISA::ParseFormatDeps(GetFormat<ISA::JType>());
            }
        }

        std::string GetDepsString() const {
            const auto [dependencies, affects] = GetDeps();
            std::string result = "";

            if (dependencies.size() != 0) {
                result += "\tDepends on: ";

                for (const auto& r : dependencies) {
                    result += " R" + std::to_string(r);
                }

            }

            if (affects.has_value())
                result += " Affects: R" + std::to_string(affects.value());

            return result;
        }

        template<class Format>
        const Format& GetFormat() const {
            return std::get<Format>(format);
        }

        void Execute(CPU& cpu) const {
            executor(cpu, *this);
        }

        bool IsJump() const {
            return opcode == ISA::Opcode::BEQ
                || opcode == ISA::Opcode::BGTZ
                || opcode == ISA::Opcode::BLTZ
                || opcode == ISA::Opcode::JR
                || opcode == ISA::Opcode::JR;
        }

        bool IsMemAccess() const {
            return opcode == ISA::Opcode::SW || opcode == ISA::Opcode::LW;
        }

        bool IsLoad() const {
            return opcode == ISA::Opcode::LW;
        }

        bool IsStore() const {
            return opcode == ISA::Opcode::SW;
        }

        bool IsNop() const {
            return opcode == ISA::Opcode::NOP;
        }

        std::string ToString() const {
            return printer(*this) + " " + GetDepsString();
        }
        
        void Print() const {
            printf("%s\n", ToString().c_str());
        }

        template<auto Factory, typename Format>
        static Instruction CreateFromFormat(const Format& format) {
            #define DEF_IN(X) \
            if constexpr (detail::SameFunctionPointer<Factory, ISA::X>()) \
                return Instruction(ISA::Opcode::X, format, std::make_pair(std::function(Executors::X), std::function(Printers::X)))
            
            // Category 1
            DEF_IN(J);  
            else DEF_IN(JR);  
            else DEF_IN(BEQ); 
            else DEF_IN(BLTZ);
            else DEF_IN(BGTZ);
            else DEF_IN(SW);  
            else DEF_IN(LW);  
            else DEF_IN(SLL); 
            else DEF_IN(SRL); 
            else DEF_IN(SRA); 
            else DEF_IN(NOP);
            else DEF_IN(BRK);

            // Category 2
            else DEF_IN(ADD); 
            else DEF_IN(SUB); 
            else DEF_IN(MUL); 
            else DEF_IN(AND); 
            else DEF_IN(OR);  
            else DEF_IN(XOR); 
            else DEF_IN(NOR); 
            else DEF_IN(SLT); 
            else DEF_IN(ADDI);
            else DEF_IN(ANDI);
            else DEF_IN(ORI); 
            else DEF_IN(XORI);
            else {
                void* _ = static_cast<void*>(Factory); // This error will occur if you add "&" when passing the instruction to Create
            }

            #undef DEF_IN
        }

        template<auto Factory, typename... Args>
        static Instruction Create(Args&&... args) {
            #define DEF_IN(X) \
            if constexpr (detail::SameFunctionPointer<Factory, ISA::X>()) \
                return Instruction(ISA::Opcode::X, (*ISA::X)(std::forward<Args>(args)...), std::make_pair(std::function(Executors::X), std::function(Printers::X)))
            
            // Category 1
            DEF_IN(J);  
            else DEF_IN(JR);  
            else DEF_IN(BEQ); 
            else DEF_IN(BLTZ);
            else DEF_IN(BGTZ);
            else DEF_IN(SW);  
            else DEF_IN(LW);  
            else DEF_IN(SLL); 
            else DEF_IN(SRL); 
            else DEF_IN(SRA); 
            else DEF_IN(NOP);
            else DEF_IN(BRK);

            // Category 2
            else DEF_IN(ADD); 
            else DEF_IN(SUB); 
            else DEF_IN(MUL); 
            else DEF_IN(AND); 
            else DEF_IN(OR);  
            else DEF_IN(XOR); 
            else DEF_IN(NOR); 
            else DEF_IN(SLT); 
            else DEF_IN(ADDI);
            else DEF_IN(ANDI);
            else DEF_IN(ORI); 
            else DEF_IN(XORI);
            else {
                void* _ = static_cast<void*>(Factory); // This error will occur if you add "&" when passing the instruction to Create
            }

            #undef DEF_IN
        }

        Instruction() : Instruction(Create<ISA::NOP>(0)) { }; // Default to just a NOP instruction
    };
}