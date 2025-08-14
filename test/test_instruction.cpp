#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "irre.hpp"

using namespace irre;

TEST_CASE("Instruction construction helpers") {
  SECTION("make::nop()") {
    auto inst = make::nop();
    REQUIRE(get_opcode(inst) == opcode::nop);
    REQUIRE(get_format(inst) == format::op);
  }

  SECTION("make::add()") {
    auto inst = make::add(reg::r1, reg::r2, reg::r3);
    REQUIRE(get_opcode(inst) == opcode::add);
    REQUIRE(get_format(inst) == format::op_reg_reg_reg);

    // verify the instruction contains the right registers
    auto add_inst = std::get<inst_op_reg_reg_reg>(inst);
    REQUIRE(add_inst.op == opcode::add);
    REQUIRE(add_inst.a == reg::r1);
    REQUIRE(add_inst.b == reg::r2);
    REQUIRE(add_inst.c == reg::r3);
  }

  SECTION("make::set()") {
    auto inst = make::set(reg::r5, 0x1234);
    REQUIRE(get_opcode(inst) == opcode::set);
    REQUIRE(get_format(inst) == format::op_reg_imm16);

    auto set_inst = std::get<inst_op_reg_imm16>(inst);
    REQUIRE(set_inst.op == opcode::set);
    REQUIRE(set_inst.a == reg::r5);
    REQUIRE(set_inst.imm == 0x1234);
  }
}

TEST_CASE("Instruction variant handling") {
  SECTION("std::variant holds correct types") {
    // test that we can construct and hold each instruction type
    instruction inst1 = inst_op{opcode::nop};
    instruction inst2 = inst_op_reg{opcode::jmp, reg::r0};
    instruction inst3 = inst_op_imm24{opcode::jmi, 0x123456};
    instruction inst4 = inst_op_reg_imm16{opcode::set, reg::r1, 0x1234};
    instruction inst5 = inst_op_reg_reg{opcode::mov, reg::r1, reg::r2};
    instruction inst6 = inst_op_reg_reg_imm8{opcode::ldw, reg::r1, reg::r2, 0x10};
    instruction inst7 = inst_op_reg_imm8x2{opcode::sia, reg::r1, 0x10, 0x04};
    instruction inst8 = inst_op_reg_reg_reg{opcode::add, reg::r1, reg::r2, reg::r3};

    // test that we can extract opcodes from all variants
    REQUIRE(get_opcode(inst1) == opcode::nop);
    REQUIRE(get_opcode(inst2) == opcode::jmp);
    REQUIRE(get_opcode(inst3) == opcode::jmi);
    REQUIRE(get_opcode(inst4) == opcode::set);
    REQUIRE(get_opcode(inst5) == opcode::mov);
    REQUIRE(get_opcode(inst6) == opcode::ldw);
    REQUIRE(get_opcode(inst7) == opcode::sia);
    REQUIRE(get_opcode(inst8) == opcode::add);
  }
}

TEST_CASE("Opcode metadata") {
  SECTION("get_opcode_info returns correct info") {
    auto info_nop = get_opcode_info(opcode::nop);
    REQUIRE(std::string(info_nop.mnemonic) == "nop");
    REQUIRE(info_nop.fmt == format::op);

    auto info_add = get_opcode_info(opcode::add);
    REQUIRE(std::string(info_add.mnemonic) == "add");
    REQUIRE(info_add.fmt == format::op_reg_reg_reg);

    auto info_set = get_opcode_info(opcode::set);
    REQUIRE(std::string(info_set.mnemonic) == "set");
    REQUIRE(info_set.fmt == format::op_reg_imm16);
  }

  SECTION("get_format helper works") {
    REQUIRE(get_format(opcode::nop) == format::op);
    REQUIRE(get_format(opcode::jmp) == format::op_reg);
    REQUIRE(get_format(opcode::jmi) == format::op_imm24);
    REQUIRE(get_format(opcode::set) == format::op_reg_imm16);
    REQUIRE(get_format(opcode::mov) == format::op_reg_reg);
    REQUIRE(get_format(opcode::ldw) == format::op_reg_reg_imm8);
    REQUIRE(get_format(opcode::sia) == format::op_reg_imm8x2);
    REQUIRE(get_format(opcode::add) == format::op_reg_reg_reg);
  }

  SECTION("get_mnemonic helper works") {
    REQUIRE(std::string(get_mnemonic(opcode::nop)) == "nop");
    REQUIRE(std::string(get_mnemonic(opcode::add)) == "add");
    REQUIRE(std::string(get_mnemonic(opcode::sub)) == "sub");
    REQUIRE(std::string(get_mnemonic(opcode::jmp)) == "jmp");
    REQUIRE(std::string(get_mnemonic(opcode::hlt)) == "hlt");
  }
}

