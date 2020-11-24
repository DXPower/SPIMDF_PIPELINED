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
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <cstdint>
#include <cmath>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>


namespace SPIMDF {
    //!--------------------------------------------------------------------------------------------------------
    //! Instruction.hpp
    //!--------------------------------------------------------------------------------------------------------

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
        struct RType;
        struct IType;
        struct JType;

        inline constexpr uint64_t Var = static_cast<uint64_t>(-1);
        // The following are definitions for defining dependencies and affections
        enum class Dep {
              RS
            , RT
            , RD
            , None
        };

        template<auto T>
        constexpr bool IsVar() {
            return T == Var;
        }

        template<typename FIter, typename NIter>
        void FieldRepPrint(FIter fCur, const FIter& fEnd, NIter nCur) {
            for (; fCur != fEnd; fCur++) {
                printf("%s: %u\t", *nCur, *fCur);

                nCur++;
            }

            printf("\n");
        }

        template<typename Format>
        uint8_t ParseRegFromFormat(const Format& format, Dep dep) {
            if (dep == Dep::RT) return format.rt;
            if (dep == Dep::RS) return format.rs;
            if constexpr (std::is_same_v<Format, RType>) {
                if (dep == Dep::RD) return format.rd;
            }

            return 255;
        }

        // Get deps in the format of [vector, uint8_t] => deps, affects
        template<typename Format>
        std::tuple<std::vector<uint8_t>, std::optional<uint8_t>> ParseFormatDeps(const Format& format) {
            auto tup = std::tuple<std::vector<uint8_t>, std::optional<uint8_t>>(std::vector<uint8_t>(), std::nullopt);
            auto& [deps, affects] = tup;

            if constexpr (std::is_same_v<Format, JType>) // JType has no dependencies/affects
                return tup;
            else {
                deps.reserve(2);

                // Get all depended registers
                for (const Dep& d : format.dependencies) {
                    if (d != Dep::None)
                        deps.push_back(ParseRegFromFormat(format, d));
                }

                // Get affected register if it exists
                if (format.affects != Dep::None)
                    affects = ParseRegFromFormat(format, format.affects);

                return tup;
            }
        }

        struct RType {
            private:
            std::array<uint_least8_t, 5> fields;
            inline const static std::array<const char*, 5> names = { "rs", "rt", "rd", "sa", "func" };

            public:
            std::array<Dep, 2> dependencies;
            Dep affects;

            uint_least8_t& rs   = fields[0];
            uint_least8_t& rt   = fields[1];
            uint_least8_t& rd   = fields[2];
            uint_least8_t& sa   = fields[3];
            uint_least8_t& func = fields[4];    

            constexpr RType(const RType& copy) : fields(copy.fields), dependencies(copy.dependencies), affects(copy.affects) { }
            constexpr RType(const decltype(fields)& fields, const std::array<Dep, 2>& deps, Dep affects)
                : fields(fields), dependencies(deps), affects(affects) { }

