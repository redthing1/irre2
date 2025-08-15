#include <catch2/catch_test_macros.hpp>
#include "assembler/disassembler.hpp"
#include "assembler/assembler.hpp"
#include "assembler/object.hpp"
#include "arch/instruction.hpp"
#include "arch/encoding.hpp"
#include <iostream>

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

TEST_CASE("assembler/disassembler end-to-end tests", "[asmdisasm_e2e]") {
  asmr::assembler asm_engine;
  asmr::disassembler disasm;

  SECTION("e2e: simple arithmetic program") {
    std::string source = R"(
        ; simple.asm - Basic instruction test
        %entry: start

        start:
            set r1 42
            set r2 17
            add r3 r1 r2
            mov r4 r3
            not r5 r4
            
            seq r7 r3 59
            set r6 success
            bve r6 r7 1
            hlt

        success:
            set r8 255
            set r9 240
            hlt
    )";

    // assemble to object file
    auto asm_result = asm_engine.assemble(source);
    REQUIRE(asm_result.is_ok());
    auto obj = asm_result.value();

    // verify object file structure
    REQUIRE(obj.code.size() > 0);
    REQUIRE(obj.code.size() % 4 == 0); // instructions are 4-byte aligned

    // disassemble back
    auto disasm_result = disasm.disassemble_object(obj, asmr::disasm_format::basic);
    if (disasm_result.is_err()) {
      FAIL("Disassembly failed: " + std::string(asmr::disasm_error_message(disasm_result.error())));
    }
    REQUIRE(disasm_result.is_ok());

    std::string disassembly = disasm_result.value();

    // verify all instructions are present and correct
    REQUIRE(disassembly.find("set r1 0x002a") != std::string::npos); // 42 in hex
    REQUIRE(disassembly.find("set r2 0x0011") != std::string::npos); // 17 in hex
    REQUIRE(disassembly.find("add r3 r1 r2") != std::string::npos);
    REQUIRE(disassembly.find("mov r4 r3") != std::string::npos);
    REQUIRE(disassembly.find("not r5 r4") != std::string::npos);
    REQUIRE(disassembly.find("seq r7 r3 0x3b") != std::string::npos); // 59 in hex
    REQUIRE(disassembly.find("set r6 0x0024") != std::string::npos);  // success label address
    REQUIRE(disassembly.find("bve r6 r7 0x01") != std::string::npos);
    REQUIRE(disassembly.find("hlt") != std::string::npos);
    REQUIRE(disassembly.find("set r8 0x00ff") != std::string::npos); // 255 in hex
    REQUIRE(disassembly.find("set r9 0x00f0") != std::string::npos); // 240 in hex
  }

  SECTION("e2e: write and read object file to disk") {
    std::string source = R"(
        %entry: main
        
        main:
            set r0 100
            set r1 200
            add r2 r0 r1
            hlt
    )";

    // assemble
    auto asm_result = asm_engine.assemble(source);
    REQUIRE(asm_result.is_ok());
    auto obj = asm_result.value();

    // write to binary format
    auto binary_data = obj.to_binary();
    REQUIRE(binary_data.size() >= 24); // at least header size

    // verify magic bytes (should be "RGVM")
    REQUIRE(binary_data[0] == 'R');
    REQUIRE(binary_data[1] == 'G');
    REQUIRE(binary_data[2] == 'V');
    REQUIRE(binary_data[3] == 'M');

    // read back from binary
    auto obj_result = asmr::object_file::from_binary(binary_data);
    REQUIRE(obj_result.is_ok());
    auto obj_loaded = obj_result.value();

    // verify they match
    REQUIRE(obj_loaded.entry_offset == obj.entry_offset);
    REQUIRE(obj_loaded.code == obj.code);
    REQUIRE(obj_loaded.data == obj.data);

    // disassemble the loaded object
    auto disasm_result = disasm.disassemble_object(obj_loaded);
    REQUIRE(disasm_result.is_ok());

    std::string disassembly = disasm_result.value();
    REQUIRE(disassembly.find("set r0 0x0064") != std::string::npos); // 100 in hex
    REQUIRE(disassembly.find("set r1 0x00c8") != std::string::npos); // 200 in hex
    REQUIRE(disassembly.find("add r2 r0 r1") != std::string::npos);
    REQUIRE(disassembly.find("hlt") != std::string::npos);
  }

  SECTION("e2e: complex control flow program") {
    std::string source = R"(
        %entry: main

        main:
            set r1 5
            set r10 factorial
            cal r10
            set r3 result_area
            stw r2 r3 0
            hlt

        factorial:
            mov r20 lr
            set r3 1
            tcu r4 r1 r3
            bif r4 base_case 2
            
            mov r21 r1
            sbi r1 r1 1
            set r10 factorial
            cal r10
            mul r2 r21 r2
            jmi cleanup
            
        base_case:
            set r2 1
            
        cleanup:
            mov lr r20
            ret

        result_area:
            %d 0
    )";

    // assemble
    auto asm_result = asm_engine.assemble(source);
    if (asm_result.is_err()) {
      auto error = asm_result.error();
      FAIL(
          "Assembly failed: " + error.message + " at line " + std::to_string(error.line) + ", column " +
          std::to_string(error.column)
      );
    }
    REQUIRE(asm_result.is_ok());
    auto obj = asm_result.value();

    // disassemble
    auto disasm_result = disasm.disassemble_object(obj);
    REQUIRE(disasm_result.is_ok());

    std::string disassembly = disasm_result.value();

    // verify key control flow instructions
    REQUIRE(disassembly.find("cal") != std::string::npos); // function call
    REQUIRE(disassembly.find("bve") != std::string::npos); // conditional branch
    REQUIRE(disassembly.find("jmi") != std::string::npos); // unconditional jump
    REQUIRE(disassembly.find("ret") != std::string::npos); // return
    REQUIRE(disassembly.find("stw") != std::string::npos); // store word
    REQUIRE(disassembly.find("mul") != std::string::npos); // multiply
    REQUIRE(disassembly.find("tcu") != std::string::npos); // unsigned compare
  }

  SECTION("e2e: test with actual file I/O like CLI tools") {
    // this test mimics what the CLI tools do
    std::string source = R"(
        %entry: test_main
        
        test_main:
            set r0 42
            set r1 0
            seq r2 r0 42
            bif r2 end 1
            set r1 1
        end:
            hlt
    )";

    // assemble
    auto asm_result = asm_engine.assemble(source);
    if (asm_result.is_err()) {
      auto error = asm_result.error();
      FAIL(
          "Assembly failed: " + error.message + " at line " + std::to_string(error.line) + ", column " +
          std::to_string(error.column)
      );
    }
    REQUIRE(asm_result.is_ok());
    auto obj = asm_result.value();

    // convert to binary (what assembler CLI writes)
    auto binary_data = obj.to_binary();

    // simulate reading from file (what disassembler CLI reads)
    auto obj_from_binary = asmr::object_file::from_binary(binary_data);
    REQUIRE(obj_from_binary.is_ok());

    // disassemble (what disassembler CLI does)
    auto disasm_result = disasm.disassemble_object(obj_from_binary.value(), asmr::disasm_format::annotated);
    REQUIRE(disasm_result.is_ok());

    std::string output = disasm_result.value();

    // verify annotated format includes headers
    REQUIRE(output.find("irre object file disassembly") != std::string::npos);
    REQUIRE(output.find("entry point:") != std::string::npos);
    REQUIRE(output.find("code size:") != std::string::npos);

    // verify instructions
    REQUIRE(output.find("set r0 0x002a") != std::string::npos);
    REQUIRE(output.find("set r1 0x0000") != std::string::npos);
    REQUIRE(output.find("seq r2 r0 0x2a") != std::string::npos);
    REQUIRE(output.find("bve ad r2") != std::string::npos); // from bif pseudo-instruction
    REQUIRE(output.find("hlt") != std::string::npos);
  }
}