TEST_CASE("Individual instruction format decoding") {
  SECTION("inst_op decode/encode") {
    word w = 0x00000000; // nop
    auto inst = inst_op::decode(w);
    REQUIRE(inst.op == opcode::nop);
    REQUIRE(inst.encode() == w);

    word w2 = 0xff000000; // hlt
    auto inst2 = inst_op::decode(w2);
    REQUIRE(inst2.op == opcode::hlt);
    REQUIRE(inst2.encode() == w2);
  }

  SECTION("inst_op_reg decode/encode") {
    word w = 0x21050000; // jmp r5
    auto inst = inst_op_reg::decode(w);
    REQUIRE(inst.op == opcode::jmp);
    REQUIRE(inst.a == reg::r5);
    REQUIRE(inst.encode() == w);
  }

  SECTION("inst_op_imm24 decode/encode") {
    word w = 0x20123456; // jmi 0x123456
    auto inst = inst_op_imm24::decode(w);
    REQUIRE(inst.op == opcode::jmi);
    REQUIRE(inst.addr == 0x123456);
    REQUIRE(inst.encode() == w);

    // test 24-bit masking
    word w2 = 0x20ffffff; // jmi 0xffffff (max 24-bit)
    auto inst2 = inst_op_imm24::decode(w2);
    REQUIRE(inst2.addr == 0xffffff);
    REQUIRE(inst2.encode() == w2);
  }

  SECTION("inst_op_reg_imm16 decode/encode") {
    word w = 0x0b051234; // set r5, 0x1234
    auto inst = inst_op_reg_imm16::decode(w);
    REQUIRE(inst.op == opcode::set);
    REQUIRE(inst.a == reg::r5);
    REQUIRE(inst.imm == 0x1234);
    REQUIRE(inst.encode() == w);
  }

  SECTION("inst_op_reg_reg decode/encode") {
    word w = 0x0c050a00; // mov r5, r10
    auto inst = inst_op_reg_reg::decode(w);
    REQUIRE(inst.op == opcode::mov);
    REQUIRE(inst.a == reg::r5);
    REQUIRE(inst.b == reg::r10);
    REQUIRE(inst.encode() == w);
  }

  SECTION("inst_op_reg_reg_imm8 decode/encode") {
    word w = 0x0d050a10; // ldw r5, r10, 0x10
    auto inst = inst_op_reg_reg_imm8::decode(w);
    REQUIRE(inst.op == opcode::ldw);
    REQUIRE(inst.a == reg::r5);
    REQUIRE(inst.b == reg::r10);
    REQUIRE(inst.offset == 0x10);
    REQUIRE(inst.encode() == w);
  }

  SECTION("inst_op_reg_imm8x2 decode/encode") {
    word w = 0x40051004; // sia r5, 0x10, 0x04
    auto inst = inst_op_reg_imm8x2::decode(w);
    REQUIRE(inst.op == opcode::sia);
    REQUIRE(inst.a == reg::r5);
    REQUIRE(inst.v0 == 0x10);
    REQUIRE(inst.v1 == 0x04);
    REQUIRE(inst.encode() == w);
  }

  SECTION("inst_op_reg_reg_reg decode/encode") {
    word w = 0x01050a0f; // add r5, r10, r15
    auto inst = inst_op_reg_reg_reg::decode(w);
    REQUIRE(inst.op == opcode::add);
    REQUIRE(inst.a == reg::r5);
    REQUIRE(inst.b == reg::r10);
    REQUIRE(inst.c == reg::r15);
    REQUIRE(inst.encode() == w);
  }
}

