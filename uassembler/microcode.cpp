#include <stdlib.h>
#include <stdio.h>

#include <bitset>
#include <bit>
#include <stdint.h>

#include <iostream>
#include <climits>


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;



template <typename T>
T swap_endian_8(T u)
{
    //check
    static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

    union
    {
        T u;
        unsigned char u8;
    } source, dest;

    source.u = u;

    for (size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];

    return dest.u;
}





enum class operand_sel_t {
    none,
    Op1,
    Op2,
    Op3
};


//Op1 A, Op2 B
//Op1 W&A, Op2 B
//Op1 W, Op2 A, Op3 B




struct ucode_t {

    enum class seqsctl_t
    {
        END,
        NEXT,
        IFOP12W
    } seqsctl : 2;

    //CONTROL
    u32 pc_in : 1; //PC in from addrbus
    u32 fetch : 1; //THIS MUST BE ON WHILE END IS NEXT SEQUENCE OR THE CPU WILL FREEZE
    enum class addrbus_out_t {
        NONE,
        MAR,
        WBUS,
        PC,
        PCINC,
        TRAP_BASE
    } addrbusctl : 3; //addrbus write control

    u32 pc_out_a: 1; //pc out to A

    //REGISTERS
    enum class regabctl_t
    {
        NONE,
        Op2A_Op3B, //001 // Op1, Op2, Op3 (add dr, sr1, sr2) / (add dr, sr1, imm5/imm4e)
        Op2B, //010 //Op1, Op2, NOTHING (not dr, sr) / (not dr, imm8/7e)
        Op1B,//011 //Op1 (jmp op1) / (jmp imm11/10e)
        Op12_DIRECT_A //100
        //PCA, // //PC+Imm12
    } regabctl : 3;

    enum class regwctl_t
    {
        none,
        Op1,
        R7,
        R6_LINK
    } regwctl : 2;

    u32 _pad21 : 1;

    u32 immsel : 1; //immediate bit enable
    enum class immctl_t {
        disable,
        Imm5,
        Imm11,
        Imm8
    } immctl : 2; //immediate mode

    u32 _pad2 : 1;

    //ALU
    u32 alu_latch_a:1; //latch onto A bus
    u32 alu_latch_b:1; //latch onto B bus

    enum class alu_opsel_t {
        AND,
        OR,
        XOR,
        ADD, 
        SUB,
        IMUL,
        LSR,
        ASR,
        ASL, 
        NOT,
        CMP = 0xF
    } alu_opsel : 4; //select ALU operation

    enum class alu_writectl_t {
        disable,
        RESULT,
        EFLAGS
    } alu_writectl:2; //what to write on the write bus when operation is complete

    u32 alu_eflags_en: 1; // enable writing to flags when op is complete

    u32 _pad3: 2;

    //MEMORY 
    u32 dbus_b: 1;
    u32 dbus_rw: 1;
    u32 dbus_in_w: 1;
    u32 mar_in: 1;
    u32 mdr_in: 1;

    u32 __pad4:2;
} ;

constexpr size_t ucodebytesz = sizeof(ucode_t);

constexpr size_t ucodebitsz = 36;
constexpr size_t ucodefullbit = ucodebitsz + 6;

ucode_t INIT{};

ucode_t BR{};
ucode_t JMP{};

ucode_t ADD{};
ucode_t SUB{};
ucode_t MUL{};

ucode_t AND{};
ucode_t OR{};
ucode_t XOR{};
ucode_t NOT{};

ucode_t LSR{};
ucode_t ASR{};
ucode_t ASL{};

ucode_t MOV{};
ucode_t LD[3]{};
ucode_t LDI{};
ucode_t ST[3]{};
ucode_t STI{};

ucode_t CALL{};
ucode_t RET{};

ucode_t PUSH{};
ucode_t POP{};

ucode_t arr[0b11111] = {};

//FILE* outfile = fopen("ucode.bin", "w+");
#include <fstream>
std::fstream UCODE_OUT("ucode.txt", std::ios::trunc | std::ios::out);
void write_u(ucode_t* ucode, int opcode, size_t sz = 1)
{
    auto opcodeb = std::bitset<5>{opcode}; 

    std::string outbuf{};

    for(int i = 0; i < sz; ++i)
    {
        std::bitset<36> g = std::bit_cast<uint64_t>(ucode[i]);

        auto bitstr = g.to_string();

        std::bitset<2> seqn = i;
        std::string str{};
        str.append(opcodeb.to_string() + seqn.to_string() + " ");

        str.append(bitstr);

        outbuf.append(str + "\n");
    }

    UCODE_OUT << outbuf;
}

