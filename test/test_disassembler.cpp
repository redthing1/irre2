#include <catch2/catch_test_macros.hpp>
#include "assembler/disassembler.hpp"
#include "assembler/assembler.hpp"
#include "assembler/object.hpp"
#include "arch/instruction.hpp"
#include "arch/encoding.hpp"

namespace asmr = irre::assembler;
using irre::byte;

TEST_CASE("disassembler basic functionality", "[disassembler]") {
  asmr::disassembler disasm;

  SECTION("disassemble single instruction - basic format") {
    auto inst = irre::make::nop();
    auto result = disasm.disassemble_instruction(inst);

    REQUIRE(result.is_ok());
    REQUIRE(result.value() == "0x0000: 00000000  nop");
  }

  SECTION("disassemble single instruction - no annotations") {
    asmr::disasm_options opts;
    opts.show_addresses = false;
    opts.show_hex_bytes = false;
    disasm.set_options(opts);

    auto inst = irre::make::add(irre::reg::r0, irre::reg::r1, irre::reg::r2);
    auto result = disasm.disassemble_instruction(inst);

    REQUIRE(result.is_ok());
    REQUIRE(result.value() == "add r0 r1 r2");
  }

  SECTION("disassemble instruction with address") {
    auto inst = irre::make::set(irre::reg::r0, 42);
    auto result = disasm.disassemble_instruction(inst, 0x1000);

    REQUIRE(result.is_ok());
    std::string expected = "0x1000: 0b00002a  set r0 0x002a";
    REQUIRE(result.value() == expected);
  }
}

TEST_CASE("disassembler raw bytes", "[disassembler]") {
  asmr::disassembler disasm;

  SECTION("disassemble simple instruction sequence") {
    // create some test instructions
    std::vector<irre::instruction> instructions = {
        irre::make::nop(), irre::make::add(irre::reg::r0, irre::reg::r1, irre::reg::r2), irre::make::hlt()
    };

    // encode to bytes
    std::vector<byte> bytes = irre::byte_utils::encode_sequence(instructions.begin(), instructions.end());

    auto result = disasm.disassemble_bytes(bytes, asmr::disasm_format::annotated);
    REQUIRE(result.is_ok());

    std::string output = result.value();

    // check that it contains expected instructions
    REQUIRE(output.find("nop") != std::string::npos);
    REQUIRE(output.find("add r0 r1 r2") != std::string::npos);
    REQUIRE(output.find("hlt") != std::string::npos);

    // check addresses
    REQUIRE(output.find("0x0000:") != std::string::npos);
    REQUIRE(output.find("0x0004:") != std::string::npos);
    REQUIRE(output.find("0x0008:") != std::string::npos);
  }

  SECTION("disassemble with custom base address") {
    asmr::disasm_options opts;
    opts.base_address = 0x2000;
    disasm.set_options(opts);

    auto inst = irre::make::nop();
    auto bytes = irre::codec::encode_bytes(inst);
    std::vector<byte> byte_vec(bytes.begin(), bytes.end());

    auto result = disasm.disassemble_bytes(byte_vec);
    REQUIRE(result.is_ok());
    REQUIRE(result.value().find("0x2000:") != std::string::npos);
  }

  SECTION("empty bytes") {
    std::vector<byte> empty_bytes;
    auto result = disasm.disassemble_bytes(empty_bytes);

    REQUIRE(result.is_ok());
    REQUIRE(result.value().empty());
  }

  SECTION("invalid byte count") {
    std::vector<byte> bad_bytes = {0x00, 0x00, 0x00}; // 3 bytes, not multiple of 4
    auto result = disasm.disassemble_bytes(bad_bytes);

    REQUIRE(result.is_err());
    REQUIRE(result.error() == asmr::disasm_error::invalid_size);
  }
}

