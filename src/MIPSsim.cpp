// This is a concatenation of all the project files.

/*
Category 1 instructions
# All Cat1 instructions begin with 01, followed by the op code, then MIPS encoding
instr op    mips                                           Instruction format
j     0000: instr_index (26b)                              j
jr    0001: rs (5b),   0 (10b), hint 0 (5b), jr 8 (6b)     r
beq   0010: rs (5b),   rt (5b), offset (16b)               i
bltz  0011: rs (5b),   0 (5b),  offset (16b)               i
bgtz  0100: rs (5b),   0 (5b),  offset (16b)               i
break 0101: base (5b), rt (5b), offset (16b)               i
sw    0110: base (5b), rt (5b), offset (16b)               i
lw    0111: base (5b), rt (5b), offset (16b)               i 
sll   1000: 0 (5b),    rt (5b), rd (5b), sa (5b), 0 (6b)   r
srl   1001: 0 (5b),    rt (5b), rd (5b), sa (5b), 2 (6b)   r
sra   1010: 0 (5b),    rt (5b), rd (5b), sa (5b), 3 (6b)   r
nop   1011: 0 (26b)                                        i

Category 2 instructions
# All Cat2 instructions begin with 11, followed by the op code, then one of the following formats

3 operand: rs (5b), rt (5b), rd (5b), 0 (11b)
2 operand: rs (5b), rt (5b), imm (16b)

instr op
add  0000
sub  0001
mul  0010
and  0011
or   0100
xor  0101
nor  0110
slt  0111
addi 1000
andi 1001
ori  1010
xori 1011

*/

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace SPIMDF {
    // Convert 2s comp string to integer
    inline int32_t FromTwosComp(std::string mach) {
        int32_t val = 0;
        
        for (size_t i = 0; i < mach.size(); i++) {
            if (mach[mach.size() - i - 1] == '1') {
                if (i != mach.size() - 1)
                    val += std::pow(2, i);
                else
                    val -= std::pow(2, i); // MSB is negative its normal power
            }
        }

        return val;
    }

    namespace ISA {
        using namespace std;

        inline constexpr uint64_t Var = static_cast<uint64_t>(-1);
        
        template<auto T>
        constexpr bool IsVar() {
            return T == Var;
        }

        // Used for debugging
        template<typename FIter, typename NIter>
        static void FieldRepPrint(FIter fCur, const FIter& fEnd, NIter nCur) {
            for (; fCur != fEnd; fCur++) {
                printf("%s: %u\t", *nCur, *fCur);

                nCur++;
            }

            printf("\n");
        }

        struct RType {
            private:
            std::array<uint_least8_t, 5> fields;
            inline const static std::array<const char*, 5> names = { "rs", "rt", "rd", "sa", "func" };

            public:
            uint_least8_t& rs   = fields[0];
            uint_least8_t& rt   = fields[1];
            uint_least8_t& rd   = fields[2];
            uint_least8_t& sa   = fields[3];
            uint_least8_t& func = fields[4];    

            constexpr RType(const RType& copy) : fields(copy.fields) { }
            constexpr RType(const decltype(fields)& fields) : fields(fields) { }

            constexpr RType(uint8_t rs, uint8_t rt, uint8_t rd, uint8_t sa, uint8_t func) : fields({ 0, 0, 0, 0, 0}) {
                this->rs = rs;
                this->rt = rt;
                this->rd = rd;
                this->sa = sa;
                this->func = func;
            }

            RType& operator=(const RType& copy) {
                fields = copy.fields;
                return *this;
            }

            void Print() const {
                FieldRepPrint(fields.cbegin(), fields.cend(), names.cbegin());
            };

            // Factory function to define an instruction declaratively
            template<auto RS, auto RT, auto RD, auto SA, auto FUNC, typename... Args>
            static RType Factory(Args&&... a) {
                if constexpr ((std::is_same_v<Args, const std::string&> || ...)) {
                    // Decode from machine code!
                    std::string mach = std::get<0>(std::tie(a...)); // Get the machine code string, which will be the first argument
                    
                    return RType(
                          static_cast<uint8_t>(IsVar<RS>()   ? std::stoi(mach.substr(6, 5)  , nullptr, 2) : RS)
                        , static_cast<uint8_t>(IsVar<RT>()   ? std::stoi(mach.substr(11, 5) , nullptr, 2) : RT)
                        , static_cast<uint8_t>(IsVar<RD>()   ? std::stoi(mach.substr(16, 5) , nullptr, 2) : RD)
                        , static_cast<uint8_t>(IsVar<SA>()   ? std::stoi(mach.substr(21, 5) , nullptr, 2) : SA)
                        , static_cast<uint8_t>(IsVar<FUNC>() ? std::stoi(mach.substr(26, 6) , nullptr, 2) : FUNC)
                    );
                } else {
                    // Construct from passed values (moreso for testing/development purposes)
                    std::array<uint32_t, sizeof...(Args)> args{ a... };
                    uint8_t curArg = 0;

                    std::array<uint8_t, 5> fields {
                        static_cast<uint8_t>(IsVar<RS>()   ? args[curArg++] : RS)
                        , static_cast<uint8_t>(IsVar<RT>()   ? args[curArg++] : RT)
                        , static_cast<uint8_t>(IsVar<RD>()   ? args[curArg++] : RD)
                        , static_cast<uint8_t>(IsVar<SA>()   ? args[curArg++] : SA)
                        , static_cast<uint8_t>(IsVar<FUNC>() ? args[curArg] : FUNC)
                    };
                    return RType(fields);

                }
            }
        }; // struct RType

        struct IType {
            private:
            std::tuple<uint_least8_t, uint_least8_t, int_least16_t> fields;
            inline const static std::array<const char*, 3> names = { "rs", "rt", "imm" };

            public:
            uint_least8_t&  rs  = std::get<0>(fields);
            uint_least8_t&  rt  = std::get<1>(fields);
            int_least16_t& imm = std::get<2>(fields);    


            constexpr IType(const IType& copy) : fields(copy.fields) { }
            constexpr IType(const decltype(fields)& fields) : fields(fields) { };

            constexpr IType(uint8_t rs, uint8_t rt, int16_t imm) {
                this->rs = rs;
                this->rt = rt;
                this->imm = imm;
            }

            IType& operator=(const IType& copy) {
                fields = copy.fields;
                return *this;
            }

            void Print() const {
                std::array<int32_t, 3> fieldsArray{ rs, rt, imm };

                FieldRepPrint(fieldsArray.cbegin(), fieldsArray.cend(), names.cbegin());
            };
            
            // Factory function to define an instruction declaratively
            template<auto RS, auto RT, auto IMM, typename... Args>
            static IType Factory(Args&&... a) {
                if constexpr ((std::is_same_v<Args, const std::string&> || ...)) {
                    // Decode from machine code!
                    std::string mach = std::get<0>(std::tie(a...)); // Get the machine code string, which will be the first argument
                    return IType(
                          static_cast<uint8_t>(IsVar<RS>()  ? std::stoi(mach.substr(6, 5)  , nullptr, 2) : RS)
                        , static_cast<uint8_t>(IsVar<RT>()  ? std::stoi(mach.substr(11, 5) , nullptr, 2) : RT)
                        , static_cast<int16_t>(IsVar<IMM>() ? FromTwosComp(mach.substr(16, 16))          : IMM)
                    );
                } else {
                    // Decode from passed arguments
                    std::array<int32_t, sizeof...(Args)> args{a...};     
                    char curArg = 0;

                    std::tuple<uint_least8_t, uint_least8_t, uint_least16_t> fields {
                          static_cast<uint8_t>( IsVar<RS>() ? args[curArg++] : RS )
                        , static_cast<uint8_t>( IsVar<RT>() ? args[curArg++] : RT )
                        , static_cast<int16_t>(IsVar<IMM>() ? args[curArg++] : IMM)
                    };

                    return IType(fields);
                }
            }
        }; // struct IType
        
        struct JType {
            private:
            std::array<int_least32_t, 1> fields;
            inline const static std::array<const char*, 3> names = { "index" };

            public:
            int_least32_t& index = fields[0];

            JType(const JType& copy) : fields(copy.fields) { }

            JType(int32_t index) {
                this->index = index;
            }

            JType& operator=(const JType& copy) {
                fields = copy.fields;
                return *this;
            }

            void Print() const {
                FieldRepPrint(fields.cbegin(), fields.cend(), names.cbegin());
            };
            
            // Factory function to define an instruction declaratively
            template<auto INDEX>
            static JType Factory(int32_t index) {
                if constexpr (INDEX == Var) {
                    return JType(index);
                } else {
                    return JType(INDEX);
                }
            }

            static JType Decode(const std::string& mach) {
                return JType(FromTwosComp(mach.substr(6, 26)));
            }
        }; // struct JType

        
        namespace detail {
            // Instruction Definitions (declarative)
            // Cat1
            inline constexpr auto J    = JType::Factory<Var>;
            inline constexpr auto JR   = RType::Factory<Var, 0, 0, 0, 8, uint8_t>;
            inline constexpr auto BEQ  = IType::Factory<Var, Var, Var, uint8_t, uint8_t, int16_t>;
            inline constexpr auto BLTZ = IType::Factory<Var, 0, Var, uint8_t, int16_t>;
            inline constexpr auto BGTZ = IType::Factory<Var, 0, Var, uint8_t, int16_t>;
            inline constexpr auto SW   = IType::Factory<Var, Var, Var, uint8_t, uint8_t, int16_t>;
            inline constexpr auto LW   = IType::Factory<Var, Var, Var, uint8_t, uint8_t, int16_t>;
            inline constexpr auto SLL  = RType::Factory<0, Var, Var, Var, 0, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto SRL  = RType::Factory<0, Var, Var, Var, 2, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto SRA  = RType::Factory<0, Var, Var, Var, 3, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto NOP  = JType::Factory<0>;
            inline constexpr auto BRK  = JType::Factory<1>;

            #define DECODE_ARGS const std::string&

            // Decode cat1
            inline constexpr auto Decode_J    = JType::Decode;
            inline constexpr auto Decode_JR   = RType::Factory<Var, 0, 0, 0, 8, DECODE_ARGS>;
            inline constexpr auto Decode_BEQ  = IType::Factory<Var, Var, Var, DECODE_ARGS>;
            inline constexpr auto Decode_BLTZ = IType::Factory<Var, 0, Var, DECODE_ARGS>;
            inline constexpr auto Decode_BGTZ = IType::Factory<Var, 0, Var, DECODE_ARGS>;
            inline constexpr auto Decode_SW   = IType::Factory<Var, Var, Var, DECODE_ARGS>;
            inline constexpr auto Decode_LW   = IType::Factory<Var, Var, Var, DECODE_ARGS>;
            inline constexpr auto Decode_SLL  = RType::Factory<0, Var, Var, Var, 0, DECODE_ARGS>;
            inline constexpr auto Decode_SRL  = RType::Factory<0, Var, Var, Var, 2, DECODE_ARGS>;
            inline constexpr auto Decode_SRA  = RType::Factory<0, Var, Var, Var, 3, DECODE_ARGS>;
            inline constexpr auto Decode_NOP  = JType::Decode;
            inline constexpr auto Decode_BRK  = JType::Decode;

            // Cat2
            inline constexpr auto ADD  = RType::Factory<Var, Var, Var, 0, 0, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto SUB  = RType::Factory<Var, Var, Var, 0, 0, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto MUL  = RType::Factory<Var, Var, Var, 0, 0, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto AND  = RType::Factory<Var, Var, Var, 0, 0, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto OR   = RType::Factory<Var, Var, Var, 0, 0, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto XOR  = RType::Factory<Var, Var, Var, 0, 0, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto NOR  = RType::Factory<Var, Var, Var, 0, 0, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto SLT  = RType::Factory<Var, Var, Var, 0, 0, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto ADDI = IType::Factory<Var, Var, Var, uint8_t, uint8_t, int16_t>;
            inline constexpr auto ANDI = IType::Factory<Var, Var, Var, uint8_t, uint8_t, int16_t>;
            inline constexpr auto ORI  = IType::Factory<Var, Var, Var, uint8_t, uint8_t, int16_t>;
            inline constexpr auto XORI = IType::Factory<Var, Var, Var, uint8_t, uint8_t, int16_t>;
            
            inline constexpr auto Decode_ADD  = RType::Factory<Var, Var, Var, 0, 0, DECODE_ARGS>;
            inline constexpr auto Decode_SUB  = RType::Factory<Var, Var, Var, 0, 0, DECODE_ARGS>;
            inline constexpr auto Decode_MUL  = RType::Factory<Var, Var, Var, 0, 0, DECODE_ARGS>;
            inline constexpr auto Decode_AND  = RType::Factory<Var, Var, Var, 0, 0, DECODE_ARGS>;
            inline constexpr auto Decode_OR   = RType::Factory<Var, Var, Var, 0, 0, DECODE_ARGS>;
            inline constexpr auto Decode_XOR  = RType::Factory<Var, Var, Var, 0, 0, DECODE_ARGS>;
            inline constexpr auto Decode_NOR  = RType::Factory<Var, Var, Var, 0, 0, DECODE_ARGS>;
            inline constexpr auto Decode_SLT  = RType::Factory<Var, Var, Var, 0, 0, DECODE_ARGS>;
            inline constexpr auto Decode_ADDI = IType::Factory<Var, Var, Var, DECODE_ARGS>;
            inline constexpr auto Decode_ANDI = IType::Factory<Var, Var, Var, DECODE_ARGS>;
            inline constexpr auto Decode_ORI  = IType::Factory<Var, Var, Var, DECODE_ARGS>;
            inline constexpr auto Decode_XORI = IType::Factory<Var, Var, Var, DECODE_ARGS>;
        }

        // We have two layers of definition here to provide a better forward facing API that can still be compared against
        // (each instruction definition here will have a different address, whereas the ones in ::detail can have the same address)
        // Instruction Definitions (declarative)
        // Cat1
        inline constexpr auto J    = &detail::J;
        inline constexpr auto JR   = &detail::JR;
        inline constexpr auto BEQ  = &detail::BEQ;
        inline constexpr auto BLTZ = &detail::BLTZ;
        inline constexpr auto BGTZ = &detail::BGTZ;
        inline constexpr auto SW   = &detail::SW;
        inline constexpr auto LW   = &detail::LW;
        inline constexpr auto SLL  = &detail::SLL;
        inline constexpr auto SRL  = &detail::SRL;
        inline constexpr auto SRA  = &detail::SRA;
        inline constexpr auto NOP  = &detail::NOP;
        inline constexpr auto BRK  = &detail::BRK;

        // Cat2
        inline constexpr auto ADD  = &detail::ADD;
        inline constexpr auto SUB  = &detail::SUB;
        inline constexpr auto MUL  = &detail::MUL;
        inline constexpr auto AND  = &detail::AND;
        inline constexpr auto OR   = &detail::OR;
        inline constexpr auto XOR  = &detail::XOR;
        inline constexpr auto NOR  = &detail::NOR;
        inline constexpr auto SLT  = &detail::SLT;
        inline constexpr auto ADDI = &detail::ADDI;
        inline constexpr auto ANDI = &detail::ANDI;
        inline constexpr auto ORI  = &detail::ORI;
        inline constexpr auto XORI = &detail::XORI;

        enum class Opcode {
              J   
            , JR  
            , BEQ 
            , BLTZ
            , BGTZ
            , SW  
            , LW  
            , SLL 
            , SRL 
            , SRA 
            , NOP 
            , BRK
            , ADD 
            , SUB 
            , MUL 
            , AND 
            , OR  
            , XOR 
            , NOR 
            , SLT 
            , ADDI
            , ANDI
            , ORI 
            , XORI
        };
    } // namespace ISA

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

    // Microcode.hpp
    class Instruction;

    #define DEF_EX(X) void X(CPU& cpu, const Instruction& instr);
    #define DEF_PR(X) std::string X(const Instruction& instr);

    class CPU;

    // Predeclare executors and printers
    namespace Executors {
        // Category 1
        DEF_EX(J);
        DEF_EX(JR);
        DEF_EX(BEQ);
        DEF_EX(BLTZ);
        DEF_EX(BGTZ);
        DEF_EX(SW); 
        DEF_EX(LW); 
        DEF_EX(SLL);
        DEF_EX(SRL);
        DEF_EX(SRA);
        DEF_EX(NOP);
        DEF_EX(BRK);
        // Category 2
        DEF_EX(ADD); 
        DEF_EX(SUB); 
        DEF_EX(MUL); 
        DEF_EX(AND); 
        DEF_EX(OR);  
        DEF_EX(XOR); 
        DEF_EX(NOR); 
        DEF_EX(SLT); 
        DEF_EX(ADDI);
        DEF_EX(ANDI);
        DEF_EX(ORI); 
        DEF_EX(XORI);
    }

    namespace Printers {
        // Category 1
        DEF_PR(J);
        DEF_PR(JR);
        DEF_PR(BEQ);
        DEF_PR(BLTZ);
        DEF_PR(BGTZ);
        DEF_PR(SW); 
        DEF_PR(LW); 
        DEF_PR(SLL);
        DEF_PR(SRL);
        DEF_PR(SRA);
        DEF_PR(NOP);
        DEF_PR(BRK);
        // Category 2
        DEF_PR(ADD); 
        DEF_PR(SUB); 
        DEF_PR(MUL); 
        DEF_PR(AND); 
        DEF_PR(OR);  
        DEF_PR(XOR); 
        DEF_PR(NOR); 
        DEF_PR(SLT); 
        DEF_PR(ADDI);
        DEF_PR(ANDI);
        DEF_PR(ORI); 
        DEF_PR(XORI);
    }

    #undef DEF_EX
    #undef DEF_PR
    
    // Instruction.cpp
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
        : opcode(opcode), format(fm), executor(ep.first), printer(ep.second) { }

        template<class Format>
        const Format& GetFormat() const {
            return std::get<Format>(format);
        }

        void Execute(CPU& cpu) const {
            executor(cpu, *this);
        }

        std::string ToString() const {
            return printer(*this);
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
    }; // class Instruction

    class CPU {
        std::map<uint32_t, Instruction> program;
        std::map<uint32_t, int32_t> memory;
        std::array<int32_t, 32> registers { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                                          , 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

        uint64_t cycle = 1;
        uint32_t pc;

        public:
        CPU(uint32_t pc = 0) : pc(pc) { };

        Instruction& Instr(uint32_t addr) { return program[addr]; };
        const Instruction& Instr(uint32_t addr) const { return program.at(addr); };

        Instruction& CurInstr() { return Instr(pc); };
        const Instruction& CurInstr() const { return Instr(pc); };

        int32_t& Mem(uint32_t addr) { return memory[addr]; };
        const auto& GetAllMem() const { return memory; };

        int32_t& Reg(uint8_t regAddr) { return registers[regAddr]; };
        const int32_t& Reg(uint8_t regAddr) const { return registers[regAddr]; };

        uint32_t GetPC() const { return pc; };
        uint64_t GetCycle() const { return cycle; };

        void Clock() {
            // Point PC to delay slot to support correct branching operation
            pc += 4; // PC now points to delay slot
            Instr(pc - 4).Execute(*this); // But execute the current instruction

            cycle++;
        };

        // Sets PC = addr
        void Jump(uint32_t addr) {
            pc = addr;
        }
    }; // class CPU

    // Microcode.cpp
    #define EX_FUNC(X) void X(CPU& cpu, const Instruction& in)
    #define PR_FUNC(X) std::string X(const Instruction& in)

    #define EX_NULL(X) EX_FUNC(X) { printf(#X " executor undefined"); }
    #define PR_NULL(X) PR_FUNC(X) { return std::string(#X " printer undefined"); }

    namespace Executors {
        // Category 1

        EX_FUNC(J) {
            const ISA::JType& format = in.GetFormat<ISA::JType>();
            cpu.Jump((cpu.GetPC() & 0xF0000000) | (format.index << 2));
        }

        EX_FUNC(JR) {
            const auto& format = in.GetFormat<ISA::RType>();
            cpu.Jump(cpu.Reg(format.rs));
        }

        EX_FUNC(BEQ) {
            const ISA::IType& format = in.GetFormat<ISA::IType>();

            if (cpu.Reg(format.rs) == cpu.Reg(format.rt))
                cpu.Jump(cpu.GetPC() + (format.imm * 4));
        }

        EX_FUNC(BLTZ) {
            const ISA::IType& format = in.GetFormat<ISA::IType>();

            if (cpu.Reg(format.rs) < 0)
                cpu.Jump(cpu.GetPC() + (format.imm * 4));
        }

        EX_FUNC(BGTZ) {
            const auto& format = in.GetFormat<ISA::IType>();

            if (cpu.Reg(format.rs) > 0)
                cpu.Jump(cpu.GetPC() + (format.imm * 4));
        }

        EX_FUNC(SW) {
            const auto& format = in.GetFormat<ISA::IType>();
            cpu.Mem(cpu.Reg(format.rs) + format.imm) = cpu.Reg(format.rt);
        }

        EX_FUNC(LW) {
            const auto& format = in.GetFormat<ISA::IType>();
            cpu.Reg(format.rt) = cpu.Mem(cpu.Reg(format.rs) + format.imm);
        }

        EX_FUNC(SLL) {
            const auto& format = in.GetFormat<ISA::RType>();
            cpu.Reg(format.rd) = cpu.Reg(format.rt) << format.sa;
        }

        EX_FUNC(SRL) {
            const auto& format = in.GetFormat<ISA::RType>();
            // Cast to unsigned int to do guarantee logical shift, then shift by shamt
            cpu.Reg(format.rd) = static_cast<int32_t>(static_cast<uint32_t>(cpu.Reg(format.rt)) >> format.sa);
        }

        EX_FUNC(SRA) {
            const auto& format = in.GetFormat<ISA::RType>();
            cpu.Reg(format.rd) = cpu.Reg(format.rt) >> format.sa;
        }

        EX_FUNC(NOP) { }

        EX_FUNC(BRK) { }

        // Category 2
        EX_FUNC(ADD) {
            const auto& format = in.GetFormat<ISA::RType>();
            cpu.Reg(format.rd) = cpu.Reg(format.rs) + cpu.Reg(format.rt);
        }

        EX_FUNC(SUB) {
            const auto& format = in.GetFormat<ISA::RType>();
            cpu.Reg(format.rd) = cpu.Reg(format.rs) - cpu.Reg(format.rt);
        }

        EX_FUNC(MUL) {
            const auto& format = in.GetFormat<ISA::RType>();
            cpu.Reg(format.rd) = cpu.Reg(format.rs) * cpu.Reg(format.rt);
        }

        EX_FUNC(AND) {
            const auto& format = in.GetFormat<ISA::RType>();
            cpu.Reg(format.rd) = cpu.Reg(format.rs) & cpu.Reg(format.rt);
        }

        EX_FUNC(OR) {
            const auto& format = in.GetFormat<ISA::RType>();
            cpu.Reg(format.rd) = cpu.Reg(format.rs) | cpu.Reg(format.rt);
        } 

        EX_FUNC(XOR) {
            const auto& format = in.GetFormat<ISA::RType>();
            cpu.Reg(format.rd) = cpu.Reg(format.rs) ^ cpu.Reg(format.rt);
        }

        EX_FUNC(NOR) {
            const auto& format = in.GetFormat<ISA::RType>();
            cpu.Reg(format.rd) = ~(cpu.Reg(format.rs) | cpu.Reg(format.rt));
        }

        EX_FUNC(SLT) {
            const auto& format = in.GetFormat<ISA::RType>();
            cpu.Reg(format.rd) = cpu.Reg(format.rs) < cpu.Reg(format.rt);
        }

        EX_FUNC(ADDI) {
            const auto& format = in.GetFormat<ISA::IType>();
            cpu.Reg(format.rt) = cpu.Reg(format.rs) + format.imm;
        }

        EX_FUNC(ANDI) {
            const auto& format = in.GetFormat<ISA::IType>();
            cpu.Reg(format.rt) = cpu.Reg(format.rs) & static_cast<uint32_t>(format.imm); // Zero extend immediate
        }

        EX_FUNC(ORI) {
            const auto& format = in.GetFormat<ISA::IType>();
            cpu.Reg(format.rt) = cpu.Reg(format.rs) | static_cast<uint32_t>(format.imm); // Zero extend immediate
        }

        EX_FUNC(XORI) {
            const auto& format = in.GetFormat<ISA::IType>();
            cpu.Reg(format.rt) = cpu.Reg(format.rs) ^ static_cast<uint32_t>(format.imm); // Zero extend immediate
        }
    } // Executors

    namespace Printers {
        std::string String_RType(const char* opcode, const ISA::RType& format) {
            std::ostringstream ss;

            ss << opcode
            << " R" << +format.rd
            << ", R" << +format.rs
            << ", R" << +format.rt;

            return ss.str();
        }

        std::string String_IType(const char* opcode, const ISA::IType& format, int mult = 1) {
            std::ostringstream ss;

            ss << opcode
            << " R" << +format.rt
            << ", R" << +format.rs
            << ", #" << (format.imm * mult);

            return ss.str();
        }

        PR_FUNC(J) {
            const ISA::JType& format = in.GetFormat<ISA::JType>();

            std::stringstream ss;
            ss << "J #" << (format.index << 2);

            return ss.str();
        }

        // Category 1
        PR_FUNC(JR) {
            const ISA::RType& format = in.GetFormat<ISA::RType>();
            
            std::stringstream ss;
            ss << "JR R" << +format.rs;

            return ss.str();
        }

        PR_FUNC(BEQ) {
            const ISA::IType& format = in.GetFormat<ISA::IType>();

            std::stringstream ss;
            ss << "BEQ R" << + format.rs << ", R" << +format.rt << ", #" << (format.imm * 4);

            return ss.str();
        }

        PR_FUNC(BLTZ) {
            const ISA::IType& format = in.GetFormat<ISA::IType>();
            
            std::stringstream ss;
            ss << "BLTZ R" << +format.rs << ", #" << (format.imm * 4);

            return ss.str();
        }

        PR_FUNC(BGTZ) {
            const ISA::IType& format = in.GetFormat<ISA::IType>();
            
            std::stringstream ss;
            ss << "BGTZ R" << +format.rs << ", #" << (format.imm * 4);

            return ss.str();
        }

        PR_FUNC(SW) {
            const ISA::IType& format = in.GetFormat<ISA::IType>();

            std::stringstream ss;
            ss << "SW R" << +format.rt << ", " << format.imm << "(R" << +format.rs << ")";

            return ss.str();
        }

        PR_FUNC(LW) {
            const ISA::IType& format = in.GetFormat<ISA::IType>();

            std::stringstream ss;
            ss << "LW R" << +format.rt << ", " << format.imm << "(R" << +format.rs  << ")";

            return ss.str();
        }

        PR_FUNC(SLL) {
            const ISA::RType& format = in.GetFormat<ISA::RType>();

            std::stringstream ss;
            ss << "SLL R" << +format.rd << ", R" << +format.rt << ", #"  << +format.sa;

            return ss.str();
        }

        PR_FUNC(SRL) {
            const ISA::RType& format = in.GetFormat<ISA::RType>();

            std::stringstream ss;
            ss << "SRL R" << +format.rd << ", R" << +format.rt << ", #"  << +format.sa;

            return ss.str();
        }

        PR_FUNC(SRA) {
            const ISA::RType& format = in.GetFormat<ISA::RType>();

            std::stringstream ss;
            ss << "SRA R" << +format.rd << ", R" << +format.rt << ", #"  << +format.sa;

            return ss.str();
        }

        PR_FUNC(NOP) {
            return "NOP";
        }

        PR_FUNC(BRK) {
            return "BREAK";
        }

        // Category 2
        #define PR_CAT2_RTYPE(X) PR_FUNC(X) { \
            const ISA::RType& format = in.GetFormat<ISA::RType>(); \
            return String_RType(#X, format); \
        }

        #define PR_CAT2_ITYPE(X) PR_FUNC(X) { \
            const ISA::IType& format = in.GetFormat<ISA::IType>(); \
            return String_IType(#X, format); \
        }

        PR_CAT2_RTYPE(ADD)
        PR_CAT2_RTYPE(SUB); 
        PR_CAT2_RTYPE(MUL); 
        PR_CAT2_RTYPE(AND); 
        PR_CAT2_RTYPE(OR);  
        PR_CAT2_RTYPE(XOR); 
        PR_CAT2_RTYPE(NOR); 
        PR_CAT2_RTYPE(SLT); 
        PR_CAT2_ITYPE(ADDI);
        PR_CAT2_ITYPE(ANDI);
        PR_CAT2_ITYPE(ORI); 
        PR_CAT2_ITYPE(XORI);
    } // Printers

    #undef EX_FUNC
    #undef PR_FUNC
    #undef EX_NULL
    #undef PR_NULL

    // Disassembler.cpp
    template<auto Factory, auto Decoder>
    Instruction DecodeHelper(const std::string& mach) {
        auto format = Decoder(mach);
        return Instruction::CreateFromFormat<Factory>(format);
    }

    using DecodeFunc = Instruction(const std::string&);

    std::unordered_map<std::string, std::pair<ISA::Opcode, std::function<DecodeFunc>>> opcodeDecoders {
        // Category 1
          { "010000", { ISA::Opcode::J   , DecodeHelper<ISA::J   , ISA::detail::Decode_J   > } } 
        , { "010001", { ISA::Opcode::JR  , DecodeHelper<ISA::JR  , ISA::detail::Decode_JR  > } } 
        , { "010010", { ISA::Opcode::BEQ , DecodeHelper<ISA::BEQ , ISA::detail::Decode_BEQ > } } 
        , { "010011", { ISA::Opcode::BLTZ, DecodeHelper<ISA::BLTZ, ISA::detail::Decode_BLTZ> } } 
        , { "010100", { ISA::Opcode::BGTZ, DecodeHelper<ISA::BGTZ, ISA::detail::Decode_BGTZ> } } 
        , { "010101", { ISA::Opcode::BRK , DecodeHelper<ISA::BRK , ISA::detail::Decode_BRK > } }  
        , { "010110", { ISA::Opcode::SW  , DecodeHelper<ISA::SW  , ISA::detail::Decode_SW  > } } 
        , { "010111", { ISA::Opcode::LW  , DecodeHelper<ISA::LW  , ISA::detail::Decode_LW  > } } 
        , { "011000", { ISA::Opcode::SLL , DecodeHelper<ISA::SLL , ISA::detail::Decode_SLL > } } 
        , { "011001", { ISA::Opcode::SRL , DecodeHelper<ISA::SRL , ISA::detail::Decode_SRL > } } 
        , { "011010", { ISA::Opcode::SRA , DecodeHelper<ISA::SRA , ISA::detail::Decode_SRA > } } 
        , { "011011", { ISA::Opcode::NOP , DecodeHelper<ISA::NOP , ISA::detail::Decode_NOP > } } 

        // Category 2
        , { "110000", { ISA::Opcode::ADD , DecodeHelper<ISA::ADD , ISA::detail::Decode_ADD > } }
        , { "110001", { ISA::Opcode::SUB , DecodeHelper<ISA::SUB , ISA::detail::Decode_SUB > } }
        , { "110010", { ISA::Opcode::MUL , DecodeHelper<ISA::MUL , ISA::detail::Decode_MUL > } }
        , { "110011", { ISA::Opcode::AND , DecodeHelper<ISA::AND , ISA::detail::Decode_AND > } }
        , { "110100", { ISA::Opcode::OR  , DecodeHelper<ISA::OR  , ISA::detail::Decode_OR  > } }
        , { "110101", { ISA::Opcode::XOR , DecodeHelper<ISA::XOR , ISA::detail::Decode_XOR > } }
        , { "110110", { ISA::Opcode::NOR , DecodeHelper<ISA::NOR , ISA::detail::Decode_NOR > } }
        , { "110111", { ISA::Opcode::SLT , DecodeHelper<ISA::SLT , ISA::detail::Decode_SLT > } }
        , { "111000", { ISA::Opcode::ADDI, DecodeHelper<ISA::ADDI, ISA::detail::Decode_ADDI> } }
        , { "111001", { ISA::Opcode::ANDI, DecodeHelper<ISA::ANDI, ISA::detail::Decode_ANDI> } }
        , { "111010", { ISA::Opcode::ORI , DecodeHelper<ISA::ORI , ISA::detail::Decode_ORI > } }
        , { "111011", { ISA::Opcode::XORI, DecodeHelper<ISA::XORI, ISA::detail::Decode_XORI> } }
    };

    Instruction DecodeMachineCode(const std::string& mach) {
        std::string binOpcode = mach.substr(0, 6);

        const auto& [opcode, decoder] = opcodeDecoders[binOpcode];

        return decoder(mach);
    }

    int32_t DecodeProgramDatum(const std::string& mach) {
        return FromTwosComp(mach);
    }

    void Disassemble(const char* filename, CPU& cpu) {
        std::ifstream file(filename);
        std::ofstream output("disassembly.txt", std::ios::binary);
        char buffer[200];

        if (!file.is_open()) {
            output << "File not found" << std::endl;
            std::terminate();
        }

        std::string machCode;
        uint32_t curAddr = 256;

        while (file >> machCode) {
            std::string category = machCode.substr(0, 2);

            Instruction instr = DecodeMachineCode(machCode);

            cpu.Instr(curAddr) = instr;
        
            sprintf(buffer, "%s\t%u\t%s\n", machCode.c_str(), curAddr, instr.ToString().c_str());
            output << buffer;

            curAddr += 4;
            
            if (instr.opcode == ISA::Opcode::BRK)
                break;
        }

        while (file >> machCode) {
            int32_t datum = DecodeProgramDatum(machCode);

            cpu.Mem(curAddr) = datum;

            sprintf(buffer, "%s\t%u\t%i\n", machCode.c_str(), curAddr, datum);
            output << buffer;

            curAddr += 4;
        }

        output << std::flush;
        output.close();
    }
}

using namespace SPIMDF;

// main.cpp
int main(int argc, const char* argv[]) {
    CPU cpu(256);
    SPIMDF::Disassemble(argv[1], cpu);

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