int main()
{

    INIT.seqsctl = ucode_t::seqsctl_t::END;
    INIT.fetch = 1;

    write_u(&INIT, 0b00000);

    ADD.seqsctl = ucode_t::seqsctl_t::END;
    ADD.fetch = 1;
    ADD.regabctl = ucode_t::regabctl_t::Op2A_Op3B;

    ADD.regwctl = ucode_t::regwctl_t::Op1;
    ADD.immctl = ucode_t::immctl_t::Imm5;
    ADD.immsel = 1;

    ADD.alu_latch_a = 1;
    ADD.alu_latch_b = 1;

    ADD.alu_opsel = ucode_t::alu_opsel_t::ADD;
    ADD.alu_writectl = ucode_t::alu_writectl_t::RESULT;

    write_u(&ADD, 0b10000);

    SUB = ADD;
    SUB.alu_opsel = ucode_t::alu_opsel_t::SUB;
    write_u(&SUB, 0b10001);

    MUL = ADD;
    MUL.alu_opsel = ucode_t::alu_opsel_t::IMUL;
    write_u(&MUL, 0b10010);

    AND = ADD;
    AND.alu_opsel = ucode_t::alu_opsel_t::AND;
    write_u(&AND, 0b10011);

    OR = ADD;
    OR.alu_opsel = ucode_t::alu_opsel_t::OR;
    write_u(&OR, 0b10100);

    XOR = ADD;
    XOR.alu_opsel = ucode_t::alu_opsel_t::OR;
    write_u(&XOR, 0b10101);

    LSR = ADD;
    LSR.alu_opsel = ucode_t::alu_opsel_t::LSR;
    write_u(&LSR, 0b100110);

    ASR = ADD;
    ASR.alu_opsel = ucode_t::alu_opsel_t::ASR;
    write_u(&ASR, 0b10111);

    ASL = ADD;
    ASL.alu_opsel = ucode_t::alu_opsel_t::ASR;
    write_u(&ASL, 0b11000);

    MOV.seqsctl = ucode_t::seqsctl_t::END;
    MOV.fetch = 1;
    MOV.regabctl = ucode_t::regabctl_t::Op2B;

    MOV.regwctl = ucode_t::regwctl_t::Op1;
    MOV.immctl = ucode_t::immctl_t::Imm8;
    MOV.immsel = 1;

    MOV.alu_latch_a = 1;
    MOV.alu_latch_b = 1;

    MOV.alu_opsel = ucode_t::alu_opsel_t::ADD;
    MOV.alu_writectl = ucode_t::alu_writectl_t::RESULT;
    write_u(&MOV, 0b01000);



    ucode_t& LDa = LD[0];
    LDa.seqsctl = ucode_t::seqsctl_t::NEXT;
    LDa.addrbusctl = ucode_t::addrbus_out_t::WBUS;
    LDa.fetch = 0;
    LDa.regabctl = ucode_t::regabctl_t::Op2B;

    LDa.regwctl = ucode_t::regwctl_t::none;
    LDa.immctl = ucode_t::immctl_t::Imm8;
    LDa.immsel = 1;

    LDa.alu_latch_a = 1;
    LDa.alu_latch_b = 1;

    LDa.alu_opsel = ucode_t::alu_opsel_t::ADD;
    LDa.alu_writectl = ucode_t::alu_writectl_t::RESULT;
    LDa.mar_in = 1;
    LDa.mdr_in = 1;

    ucode_t& LDb = LD[1];
    LDb.seqsctl = ucode_t::seqsctl_t::END;
    LDb.addrbusctl = ucode_t::addrbus_out_t::NONE;
    LDb.fetch = 1;

    LDb.regabctl = ucode_t::regabctl_t::NONE;
    LDb.regwctl = ucode_t::regwctl_t::Op1;
    LDb.immsel = 0;

    LDb.alu_latch_a = 0;
    LDb.alu_latch_b = 1;

    LDb.alu_opsel = ucode_t::alu_opsel_t::ADD;
    LDb.alu_writectl = ucode_t::alu_writectl_t::RESULT;

    LDb.dbus_b = 1;

    ucode_t& STa = ST[0];
    STa.seqsctl = ucode_t::seqsctl_t::NEXT;
    STa.addrbusctl = ucode_t::addrbus_out_t::WBUS;
    STa.fetch = 0;
    STa.regabctl = ucode_t::regabctl_t::Op2B;

    STa.regwctl = ucode_t::regwctl_t::none;
    STa.immctl = ucode_t::immctl_t::Imm8;
    STa.immsel = 1;

    STa.alu_latch_a = 1;
    STa.alu_latch_b = 1;

    STa.alu_opsel = ucode_t::alu_opsel_t::ADD;
    STa.alu_writectl = ucode_t::alu_writectl_t::RESULT;
    STa.mar_in = 1;

    ucode_t& STb = ST[1];
    STb.seqsctl = ucode_t::seqsctl_t::END;
    STb.addrbusctl = ucode_t::addrbus_out_t::MAR;
    STa.mar_in = 1;
    STb.fetch = 0;

    STb.regabctl = ucode_t::regabctl_t::NONE;
    STb.regwctl = ucode_t::regwctl_t::none;
    STb.immsel = 0;

    STb.alu_latch_a = 0;
    STb.alu_latch_b = 0;
    STb.alu_writectl = ucode_t::alu_writectl_t::disable;

    STb.dbus_rw = 1;
    STb.dbus_in_w = 1;

    write_u(LD, 0b01001, 2);

    UCODE_OUT.seekg(0);

    UCODE_OUT.close();

    std::cout << UCODE_OUT.rdbuf();

    return 0;
}