TEST_CASE("disassembler object files", "[disassembler]") {
  asmr::disassembler disasm;

  SECTION("disassemble simple object file") {
    // create object file manually
    asmr::object_file obj;
    obj.entry_offset = 0;

    // add some instructions
    std::vector<irre::instruction> instructions = {
        irre::make::set(irre::reg::r0, 42), irre::make::mov(irre::reg::r1, irre::reg::r0), irre::make::hlt()
    };

    obj.code = irre::byte_utils::encode_sequence(instructions.begin(), instructions.end());

    auto result = disasm.disassemble_object(obj);
    REQUIRE(result.is_ok());

    std::string output = result.value();

    // check for object file header
    REQUIRE(output.find("irre object file disassembly") != std::string::npos);
    REQUIRE(output.find("entry point: 0x0") != std::string::npos);

    // check instructions
    REQUIRE(output.find("set r0 0x002a") != std::string::npos);
    REQUIRE(output.find("mov r1 r0") != std::string::npos);
    REQUIRE(output.find("hlt") != std::string::npos);
  }

  SECTION("disassemble object file with data section") {
    asmr::object_file obj;
    obj.entry_offset = 0;
    obj.code = {0x00, 0x00, 0x00, 0x00}; // nop
    obj.data = {0xde, 0xad, 0xbe, 0xef, 0x12, 0x34};

    auto result = disasm.disassemble_object(obj);
    REQUIRE(result.is_ok());

    std::string output = result.value();

    // check for data section
    REQUIRE(output.find("data section") != std::string::npos);
    REQUIRE(output.find("deadbeef1234") != std::string::npos);
  }

  SECTION("empty object file") {
    asmr::object_file obj; // empty

    auto result = disasm.disassemble_object(obj);
    REQUIRE(result.is_ok());
    REQUIRE(result.value().empty());
  }
}

TEST_CASE("disassembler round-trip testing", "[disassembler][assembler]") {
  // test that assemble → disassemble produces readable output
  asmr::assembler asm_engine;
  asmr::disassembler disasm;

  SECTION("round-trip simple program") {
    std::string source = R"(
        %entry: main
        
        main:
            set r0 42
            set r1 100
            add r2 r0 r1
            hlt
    )";

    // assemble
    auto asm_result = asm_engine.assemble(source);
    REQUIRE(asm_result.is_ok());

    auto obj = asm_result.value();

    // disassemble
    auto disasm_result = disasm.disassemble_object(obj);
    REQUIRE(disasm_result.is_ok());

    std::string disassembly = disasm_result.value();

    // verify instructions are present
    REQUIRE(disassembly.find("set r0 0x002a") != std::string::npos); // 42 in hex
    REQUIRE(disassembly.find("set r1 0x0064") != std::string::npos); // 100 in hex
    REQUIRE(disassembly.find("add r2 r0 r1") != std::string::npos);
    REQUIRE(disassembly.find("hlt") != std::string::npos);
  }

  SECTION("round-trip with pseudo-instructions") {
    std::string source = R"(
        main:
            adi r0 r1 10
            sbi r2 r3 5
            bif r4 main 0
    )";

    auto asm_result = asm_engine.assemble(source);
    REQUIRE(asm_result.is_ok());

    auto obj = asm_result.value();
    auto disasm_result = disasm.disassemble_object(obj);
    REQUIRE(disasm_result.is_ok());

    std::string disassembly = disasm_result.value();

    // pseudo-instructions should be expanded, so we see the real instructions
    REQUIRE(disassembly.find("set at") != std::string::npos);       // from adi/sbi expansion
    REQUIRE(disassembly.find("add r0 r1 at") != std::string::npos); // from adi
    REQUIRE(disassembly.find("sub r2 r3 at") != std::string::npos); // from sbi
    REQUIRE(disassembly.find("set ad") != std::string::npos);       // from bif expansion
    REQUIRE(disassembly.find("bve ad r4") != std::string::npos);    // from bif
  }
}

TEST_CASE("disassembler all instruction formats", "[disassembler]") {
  asmr::disassembler disasm;

  // disable annotations for cleaner testing
  asmr::disasm_options opts;
  opts.show_addresses = false;
  opts.show_hex_bytes = false;
  disasm.set_options(opts);

  SECTION("format op") {
    auto inst = irre::make::nop();
    auto result = disasm.disassemble_instruction(inst);
    REQUIRE(result.is_ok());
    REQUIRE(result.value() == "nop");
  }

  SECTION("format op_reg") {
    auto inst = irre::make::jmp(irre::reg::r5);
    auto result = disasm.disassemble_instruction(inst);
    REQUIRE(result.is_ok());
    REQUIRE(result.value() == "jmp r5");
  }

  SECTION("format op_imm24") {
    auto inst = irre::make::jmi(0x123456);
    auto result = disasm.disassemble_instruction(inst);
    REQUIRE(result.is_ok());
    REQUIRE(result.value() == "jmi 0x123456");
  }

  SECTION("format op_reg_imm16") {
    auto inst = irre::make::set(irre::reg::r10, 0x1234);
    auto result = disasm.disassemble_instruction(inst);
    REQUIRE(result.is_ok());
    REQUIRE(result.value() == "set r10 0x1234");
  }

  SECTION("format op_reg_reg") {
    auto inst = irre::make::mov(irre::reg::r3, irre::reg::r7);
    auto result = disasm.disassemble_instruction(inst);
    REQUIRE(result.is_ok());
    REQUIRE(result.value() == "mov r3 r7");
  }

  SECTION("format op_reg_reg_imm8") {
    auto inst = irre::make::ldw(irre::reg::r1, irre::reg::sp, 8);
    auto result = disasm.disassemble_instruction(inst);
    REQUIRE(result.is_ok());
    REQUIRE(result.value() == "ldw r1 sp 0x08");
  }

  SECTION("format op_reg_imm8x2") {
    auto inst = irre::make::sia(irre::reg::r2, 0x12, 0x34);
    auto result = disasm.disassemble_instruction(inst);
    REQUIRE(result.is_ok());
    REQUIRE(result.value() == "sia r2 0x12 0x34");
  }

  SECTION("format op_reg_reg_reg") {
    auto inst = irre::make::add(irre::reg::r0, irre::reg::r1, irre::reg::r2);
    auto result = disasm.disassemble_instruction(inst);
    REQUIRE(result.is_ok());
    REQUIRE(result.value() == "add r0 r1 r2");
  }
}