TEST_CASE("Register name utility") {
  SECTION("reg_name returns correct names") {
    REQUIRE(std::string(reg_name(reg::r0)) == "r0");
    REQUIRE(std::string(reg_name(reg::r15)) == "r15");
    REQUIRE(std::string(reg_name(reg::r31)) == "r31");
    REQUIRE(std::string(reg_name(reg::pc)) == "pc");
    REQUIRE(std::string(reg_name(reg::lr)) == "lr");
    REQUIRE(std::string(reg_name(reg::ad)) == "ad");
    REQUIRE(std::string(reg_name(reg::at)) == "at");
    REQUIRE(std::string(reg_name(reg::sp)) == "sp");
  }

  SECTION("register classification helpers") {
    // test GPR classification
    REQUIRE(is_gpr(reg::r0) == true);
    REQUIRE(is_gpr(reg::r31) == true);
    REQUIRE(is_gpr(reg::pc) == false);
    REQUIRE(is_gpr(reg::sp) == false);

    // test special register classification
    REQUIRE(is_special(reg::r0) == false);
    REQUIRE(is_special(reg::r31) == false);
    REQUIRE(is_special(reg::pc) == true);
    REQUIRE(is_special(reg::lr) == true);
    REQUIRE(is_special(reg::sp) == true);
  }
}

TEST_CASE("All convenience constructors") {
  SECTION("Basic operations") {
    // test that all convenience constructors work and produce correct opcodes
    REQUIRE(get_opcode(make::nop()) == opcode::nop);
    REQUIRE(get_opcode(make::hlt()) == opcode::hlt);
    REQUIRE(get_opcode(make::ret()) == opcode::ret);
  }

  SECTION("Arithmetic operations") {
    REQUIRE(get_opcode(make::add(reg::r1, reg::r2, reg::r3)) == opcode::add);
    REQUIRE(get_opcode(make::sub(reg::r1, reg::r2, reg::r3)) == opcode::sub);
    REQUIRE(get_opcode(make::mul(reg::r1, reg::r2, reg::r3)) == opcode::mul);
    REQUIRE(get_opcode(make::div(reg::r1, reg::r2, reg::r3)) == opcode::div);
    REQUIRE(get_opcode(make::mod(reg::r1, reg::r2, reg::r3)) == opcode::mod);
  }

  SECTION("Data movement") {
    REQUIRE(get_opcode(make::mov(reg::r1, reg::r2)) == opcode::mov);
    REQUIRE(get_opcode(make::set(reg::r1, 0x1234)) == opcode::set);
  }

  SECTION("Memory operations") {
    REQUIRE(get_opcode(make::ldw(reg::r1, reg::r2, 0x10)) == opcode::ldw);
    REQUIRE(get_opcode(make::stw(reg::r1, reg::r2, 0x10)) == opcode::stw);
    REQUIRE(get_opcode(make::ldb(reg::r1, reg::r2, 0x10)) == opcode::ldb);
    REQUIRE(get_opcode(make::stb(reg::r1, reg::r2, 0x10)) == opcode::stb);
  }

  SECTION("Control flow") {
    REQUIRE(get_opcode(make::jmp(reg::r1)) == opcode::jmp);
    REQUIRE(get_opcode(make::jmi(0x123456)) == opcode::jmi);
    REQUIRE(get_opcode(make::cal(reg::r1)) == opcode::cal);
    REQUIRE(get_opcode(make::bve(reg::r1, reg::r2, 0x05)) == opcode::bve);
    REQUIRE(get_opcode(make::bvn(reg::r1, reg::r2, 0x05)) == opcode::bvn);
  }

  SECTION("System operations") {
    REQUIRE(get_opcode(make::int_(0x123456)) == opcode::int_);
    REQUIRE(get_opcode(make::snd(reg::r1, reg::r2, reg::r3)) == opcode::snd);
  }
}