            constexpr RType(uint8_t rs, uint8_t rt, uint8_t rd, uint8_t sa, uint8_t func, const std::array<Dep, 2>& deps, Dep affects)
                : fields({ 0, 0, 0, 0, 0})
                , dependencies(deps)
                , affects(affects)
            {
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
            template<auto RS, auto RT, auto RD, auto SA, auto FUNC, Dep DEP1, Dep DEP2, Dep AFF, typename... Args>
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
                        , { DEP1, DEP2 }
                        , AFF
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

                    return RType(fields, { DEP1, DEP2 }, AFF);
                }
            }
        };

        struct IType {
            private:
            std::tuple<uint_least8_t, uint_least8_t, int_least16_t> fields;
            inline const static std::array<const char*, 3> names = { "rs", "rt", "imm" };

            public:
            std::array<Dep, 2> dependencies;
            Dep affects;

            uint_least8_t&  rs  = std::get<0>(fields);
            uint_least8_t&  rt  = std::get<1>(fields);
            int_least16_t& imm = std::get<2>(fields);    


            constexpr IType(const IType& copy) : fields(copy.fields), dependencies(copy.dependencies), affects(copy.affects) { }
            constexpr IType(const decltype(fields)& fields, const std::array<Dep, 2>& deps, Dep affects)
                : fields(fields), dependencies(deps), affects(affects)
            { };

            constexpr IType(uint8_t rs, uint8_t rt, int16_t imm, const std::array<Dep, 2>& deps, Dep affects)
                : dependencies(deps), affects(affects)
            {
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
            template<auto RS, auto RT, auto IMM, Dep DEP1, Dep DEP2, Dep AFF, typename... Args>
            static IType Factory(Args&&... a) {
                if constexpr ((std::is_same_v<Args, const std::string&> || ...)) {
                    // Decode from machine code!
                    std::string mach = std::get<0>(std::tie(a...)); // Get the machine code string, which will be the first argument
                    return IType(
                          static_cast<uint8_t>(IsVar<RS>()  ? std::stoi(mach.substr(6, 5)  , nullptr, 2) : RS)
                        , static_cast<uint8_t>(IsVar<RT>()  ? std::stoi(mach.substr(11, 5) , nullptr, 2) : RT)
                        , static_cast<int16_t>(IsVar<IMM>() ? FromTwosComp(mach.substr(16, 16))          : IMM)
                        , { DEP1, DEP2 }
                        , AFF
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

                    return IType(fields, { DEP1, DEP2 }, AFF);
                }
            }
        };
        
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
        };

        
        namespace detail {
            // Instruction Definitions (declarative)
            // Cat1
            inline constexpr auto J    = JType::Factory<Var>;
            inline constexpr auto JR   = RType::Factory<Var, 0, 0, 0, 8,     Dep::RS, Dep::None, Dep::None, uint8_t>;
            inline constexpr auto BEQ  = IType::Factory<Var, Var, Var,       Dep::RS, Dep::RT,   Dep::None, uint8_t, uint8_t, int16_t>;
            inline constexpr auto BLTZ = IType::Factory<Var, 0, Var,         Dep::RS, Dep::None, Dep::None, uint8_t, int16_t>;
            inline constexpr auto BGTZ = IType::Factory<Var, 0, Var,         Dep::RS, Dep::None, Dep::None, uint8_t, int16_t>;
            inline constexpr auto SW   = IType::Factory<Var, Var, Var,       Dep::RS, Dep::RT,   Dep::None, uint8_t, uint8_t, int16_t>;
            inline constexpr auto LW   = IType::Factory<Var, Var, Var,       Dep::RS, Dep::None, Dep::RT,   uint8_t, uint8_t, int16_t>;
            inline constexpr auto SLL  = RType::Factory<0, Var, Var, Var, 0, Dep::RT, Dep::None, Dep::RD,   uint8_t, uint8_t, uint8_t>;
            inline constexpr auto SRL  = RType::Factory<0, Var, Var, Var, 2, Dep::RT, Dep::None, Dep::RD,   uint8_t, uint8_t, uint8_t>;
            inline constexpr auto SRA  = RType::Factory<0, Var, Var, Var, 3, Dep::RT, Dep::None, Dep::RD,   uint8_t, uint8_t, uint8_t>;
            inline constexpr auto NOP  = JType::Factory<0>;
            inline constexpr auto BRK  = JType::Factory<1>;

            #define DECODE_ARGS const std::string&

            // Decode cat1
            inline constexpr auto Decode_J    = JType::Decode;
            inline constexpr auto Decode_JR   = RType::Factory<Var, 0, 0, 0, 8,     Dep::RS, Dep::None, Dep::None, DECODE_ARGS>;
            inline constexpr auto Decode_BEQ  = IType::Factory<Var, Var, Var,       Dep::RS, Dep::RT,   Dep::None, DECODE_ARGS>;
            inline constexpr auto Decode_BLTZ = IType::Factory<Var, 0, Var,         Dep::RS, Dep::None, Dep::None, DECODE_ARGS>;
            inline constexpr auto Decode_BGTZ = IType::Factory<Var, 0, Var,         Dep::RS, Dep::None, Dep::None, DECODE_ARGS>;
            inline constexpr auto Decode_SW   = IType::Factory<Var, Var, Var,       Dep::RS, Dep::RT,   Dep::None, DECODE_ARGS>;
            inline constexpr auto Decode_LW   = IType::Factory<Var, Var, Var,       Dep::RS, Dep::None, Dep::RT,   DECODE_ARGS>;
            inline constexpr auto Decode_SLL  = RType::Factory<0, Var, Var, Var, 0, Dep::RT, Dep::None, Dep::RD,   DECODE_ARGS>;
            inline constexpr auto Decode_SRL  = RType::Factory<0, Var, Var, Var, 2, Dep::RT, Dep::None, Dep::RD,   DECODE_ARGS>;
            inline constexpr auto Decode_SRA  = RType::Factory<0, Var, Var, Var, 3, Dep::RT, Dep::None, Dep::RD,   DECODE_ARGS>;
            inline constexpr auto Decode_NOP  = JType::Decode;
            inline constexpr auto Decode_BRK  = JType::Decode;

            // Cat2
            inline constexpr auto ADD  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto SUB  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto MUL  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto AND  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto OR   = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto XOR  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto NOR  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto SLT  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, uint8_t, uint8_t, uint8_t>;
            inline constexpr auto ADDI = IType::Factory<Var, Var, Var, Dep::RS, Dep::None, Dep::RT, uint8_t, uint8_t, int16_t>;
            inline constexpr auto ANDI = IType::Factory<Var, Var, Var, Dep::RS, Dep::None, Dep::RT, uint8_t, uint8_t, int16_t>;
            inline constexpr auto ORI  = IType::Factory<Var, Var, Var, Dep::RS, Dep::None, Dep::RT, uint8_t, uint8_t, int16_t>;
            inline constexpr auto XORI = IType::Factory<Var, Var, Var, Dep::RS, Dep::None, Dep::RT, uint8_t, uint8_t, int16_t>;
            
            inline constexpr auto Decode_ADD  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, DECODE_ARGS>;
            inline constexpr auto Decode_SUB  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, DECODE_ARGS>;
            inline constexpr auto Decode_MUL  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, DECODE_ARGS>;
            inline constexpr auto Decode_AND  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, DECODE_ARGS>;
            inline constexpr auto Decode_OR   = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, DECODE_ARGS>;
            inline constexpr auto Decode_XOR  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, DECODE_ARGS>;
            inline constexpr auto Decode_NOR  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, DECODE_ARGS>;
            inline constexpr auto Decode_SLT  = RType::Factory<Var, Var, Var, 0, 0, Dep::RS, Dep::RT, Dep::RD, DECODE_ARGS>;
            inline constexpr auto Decode_ADDI = IType::Factory<Var, Var, Var, Dep::RS, Dep::None, Dep::RT, DECODE_ARGS>;
            inline constexpr auto Decode_ANDI = IType::Factory<Var, Var, Var, Dep::RS, Dep::None, Dep::RT, DECODE_ARGS>;
            inline constexpr auto Decode_ORI  = IType::Factory<Var, Var, Var, Dep::RS, Dep::None, Dep::RT, DECODE_ARGS>;
            inline constexpr auto Decode_XORI = IType::Factory<Var, Var, Var, Dep::RS, Dep::None, Dep::RT, DECODE_ARGS>;
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
    }

//!--------------------------------------------------------------------------------------------------------
//! Microcode.hpp
//!--------------------------------------------------------------------------------------------------------

    class Instruction;
    class CPU;

    #define DEF_EX(X) void X(CPU& cpu, const Instruction& instr, const std::function<void(int32_t)>&);
    #define DEF_PR(X) std::string X(const Instruction& instr);

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

//!--------------------------------------------------------------------------------------------------------
//! Instruction.hpp
//!--------------------------------------------------------------------------------------------------------

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
        using ResultCB_g = void(int32_t);
        using Executor_g = void(CPU& cpu, const Instruction&, const std::function<ResultCB_g>&);
        using Printer_g = std::string(const Instruction&);
        using EP = std::pair<std::function<Executor_g>, std::function<Printer_g>>;

        using Deps_t = std::vector<uint8_t>;
        using Affects_t = std::optional<uint8_t>;

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

        template<class Format>
        const Format& GetFormat() const {
            return std::get<Format>(format);
        }

        void Execute(CPU& cpu) const {
            executor(cpu, *this, [](int32_t) { });
        }

        int32_t ExecuteResult(CPU& cpu) const {
            int32_t res = 0xDEAD;

            const auto resultCB = [&](int32_t x) {
                res = x;
            };

            executor(cpu, *this, resultCB);

            return res;
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

        bool IsJump() const {
            return opcode == ISA::Opcode::BEQ
                || opcode == ISA::Opcode::BGTZ
                || opcode == ISA::Opcode::BLTZ
                || opcode == ISA::Opcode::J
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

        std::string ToString(bool ignoreNop = false) const {
            if (opcode == ISA::Opcode::NOP && ignoreNop) {
                return "";
            }

            // return printer(*this) + " " + GetDepsString();
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
    };

//!--------------------------------------------------------------------------------------------------------
//! opt_array.hpp
//!--------------------------------------------------------------------------------------------------------

    template<typename T, int N>
    class opt_array : public std::array<std::optional<T>, N> {
        using El_t = std::optional<T>;
        public:
        opt_array() : std::array<El_t, N>() {
            this->fill(std::nullopt);
        };

        auto next_slot() {
            return std::find(this->begin(), this->end(), std::nullopt);
        }

        const auto next_slot() const {
            return std::find(this->cbegin(), this->cend(), std::nullopt);
        }

        // If there is room, return iterator to location. Else, return iterator to end()
        typename std::array<El_t, N>::iterator push_back(T&& o) {
            auto slot = next_slot();

            if (is_full())
                return slot;

            *slot = std::move(o);
            return slot;
        }

        // If there is room, return iterator to location. Else, return iterator to end()
        typename std::array<El_t, N>::iterator push_front(T&& o) {
            if (!is_full()) {
                std::rotate(this->rbegin(), this->rbegin() + 1, this->rend()); // Single rotate to right
                (*this)[0] = std::move(o);

                return this->begin();
            } else {
                return this->end();
            }
        }

        // If there is room, return iterator to location. Else, return iterator to end()
        T pop_front() {
        T popped = std::move((*this)[0].value()); // Get frontmost element
        (*this)[0] = std::nullopt;

        std::rotate(this->begin(), this->begin() + 1, this->end()); // Single rotate to left

        return popped;
        }

        T pop_back() {
            auto it = std::prev(next_slot());
            T popped = std::move(it->value()); // Get backmost element
            
            *it = std::nullopt;

            return popped;
        }

        T pull(const typename std::array<std::optional<T>, N>::iterator& pos) {
            T pulled = std::move(pos->value());

            pos->reset();
            std::rotate(pos, pos + 1, this->end()); // Rotate to the left from the removed position

            return pulled;
        }

        void remove(const typename std::array<std::optional<T>, N>::iterator& pos) {
            pull(pos);
        }

        bool is_empty() const {
            return next_slot() == this->begin();
        }

        bool is_full() const {
            return next_slot() == this->end();
        }

        std::size_t num_empty() const {
            return std::distance(next_slot(), this->end());
        }
    };



//!--------------------------------------------------------------------------------------------------------
//! Buffer.hpp
//!--------------------------------------------------------------------------------------------------------

    namespace BufferEntry {
        struct PreIssue {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
        };

        struct PreALU {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
        };

        struct PostALU {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
            int32_t result = 0;
        };

        struct PreMemALU {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
        };

        struct PreMem {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
            uint32_t address = 0;
        };

        struct PostMem {
            Instruction instruction = Instruction::Create<ISA::NOP>(5);
            int32_t result = 0;
        };
    };

    template<typename Entry_t, int N>
    struct Buffer {
        opt_array<Entry_t, N> entries;

        std::string ToPrintingString() const {
            std::stringstream ss;
            
            if constexpr (N > 1) {
                std::size_t i = 0;
                for (const auto& entry : entries) {
                    ss << '\t' << "Entry " << i++ << ":";

                    if (entry.has_value())
                        ss << " [" << entry.value().instruction.ToString() << "]";

                    ss << '\n';
                }
            } else {
                if (entries[0].has_value())
                    ss << " [" << entries[0].value().instruction.ToString() << "]";
            }

            return ss.str();
        }
    };

    using PreIssueQueue  = Buffer<BufferEntry::PreIssue, 4>;
 
    using PreALUQueue    = Buffer<BufferEntry::PreALU, 2>;
    using PostALUQueue   = Buffer<BufferEntry::PostALU, 1>;

    using PreMemALUQueue = Buffer<BufferEntry::PreMemALU, 2>;
    using PreMemQueue    = Buffer<BufferEntry::PreMem, 1>;
    using PostMemQueue   = Buffer<BufferEntry::PostMem, 1>;

//!--------------------------------------------------------------------------------------------------------
//! Execs.hpp
//!--------------------------------------------------------------------------------------------------------

    class CPU;

    struct Executor {
        CPU* cpu;

        Executor(CPU& cpu) : cpu(&cpu) { };
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

//!--------------------------------------------------------------------------------------------------------
//! CPU.hpp
//!--------------------------------------------------------------------------------------------------------

    enum class Hazard {
          RAW
        , WAW
        , WAR
    };

    class CPU {
        struct Register_t {
            int32_t value = 0;
            bool pendingRead = false;
            bool pendingWrite = false;
        };

        std::map<uint32_t, Instruction> program;
        std::map<uint32_t, int32_t> memory;
        std::array<Register_t, 32> registers;

        uint64_t cycle = 1;
        uint32_t pc;

        public:
        // Queues
        struct {
            PreIssueQueue preIssue;
            PreALUQueue preALU;
            PostALUQueue postALU;
            PreMemALUQueue preMemALU;
            PreMemQueue preMem;
            PostMemQueue postMem;
        } queues;

        // Executors
        struct {
            FetchExec fetch;
            IssueExec issue;
            ALUExec alu;
            MemALUExec memALU;
            MemExec mem;
            WritebackExec writeback;
        } executors;

        CPU(uint32_t pc = 0)
        : pc(pc)
        , executors({
              FetchExec(*this)
            , IssueExec(*this)
            , ALUExec(*this)
            , MemALUExec(*this)
            , MemExec(*this) 
            , WritebackExec(*this)
        })
        { };

        Instruction& Instr(uint32_t addr) { return program[addr]; };
        const Instruction& Instr(uint32_t addr) const { return program.at(addr); };

        Instruction& CurInstr() { return Instr(pc); };
        const Instruction& CurInstr() const { return Instr(pc); };

        int32_t& Mem(uint32_t addr) { return memory[addr]; };
        const auto& GetAllMem() const { return memory; };

        int32_t& Reg(uint8_t regAddr) { return registers[regAddr].value; };
        const int32_t& Reg(uint8_t regAddr) const { return registers[regAddr].value; };

        bool IsRegPendingRead(uint8_t regAddr) const { return registers[regAddr].pendingRead; };
        bool IsRegPendingWrite(uint8_t regAddr) const { return registers[regAddr].pendingWrite; };
        void SetRegPendingRead(uint8_t regAddr, bool flag) { registers[regAddr].pendingRead = flag; };
        void SetRegPendingWrite(uint8_t regAddr, bool flag) { registers[regAddr].pendingWrite = flag; };

        uint32_t GetPC() const { return pc; };
        uint64_t GetCycle() const { return cycle; };

        void Clock() {
            executors.fetch.Consume();
            executors.issue.Consume();
            executors.alu.Consume();
            executors.memALU.Consume();
            executors.mem.Consume();
            executors.writeback.Consume();

            executors.fetch.Produce();
            executors.issue.Produce();
            executors.alu.Produce();
            executors.memALU.Produce();
            executors.mem.Produce();
            executors.writeback.Produce();

            cycle++;
        };

        // Sets PC = addr
        void Jump(uint32_t addr) {
            pc = addr;
        }

        // Sets PC = PC + offset
        void RelJump(int32_t offset) {
            pc = pc + offset;
        }

        // Called when an instruction is either issued or flushed from pipeline
        void SetLocks(const Instruction& instr, bool flag) {
            const auto [deps, affects] = instr.GetDeps();

            for (const auto& r : deps) {
                SetRegPendingRead(r, flag);
            }

            if (affects.has_value())
                SetRegPendingWrite(affects.value(), flag);
        }

        // Called when an instruction is issued and we have to update the locks
        void AddLocks(const Instruction& instr) {
            SetLocks(instr, true);
        }

        // Called when an instruction is flushed and we have to update the locks
        void RemoveLocks(const Instruction& instr) {
            SetLocks(instr, false);
        }

        template<Hazard... Args>
        bool HasActiveHazard(const Instruction& instr) const {
            static_assert(sizeof...(Args) > 0);

            const auto [deps, affects] = instr.GetDeps();

            // Check RAW
            if constexpr (((Args == Hazard::RAW) || ...)) {
                for (const auto& r : deps) {
                    if (IsRegPendingWrite(r))
                        return true;
                }
            }

            if (!affects.has_value()) return false; // Cannot have WAW or WAR if there are no affects
                
            // Check WAW
            if constexpr (((Args == Hazard::WAW) || ...)) {
                if (IsRegPendingWrite(affects.value()))
                    return true;
            }

            // Check WAR
            if constexpr (((Args == Hazard::WAR) || ...)) {
                return IsRegPendingRead(affects.value());
                    
            }

            return false;
        }

        template<Hazard... Args>
        static bool HasInterHazard(const Instruction& earlier, const Instruction& later) {
            static_assert(sizeof...(Args) > 0);

            const auto [eDeps, eAffects] = earlier.GetDeps();
            const auto [lDeps, lAffects] = later.GetDeps();

            // Check RAW
            if constexpr (((Args == Hazard::RAW) || ...)) {
                for (const auto& r : lDeps) {
                    if (r == eAffects)
                        return true;
                }
            }

            if (!lAffects.has_value()) return false; // Cannot have WAR or WAW if there is no affects

            // Check WAR
            if constexpr (((Args == Hazard::WAR) || ...)) {
                for (const auto& r : eDeps) {
                    if (r == lAffects)
                        return true;
                }
            }

            // Check WAW
            if constexpr (((Args == Hazard::WAW) || ...)) {
                return lAffects == eAffects;
            }

            return false;
        }
    };
}

//!--------------------------------------------------------------------------------------------------------
//! Execs.cpp
//!--------------------------------------------------------------------------------------------------------
// This is the meat of the pipeline

using namespace SPIMDF;

void FetchExec::Consume() {
    if (IsStalled() || isBroken)
        return;
    
    // Check how many empty slots there are so we fetch the right amount
    std::size_t numEmpty = cpu->queues.preIssue.entries.num_empty();

    // Decode  for first slot
    if (numEmpty == 0) return; // Check for empty space in preissue queue
    if (cpu->CurInstr().opcode == ISA::Opcode::BRK) {
        isBroken = true;
        goto DecodedJumpOrBreak;
    } 
    if (cpu->CurInstr().IsJump()) // If it is a jump, we need to stall
        goto DecodedJumpOrBreak;

    // Set first slot
    slot1 = cpu->CurInstr();
    cpu->RelJump(4);

    // Decode for second slot
    if (numEmpty == 1) return; // Check for empty space in preissue queue
    if (cpu->CurInstr().opcode == ISA::Opcode::BRK) {
        isBroken = true;
        goto DecodedJumpOrBreak;
    } 
    if (cpu->CurInstr().IsJump()) // If it is a jump, we need to stall
        goto DecodedJumpOrBreak;

    // Set second slot
    slot2 = cpu->CurInstr();
    cpu->RelJump(4);
    return;

DecodedJumpOrBreak: // Stall if we encounter a jump instruction
    staller = cpu->CurInstr();
    cpu->RelJump(4);
    return;
}

void FetchExec::Produce() {
    // Don't need to check for empty space because we checked that in Consume().
    // Using std::move will make the instruction a noop.
    if (slot1.opcode != ISA::Opcode::NOP)
        cpu->queues.preIssue.entries.push_back(BufferEntry::PreIssue{std::move(slot1)});
    if (slot2.opcode != ISA::Opcode::NOP)
        cpu->queues.preIssue.entries.push_back(BufferEntry::PreIssue{std::move(slot2)});
    
    // Remove the executed instruction if it exists
    if (IsExecuted()) {
        const auto _ = std::move(executed); // Clear previously executed instruction
    }

    if (IsStalled()) {
        // Check if we are stalled. If we are, check reg status and possibly move to execution
        if (!cpu->HasActiveHazard<Hazard::RAW>(staller) && !HasStallerPreIssueHazard()) {
            staller.Execute(*cpu);
            executed = std::move(staller);
        }
    }
}

bool FetchExec::HasStallerPreIssueHazard() const {
    for (const auto& entry : cpu->queues.preIssue.entries) {
        if (!entry.has_value()) break;
        
        if (cpu->HasInterHazard<Hazard::RAW>(entry->instruction, staller))
            return true;
    }

    return false;
}

bool FetchExec::IsStalled() const {
    return staller.opcode != ISA::Opcode::NOP;
}

bool FetchExec::IsExecuted() const {
    return executed.opcode != ISA::Opcode::NOP;
}

void IssueExec::Consume() {
    auto& preIssEntries = cpu->queues.preIssue.entries;
    
    auto instr1 = preIssEntries.end();
    auto instr2 = preIssEntries.end();

    for (auto it = preIssEntries.begin(); it != preIssEntries.end(); it++) {
        auto& entry = *it;

        if (!entry.has_value()) break;
        
        Instruction& potentialIssue = entry.value().instruction;

        // Check for structural hazard with existing instructions in PreALU and PreMemALU
        if (potentialIssue.IsMemAccess() && cpu->queues.preMemALU.entries.is_full())
            continue;
        
        if (!potentialIssue.IsMemAccess() && cpu->queues.preALU.entries.is_full())
            continue;

        // Check if RAW or WAW hazard exists on active instructions (anything issued but not finished)
        if (cpu->HasActiveHazard<Hazard::RAW, Hazard::WAW>(potentialIssue))
            continue;
     
        // Now check all previous not-issued instructions for hazards
        for (auto pit = preIssEntries.begin(); pit != it; pit++) {
            // Check RAW, WAW, WAR hazard
            if (cpu->HasInterHazard<Hazard::RAW, Hazard::WAW, Hazard::WAR>(pit->value().instruction, potentialIssue))
                goto SKIP_INSTR; // continue outer loop

            // Make sure all loads only happen once previous stores have been issued (if it potentialIssue is a load)
            // Additionally make sure any stores happen in order (if potentialIssue is a store)
            if (potentialIssue.IsMemAccess() && pit->value().instruction.IsStore())
                goto SKIP_INSTR; // continue outer loop
        }

        // No hazard, select this instruction.
        // But, we can't change the array because we are iterating through it
        if (instr1 == preIssEntries.end()) {
            instr1 = it;
        } else if (instr2 == preIssEntries.end()) {
            // Now we need to check if this second instruction will have a structural hazard with the first instruction selected
            if (instr1->value().instruction.IsMemAccess() == potentialIssue.IsMemAccess()) // If they are the same type of "access", then hazard
                continue;

            instr2 = it;
            break; // Maximum of 2 instructions.
        }

        SKIP_INSTR: // For continuing outer loop from inner loop
        ; // Noop
    }

    // Continue with the selected instructions
    // Do second instruction first so that the iterator positions are not invalidated
    //TODO: Check if HasInterWAW_WAR here is necessary
    // if (instr2 != preIssEntries.end() && !CPU::HasInterHazard<Hazard::WAW, Hazard::WAR>(instr1->value().instruction, instr2->value().instruction)) {
    if (instr2 != preIssEntries.end()) {
        slot2 = preIssEntries.pull(instr2).instruction;
        cpu->AddLocks(slot2); // Need to add locks here because a branch instruction will not check slots for hazards on execution
    }

    if (instr1 != preIssEntries.end()) {
        slot1 = preIssEntries.pull(instr1).instruction;
        cpu->AddLocks(slot1);
    }
}

void IssueExec::Produce() {
    if (!slot1.IsNop()) {
        // cpu->AddLocks(slot1);

        if (slot1.IsMemAccess())
            cpu->queues.preMemALU.entries.push_back(BufferEntry::PreMemALU{ std::move(slot1) });
        else
            cpu->queues.preALU.entries.push_back(BufferEntry::PreALU{ std::move(slot1) });

        if (!slot2.IsNop()) {
            // cpu->AddLocks(slot2);

            if (slot2.IsMemAccess())
                cpu->queues.preMemALU.entries.push_back(BufferEntry::PreMemALU{ std::move(slot2) });
            else
                cpu->queues.preALU.entries.push_back(BufferEntry::PreALU{ std::move(slot2) });
        }
    }
        
}

void ALUExec::Consume() {
    if (!cpu->queues.preALU.entries.is_empty())
        slot = cpu->queues.preALU.entries.pop_front().instruction;
}

void ALUExec::Produce() {
    if (slot.IsNop()) return;
    
    int32_t result = slot.ExecuteResult(*cpu);
    cpu->queues.postALU.entries.push_back(BufferEntry::PostALU{ std::move(slot), result });
}

void MemALUExec::Consume() {
    if (!cpu->queues.preMemALU.entries.is_empty())
        slot = cpu->queues.preMemALU.entries.pop_front().instruction;
}

void MemALUExec::Produce() {
    if (slot.IsNop()) return;

    uint32_t memAddr = (uint32_t) slot.ExecuteResult(*cpu);

    cpu->queues.preMem.entries.push_back(BufferEntry::PreMem{ std::move(slot), memAddr });    
}

void MemExec::Consume() {
    if (!cpu->queues.preMem.entries.is_empty())
        slot = cpu->queues.preMem.entries.pop_front();
}

void MemExec::Produce() {
    if (!slot.has_value()) return;

    if (slot->instruction.IsStore()) {
        cpu->Mem(slot->address) = cpu->Reg(slot->instruction.GetFormat<ISA::IType>().rt);
    } else if (slot->instruction.IsLoad()) {
        int32_t result = cpu->Mem(slot->address);
        cpu->queues.postMem.entries.push_back(BufferEntry::PostMem{ slot->instruction, result });
    }

    slot.reset();
}

void WritebackExec::Consume() {
    if (!cpu->queues.postALU.entries.is_empty())
        slotALU = cpu->queues.postALU.entries.pop_front();

    if (!cpu->queues.postMem.entries.is_empty())
        slotMem = cpu->queues.postMem.entries.pop_front();
}

void WritebackExec::Produce() {
    // We can assume affects has a value because it will not reach WB if it does not
    if (slotALU.has_value()) {
        const auto [deps, affects] = slotALU->instruction.GetDeps();
        cpu->Reg(affects.value()) = slotALU->result;
        
        cpu->RemoveLocks(slotALU->instruction);
        slotALU.reset();
    }

    if (slotMem.has_value()) {
        const auto [deps, affects] = slotMem->instruction.GetDeps();
        cpu->Reg(affects.value()) = slotMem->result;

        cpu->RemoveLocks(slotMem->instruction);
        slotMem.reset();
    }
}

//!--------------------------------------------------------------------------------------------------------
//! Microcode.cpp
//!--------------------------------------------------------------------------------------------------------

#define EX_FUNC(X) void Executors::X(CPU& cpu, const Instruction& in,  const std::function<void(int32_t)>& resultCB)
#define PR_FUNC(X) std::string Printers::X(const Instruction& in)

#define EX_NULL(X) EX_FUNC(X) { printf(#X " executor undefined"); }
#define PR_NULL(X) PR_FUNC(X) { return std::string(#X " printer undefined"); }

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
    // cpu.Mem(cpu.Reg(format.rs) + format.imm) = cpu.Reg(format.rt);
    resultCB((int32_t) (cpu.Reg(format.rs) + format.imm)); // Calculate address
}

EX_FUNC(LW) {
    const auto& format = in.GetFormat<ISA::IType>();
    // cpu.Reg(format.rt) = cpu.Mem(cpu.Reg(format.rs) + format.imm);
    resultCB((int32_t) (cpu.Reg(format.rs) + format.imm)); // Calculate address
}

EX_FUNC(SLL) {
    const auto& format = in.GetFormat<ISA::RType>();
    resultCB(cpu.Reg(format.rt) << format.sa);
}

EX_FUNC(SRL) {
    const auto& format = in.GetFormat<ISA::RType>();
    // Cast to unsigned int to do guarantee logical shift, then shift by shamt
    resultCB(static_cast<int32_t>(static_cast<uint32_t>(cpu.Reg(format.rt)) >> format.sa));
}

EX_FUNC(SRA) {
    const auto& format = in.GetFormat<ISA::RType>();
    resultCB(cpu.Reg(format.rt) >> format.sa);
}

EX_FUNC(NOP) { }

EX_FUNC(BRK) { }

// Category 2
EX_FUNC(ADD) {
    const auto& format = in.GetFormat<ISA::RType>();
    // cpu.Reg(format.rd) = cpu.Reg(format.rs) + cpu.Reg(format.rt);
    resultCB(cpu.Reg(format.rs) + cpu.Reg(format.rt));
}

EX_FUNC(SUB) {
    const auto& format = in.GetFormat<ISA::RType>();
    resultCB(cpu.Reg(format.rs) - cpu.Reg(format.rt));
}

EX_FUNC(MUL) {
    const auto& format = in.GetFormat<ISA::RType>();
    resultCB(cpu.Reg(format.rs) * cpu.Reg(format.rt));
}

EX_FUNC(AND) {
    const auto& format = in.GetFormat<ISA::RType>();
    resultCB(cpu.Reg(format.rs) & cpu.Reg(format.rt));
}

EX_FUNC(OR) {
    const auto& format = in.GetFormat<ISA::RType>();
    resultCB(cpu.Reg(format.rs) | cpu.Reg(format.rt));
} 

EX_FUNC(XOR) {
    const auto& format = in.GetFormat<ISA::RType>();
    resultCB(cpu.Reg(format.rs) ^ cpu.Reg(format.rt));
}

EX_FUNC(NOR) {
    const auto& format = in.GetFormat<ISA::RType>();
    resultCB( ~(cpu.Reg(format.rs) | cpu.Reg(format.rt)) );
}

EX_FUNC(SLT) {
    const auto& format = in.GetFormat<ISA::RType>();
    resultCB(cpu.Reg(format.rs) < cpu.Reg(format.rt));
}

EX_FUNC(ADDI) {
    const auto& format = in.GetFormat<ISA::IType>();
    // cpu.Reg(format.rt) = cpu.Reg(format.rs) + format.imm;
    resultCB(cpu.Reg(format.rs) + format.imm);
}

EX_FUNC(ANDI) {
    const auto& format = in.GetFormat<ISA::IType>();
    resultCB(cpu.Reg(format.rs) & static_cast<uint32_t>(format.imm)); // Zero extend immediate
}

EX_FUNC(ORI) {
    const auto& format = in.GetFormat<ISA::IType>();
    resultCB(cpu.Reg(format.rs) | static_cast<uint32_t>(format.imm)); // Zero extend immediate
}

EX_FUNC(XORI) {
    const auto& format = in.GetFormat<ISA::IType>();
    resultCB(cpu.Reg(format.rs) ^ static_cast<uint32_t>(format.imm)); // Zero extend immediate
}

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

//!--------------------------------------------------------------------------------------------------------
//! Disassembler.cpp
//!--------------------------------------------------------------------------------------------------------

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


using namespace SPIMDF;


int main(int argc, const char** argv) {
    CPU cpu(256);
    Disassemble(argv[1], cpu);

    std::ofstream output("simulation.txt", std::ios::binary);

    char buffer[200];
    std::string temp;

    bool hitBreak = false;

    while (true) {
        output << "--------------------\n";
        
        sprintf(buffer, "Cycle %lu:\n\n", cpu.GetCycle());
        output << buffer;

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
    }

    output.close();
}
