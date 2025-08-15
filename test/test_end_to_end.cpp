#include <catch2/catch_test_macros.hpp>
#include "assembler/assembler.hpp"
#include "assembler/disassembler.hpp"
#include "assembler/object.hpp"
#include <fstream>

namespace asmr = irre::assembler;

TEST_CASE("End-to-end assembler/disassembler tests", "[e2e]") {
  asmr::assembler asm_engine;
  asmr::disassembler disasm;

  SECTION("simple arithmetic program") {
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

    // Assemble
    auto asm_result = asm_engine.assemble(source);
    REQUIRE(asm_result.is_ok());
    auto obj = asm_result.value();

    // Verify object file structure
    REQUIRE(obj.code.size() > 0);
    REQUIRE(obj.code.size() % 4 == 0);

    // Disassemble
    auto disasm_result = disasm.disassemble_object(obj, asmr::disasm_format::basic);
    REQUIRE(disasm_result.is_ok());

    std::string disassembly = disasm_result.value();

    // Verify key instructions are present
    REQUIRE(disassembly.find("set r1 0x002a") != std::string::npos); // 42
    REQUIRE(disassembly.find("set r2 0x0011") != std::string::npos); // 17
    REQUIRE(disassembly.find("add r3 r1 r2") != std::string::npos);
    REQUIRE(disassembly.find("mov r4 r3") != std::string::npos);
    REQUIRE(disassembly.find("not r5 r4") != std::string::npos);
    REQUIRE(disassembly.find("seq r7 r3 0x3b") != std::string::npos); // 59
    REQUIRE(disassembly.find("hlt") != std::string::npos);
  }

  SECTION("math operations with data directive") {
    std::string source = R"(
            %entry: main

            main:
                set r1 100
                set r2 25
                add r3 r1 r2
                sub r4 r1 r2
                mul r5 r1 r2
                div r6 r1 r2
                mod r7 r1 r2
                
                set r8 result_area
                stw r3 r8 0
                stw r4 r8 4
                stw r5 r8 8
                stw r6 r8 12
                stw r7 r8 16
                hlt

            result_area:
                %d 0 0 0 0 0
        )";

    auto asm_result = asm_engine.assemble(source);
    REQUIRE(asm_result.is_ok());
    auto obj = asm_result.value();

    auto disasm_result = disasm.disassemble_object(obj);
    REQUIRE(disasm_result.is_ok());

    std::string disassembly = disasm_result.value();

    // Verify arithmetic operations
    REQUIRE(disassembly.find("add r3 r1 r2") != std::string::npos);
    REQUIRE(disassembly.find("sub r4 r1 r2") != std::string::npos);
    REQUIRE(disassembly.find("mul r5 r1 r2") != std::string::npos);
    REQUIRE(disassembly.find("div r6 r1 r2") != std::string::npos);
    REQUIRE(disassembly.find("mod r7 r1 r2") != std::string::npos);

    // Verify store operations
    REQUIRE(disassembly.find("stw") != std::string::npos);
  }

  SECTION("control flow with function calls") {
    std::string source = R"(
            %entry: main

            main:
                set r1 5
                set r10 factorial
                cal r10
                set r3 result
                stw r2 r3 0
                hlt

            factorial:
                mov r20 lr
                set r2 1              ; result accumulator
                set r3 1              ; counter
                
            factorial_loop:
                ; check if counter > input
                tcu r4 r3 r1          ; r4 = sign(r3 - r1)
                set ad factorial_done
                bve ad r4 1           ; if r3 > r1, exit loop
                
                ; multiply result by counter
                mul r2 r2 r3
                
                ; increment counter
                adi r3 r3 1
                jmi factorial_loop
                
            factorial_done:
                mov lr r20
                ret

            result:
                %d 0
        )";

    auto asm_result = asm_engine.assemble(source);
    REQUIRE(asm_result.is_ok());
    auto obj = asm_result.value();

    auto disasm_result = disasm.disassemble_object(obj);
    REQUIRE(disasm_result.is_ok());

    std::string disassembly = disasm_result.value();

    // Verify control flow instructions
    REQUIRE(disassembly.find("cal") != std::string::npos);
    REQUIRE(disassembly.find("ret") != std::string::npos);
    REQUIRE(disassembly.find("mul r2 r2 r3") != std::string::npos); // iterative multiplication

    // Verify pseudo-instructions were expanded
    REQUIRE(disassembly.find("set at") != std::string::npos);       // from adi
    REQUIRE(disassembly.find("add r3 r3 at") != std::string::npos); // from adi
    REQUIRE(disassembly.find("set ad") != std::string::npos);       // from branch setup
    REQUIRE(disassembly.find("bve ad r4") != std::string::npos);    // from loop condition
  }

  SECTION("binary format round-trip") {
    std::string source = R"(
            %entry: test_main
            
            test_main:
                set r0 42
                set r1 0
                seq r2 r0 42
                set ad end
                bve ad r2 1
                set r1 1
            end:
                hlt
        )";

    // Assemble
    auto asm_result = asm_engine.assemble(source);
    REQUIRE(asm_result.is_ok());
    auto obj = asm_result.value();

    // Convert to binary (what CLI tools write/read)
    auto binary_data = obj.to_binary();
    REQUIRE(binary_data.size() >= 24); // at least header size

    // Verify magic bytes
    REQUIRE(binary_data[0] == 'R');
    REQUIRE(binary_data[1] == 'G');
    REQUIRE(binary_data[2] == 'V');
    REQUIRE(binary_data[3] == 'M');

    // Read back from binary
    auto obj_from_binary = asmr::object_file::from_binary(binary_data);
    REQUIRE(obj_from_binary.is_ok());
    auto obj_loaded = obj_from_binary.value();

    // Verify objects match
    REQUIRE(obj_loaded.entry_offset == obj.entry_offset);
    REQUIRE(obj_loaded.code == obj.code);
    REQUIRE(obj_loaded.data == obj.data);

    // Disassemble the loaded object
    auto disasm_result = disasm.disassemble_object(obj_loaded, asmr::disasm_format::annotated);
    REQUIRE(disasm_result.is_ok());

    std::string output = disasm_result.value();

    // Verify annotated format includes headers
    REQUIRE(output.find("irre object file disassembly") != std::string::npos);
    REQUIRE(output.find("entry point:") != std::string::npos);
    REQUIRE(output.find("code size:") != std::string::npos);

    // Verify instructions
    REQUIRE(output.find("set r0 0x002a") != std::string::npos);
    REQUIRE(output.find("set r1 0x0000") != std::string::npos);
    REQUIRE(output.find("seq r2 r0 0x2a") != std::string::npos);
    REQUIRE(output.find("hlt") != std::string::npos);
  }

  SECTION("test all sample programs") {
    std::vector<std::string> sample_files = {
        "./sample/asm/simple.asm", "./sample/asm/math.asm", "./sample/asm/hello.asm", "./sample/asm/loops.asm"
    };

    for (const auto& file : sample_files) {
      INFO("Testing sample file: " << file);

      // Read source file
      std::ifstream infile(file);
      REQUIRE(infile.is_open());
      std::string source((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
      infile.close();

      // Assemble
      auto asm_result = asm_engine.assemble(source);
      REQUIRE(asm_result.is_ok());
      auto obj = asm_result.value();

      // Disassemble
      auto disasm_result = disasm.disassemble_object(obj);
      REQUIRE(disasm_result.is_ok());

      // Basic verification - should contain at least one instruction
      std::string disassembly = disasm_result.value();
      REQUIRE(!disassembly.empty());

      // Should contain entry point info for annotated format
      auto annotated_result = disasm.disassemble_object(obj, asmr::disasm_format::annotated);
      REQUIRE(annotated_result.is_ok());
      std::string annotated = annotated_result.value();
      REQUIRE(annotated.find("entry point:") != std::string::npos);
    }
  }
}