TEST_CASE("disassembler configuration options", "[disassembler]") {
  asmr::disassembler disasm;

  SECTION("decimal address format") {
    asmr::disasm_options opts;
    opts.address_format = "decimal";
    disasm.set_options(opts);

    auto inst = irre::make::nop();
    auto result = disasm.disassemble_instruction(inst, 1000);
    REQUIRE(result.is_ok());
    REQUIRE(result.value().find("1000:") != std::string::npos);
  }

  SECTION("show addresses only") {
    asmr::disasm_options opts;
    opts.show_addresses = true;
    opts.show_hex_bytes = false;
    disasm.set_options(opts);

    auto inst = irre::make::nop();
    auto result = disasm.disassemble_instruction(inst);
    REQUIRE(result.is_ok());

    std::string output = result.value();
    REQUIRE(output.find("0x0000:") != std::string::npos);
    REQUIRE(output.find("00000000") == std::string::npos); // no hex bytes
  }

  SECTION("show hex bytes only") {
    asmr::disasm_options opts;
    opts.show_addresses = false;
    opts.show_hex_bytes = true;
    disasm.set_options(opts);

    auto inst = irre::make::nop();
    auto result = disasm.disassemble_instruction(inst);
    REQUIRE(result.is_ok());

    std::string output = result.value();
    REQUIRE(output.find("00000000") != std::string::npos); // hex bytes present
    REQUIRE(output.find("0x0000:") == std::string::npos);  // no address
  }
}

TEST_CASE("disassembler error handling", "[disassembler]") {
  asmr::disassembler disasm;

  SECTION("invalid instruction bytes") {
    // create bytes with invalid opcode
    std::vector<byte> bad_bytes = {0x00, 0x00, 0x00, 0xfe}; // opcode 0xfe doesn't exist

    auto result = disasm.disassemble_bytes(bad_bytes);
    REQUIRE(result.is_err());
    REQUIRE(result.error() == asmr::disasm_error::decode_failed);
  }

  SECTION("invalid register in instruction") {
    // create instruction with invalid register (> 0x24) in correct position
    std::vector<byte> bad_reg_bytes = {0x00, 0x00, 0xff, 0x21}; // jmp with reg 0xff in a1 position

    auto result = disasm.disassemble_bytes(bad_reg_bytes);
    REQUIRE(result.is_err());
    REQUIRE(result.error() == asmr::disasm_error::decode_failed);
  }
}

TEST_CASE("disassembler convenience functions", "[disassembler]") {
  SECTION("convenience disasm::bytes function") {
    auto inst = irre::make::set(irre::reg::r0, 42);
    auto bytes = irre::codec::encode_bytes(inst);
    std::vector<byte> byte_vec(bytes.begin(), bytes.end());

    auto result = asmr::disasm::bytes(byte_vec);
    REQUIRE(result.is_ok());
    REQUIRE(result.value().find("set r0 0x002a") != std::string::npos);
  }

  SECTION("convenience disasm::object function") {
    asmr::object_file obj;
    obj.code = {0x00, 0x00, 0x00, 0x00}; // nop

    auto result = asmr::disasm::object(obj);
    REQUIRE(result.is_ok());
    REQUIRE(result.value().find("nop") != std::string::npos);
  }
}