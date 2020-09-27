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

#pragma once

#include <array>
#include <functional>
#include <tuple>
#include <variant>
#include <type_traits>
#include <cstdint>
#include <cmath>
#include <string>

namespace SPIMDF {
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

        struct DecodeTag { };

        inline constexpr uint64_t Var = static_cast<uint64_t>(-1);
        
        template<auto T>
        constexpr bool IsVar() {
            return T == Var;
        }

        struct FieldRep {
            template<typename FIter, typename NIter>
            // requires requires (FIter fCur, NIter nCur) {
            //     { *fCur + 1 } -> std::integral;
            //     { *nCur } -> std::same_as<const char* const&>;
            // }
            static void Print(FIter fCur, const FIter& fEnd, NIter nCur) {
                for (; fCur != fEnd; fCur++) {
                    printf("%s: %u\t", *nCur, *fCur);

                    nCur++;
                }

                printf("\n");
            }
        };

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
                FieldRep::Print(fields.cbegin(), fields.cend(), names.cbegin());
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
        };

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

                FieldRep::Print(fieldsArray.cbegin(), fieldsArray.cend(), names.cbegin());
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
                FieldRep::Print(fields.cbegin(), fields.cend(), names.cbegin());
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
    }
    
}