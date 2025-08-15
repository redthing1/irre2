#include <catch2/catch_test_macros.hpp>
#include "assembler/assembler.hpp"
#include "assembler/object.hpp"

namespace asmr = irre::assembler;
using irre::byte;

TEST_CASE("assembler basic functionality", "[assembler]") {
  asmr::assembler asm_engine;

  SECTION("simple assembly") {
    std::string source = "nop\nhlt";
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_ok());
    auto obj = result.value();
    REQUIRE(obj.code.size() == 8); // 2 instructions * 4 bytes each
  }

  SECTION("assembly with labels") {
    std::string source = "main:\n  nop\n  hlt";
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_ok());
    auto obj = result.value();
    REQUIRE(obj.code.size() == 8);
  }

  SECTION("assembly with entry directive") {
    std::string source = "%entry: main\nmain:\n  nop\n  hlt";
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_ok());
    auto obj = result.value();
    REQUIRE(obj.code.size() == 8);
    REQUIRE(obj.entry_offset == 0);
  }
}

TEST_CASE("assembler error handling", "[assembler][validation]") {
  asmr::assembler asm_engine;

  SECTION("unknown instruction") {
    std::string source = "unknown_instruction";
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.error == asmr::assemble_error::invalid_instruction);
    REQUIRE(error.message.find("unknown instruction") != std::string::npos);
  }

  SECTION("wrong operand count") {
    std::string source = "nop r1"; // nop takes no operands
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.error == asmr::assemble_error::invalid_instruction);
    REQUIRE(error.message.find("expects 0 operands") != std::string::npos);
  }

  SECTION("invalid register") {
    std::string source = "mov invalid_reg r1";
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.error == asmr::assemble_error::invalid_instruction);
    REQUIRE(error.message.find("must be register") != std::string::npos);
  }

  SECTION("immediate out of range") {
    std::string source = "set r1 $10000"; // 16-bit max is $FFFF = 65535, $10000 = 65536 should fail
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.error == asmr::assemble_error::invalid_immediate);
    REQUIRE(error.message.find("16-bit range") != std::string::npos);
  }

  SECTION("parse error for invalid syntax") {
    std::string source = "set r1 $xyz"; // invalid hex - should fail at parse level
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.error == asmr::assemble_error::parse_error); // grammar rejects this
  }

  SECTION("parse error for incomplete syntax") {
    std::string source = "set r1 $"; // incomplete - should fail at parse level
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.error == asmr::assemble_error::parse_error); // grammar rejects this
  }
}

TEST_CASE("symbol resolution error handling", "[assembler][symbols]") {
  asmr::assembler asm_engine;

  SECTION("undefined symbol reference") {
    std::string source = "jmi undefined_label";
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.error == asmr::assemble_error::undefined_symbol);
    REQUIRE(error.message.find("undefined symbol 'undefined_label'") != std::string::npos);
  }

  SECTION("duplicate label definition") {
    std::string source = "main:\n  nop\nmain:\n  hlt";
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.error == asmr::assemble_error::undefined_symbol); // maps to undefined_symbol error type
    REQUIRE(error.message.find("duplicate symbol 'main'") != std::string::npos);
  }

  SECTION("forward reference resolution") {
    std::string source = "jmi forward_label\nforward_label:\n  nop\n  hlt";
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_ok()); // forward references should work
    auto obj = result.value();
    REQUIRE(obj.code.size() == 12); // jmi + nop + hlt = 3 instructions * 4 bytes
  }

  SECTION("symbol used in different instruction formats") {
    std::string source = "main:\n  nop\n  jmi main\n  hlt";
    auto result = asm_engine.assemble(source);

    REQUIRE(result.is_ok());
    auto obj = result.value();
    REQUIRE(obj.code.size() == 12); // 3 instructions * 4 bytes
  }
}

TEST_CASE("object file format", "[object]") {
  SECTION("binary serialization and deserialization") {
    // create test object file
    asmr::object_file original;
    original.entry_offset = 4; // must be within code section
    original.code = {0x00, 0x00, 0x00, 0xff, 0x01, 0x02, 0x03, 0x04};
    original.data = {0xde, 0xad, 0xbe, 0xef};

    // serialize to binary
    auto binary = original.to_binary();

    // check header structure
    REQUIRE(binary.size() >= 24); // minimum header size
    REQUIRE(binary[0] == 'R');    // magic
    REQUIRE(binary[1] == 'G');
    REQUIRE(binary[2] == 'V');
    REQUIRE(binary[3] == 'M');

    // deserialize back
    auto result = asmr::object_file::from_binary(binary);
    if (result.is_err()) {
      INFO("Deserialization error: " << result.error());
    }
    REQUIRE(result.is_ok());

    auto restored = result.value();
    REQUIRE(restored.entry_offset == original.entry_offset);
    REQUIRE(restored.code == original.code);
    REQUIRE(restored.data == original.data);
  }

  SECTION("invalid magic detection with detailed error") {
    std::vector<byte> bad_binary = {'B', 'A', 'D', '!', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    auto result = asmr::object_file::from_binary(bad_binary);
    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.find("invalid magic bytes 'BAD!'") != std::string::npos);
    REQUIRE(error.find("expected 'RGVM'") != std::string::npos);
  }

  SECTION("file too small detection with size info") {
    std::vector<byte> tiny_binary = {'R', 'G', 'V', 'M'};
    auto result = asmr::object_file::from_binary(tiny_binary);
    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.find("file too small (4 bytes)") != std::string::npos);
    REQUIRE(error.find("require at least 24 bytes") != std::string::npos);
  }

  SECTION("empty file detection") {
    std::vector<byte> empty_binary;
    auto result = asmr::object_file::from_binary(empty_binary);
    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.find("empty file") != std::string::npos);
  }

  SECTION("version mismatch detection") {
    std::vector<byte> wrong_version = {'R', 'G', 'V', 'M', 0x99, 0x00, 0, 0, 0, 0, 0, 0,
                                       0,   0,   0,   0,   0,    0,    0, 0, 0, 0, 0, 0};
    auto result = asmr::object_file::from_binary(wrong_version);
    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.find("unsupported version 153") != std::string::npos);
    REQUIRE(error.find("supports version 1") != std::string::npos);
  }

  SECTION("misaligned entry point detection") {
    // create valid header but with misaligned entry point
    std::vector<byte> misaligned_binary;
    irre::byte_io::write_magic(misaligned_binary);
    irre::byte_io::write_u16_le(misaligned_binary, 1); // version
    irre::byte_io::write_u16_le(misaligned_binary, 0); // reserved
    irre::byte_io::write_u32_le(misaligned_binary, 1); // entry offset = 1 (misaligned)
    irre::byte_io::write_u32_le(misaligned_binary, 8); // code size
    irre::byte_io::write_u32_le(misaligned_binary, 0); // data size
    irre::byte_io::write_u32_le(misaligned_binary, 0); // reserved
    // add 8 bytes of code
    for (int i = 0; i < 8; i++) {
      misaligned_binary.push_back(0);
    }

    auto result = asmr::object_file::from_binary(misaligned_binary);
    REQUIRE(result.is_err());
    auto error = result.error();
    REQUIRE(error.find("not 4-byte aligned") != std::string::npos);
  }
}

TEST_CASE("comprehensive assembly programs", "[assembler][programs]") {
  asmr::assembler asm_engine;

  SECTION("fibonacci calculator program") {
    std::string source = R"(
            %entry: main
            
            ; fibonacci calculator using iterative approach
            main:
                set r0 0      ; fib(0) = 0
                set r1 1      ; fib(1) = 1
                set r2 10     ; calculate fib(10)
                set r3 0      ; counter
                
            fib_loop:
                ; check if counter >= target
                tcu r4 r3 r2  ; r4 = sign of (r3 - r2)
                bif r4 done 1 ; if r4 == 1 (r3 >= r2), branch to done
                
                ; calculate next fibonacci number
                add r4 r0 r1  ; r4 = r0 + r1
                mov r0 r1     ; r0 = r1 (previous becomes current)
                mov r1 r4     ; r1 = r4 (new value)
                
                ; increment counter
                adi r3 r3 1
                
                jmi fib_loop
                
            done:
                hlt
        )";

    auto result = asm_engine.assemble(source);
    REQUIRE(result.is_ok());

    auto obj = result.value();
    REQUIRE(obj.code.size() > 0);
    REQUIRE(obj.code.size() % 4 == 0); // all instructions are 4 bytes
    REQUIRE(obj.entry_offset == 0);

    // verify we have a reasonable number of instructions (should be ~15 instructions)
    size_t instruction_count = obj.code.size() / 4;
    REQUIRE(instruction_count >= 10);
    REQUIRE(instruction_count <= 20);
  }

  SECTION("function with subroutine calls") {
    std::string source = R"(
            %entry: main
            
            main:
                set r0 5      ; argument for factorial
                set lr main_return  ; set return address
                jmi factorial ; jump to factorial function
            main_return:
                hlt           ; result is in r0
                
            factorial:
                ; calculate factorial recursively
                ; input: r0 = n, output: r0 = n!
                set r1 1      ; base case check
                tcu r2 r1 r0  ; r2 = sign(1 - r0)
                bif r2 factorial_base 1  ; if 1-r0 >= 0 (r0 <= 1), go to base case
                
                ; recursive case: n! = n * (n-1)!
                mov r1 r0     ; save n
                sbi r0 r0 1   ; r0 = n-1 (using pseudo-instruction)
                set lr factorial_return  ; set return address
                jmi factorial ; recursive call
            factorial_return:
                mul r0 r0 r1  ; r0 = (n-1)! * n
                jmp lr        ; return
                
            factorial_base:
                set r0 1      ; return 1
                jmp lr        ; return
        )";

    auto result = asm_engine.assemble(source);
    if (result.is_err()) {
      INFO("Subroutine assembly error: " << result.error().message);
    }
    REQUIRE(result.is_ok());

    auto obj = result.value();
    REQUIRE(obj.code.size() > 0);
    REQUIRE(obj.entry_offset == 0);

    // verify all instructions are properly encoded
    size_t instruction_count = obj.code.size() / 4;
    REQUIRE(instruction_count >= 8);
  }

  SECTION("memory operations and data structures") {
    std::string source = R"(
            %entry: main
            
            main:
                ; test basic memory operations
                set sp $1000  ; set stack pointer
                set r0 42     ; test value
                set r1 100    ; another test value
                
                ; store and load word operations
                stw r0 sp 0   ; store r0 at sp+0
                stw r1 sp 4   ; store r1 at sp+4
                ldw r2 sp 0   ; load from sp+0 to r2
                ldw r3 sp 4   ; load from sp+4 to r3
                
                ; store and load byte operations  
                stb r0 sp 8   ; store byte at sp+8
                ldb r4 sp 8   ; load byte from sp+8
                
                ; verify values match
                tcu r5 r0 r2  ; compare original and loaded word
                tcu r6 r1 r3  ; compare original and loaded word
                
                hlt
        )";

    auto result = asm_engine.assemble(source);
    if (result.is_err()) {
      INFO("Memory operations assembly error: " << result.error().message);
    }
    REQUIRE(result.is_ok());

    auto obj = result.value();
    REQUIRE(obj.code.size() > 0);

    // verify we handle all memory operations correctly
    size_t instruction_count = obj.code.size() / 4;
    REQUIRE(instruction_count >= 10);
  }

  SECTION("all instruction formats in one program") {
    std::string source = R"(
            %entry: main
            
            main:
                ; format::op
                nop
                hlt
                ret
                
                ; format::op_reg  
                jmp r0
                cal r1
                
                ; format::op_imm24
                jmi end_label
                int $123456
                
                ; format::op_reg_imm16
                set r0 $1234
                sup r1 $5678
                
                ; format::op_reg_reg
                mov r0 r1
                sxt r2 r3
                not r4 r5
                
                ; format::op_reg_reg_imm8
                ldw r0 r1 4
                stw r2 r3 8
                ldb r4 r5 12
                stb r6 r7 16
                seq r4 r5 3
                ; demonstrate bve/bvn with literal addresses
                set r10 $100      ; hex example address
                set r11 #0        ; decimal test value
                bve r10 r11 0     ; branch to r10 if r11 == 0 
                bvn r10 r11 1     ; branch to r10 if r11 != 1
                
                ; format::op_reg_imm8x2
                sia r0 $12 $34
                
                ; format::op_reg_reg_reg
                add r0 r1 r2
                sub r3 r4 r5
                mul r6 r7 r8
                div r9 r10 r11
                mod r12 r13 r14
                and r15 r16 r17
                orr r18 r19 r20
                xor r21 r22 r23
                lsh r24 r25 r26
                ash r27 r28 r29
                tcu r30 r31 r0
                tcs r1 r2 r3
                snd r4 r5 r6
                
            end_label:
                hlt
        )";

    auto result = asm_engine.assemble(source);
    REQUIRE(result.is_ok());

    auto obj = result.value();
    REQUIRE(obj.code.size() > 0);

    // should have assembled ~35 instructions
    size_t instruction_count = obj.code.size() / 4;
    REQUIRE(instruction_count >= 30);
    REQUIRE(instruction_count <= 40);
  }

  SECTION("pseudo-instructions expansion") {
    std::string source = R"(
            %entry: main
            
            main:
                ; test all pseudo-instructions
                adi r0 r1 42    ; should expand to: set at 42; add r0 r1 at
                sbi r2 r3 15    ; should expand to: set at 15; sub r2 r3 at
                bif r4 loop 1   ; should expand to: set ad loop; bve ad r4 1
                
                set r0 100
                
            loop:
                ; more pseudo-instruction usage
                sbi r0 r0 1     ; decrement
                bif r0 done 0   ; branch if zero
                jmi loop
                
            done:
                hlt
        )";

    auto result = asm_engine.assemble(source);
    REQUIRE(result.is_ok());

    auto obj = result.value();
    REQUIRE(obj.code.size() > 0);

    // each pseudo-instruction expands to 2 real instructions
    // so we should have more instructions than source lines suggest
    size_t instruction_count = obj.code.size() / 4;
    REQUIRE(instruction_count >= 10); // accounting for expansions
  }

  SECTION("complex label resolution with forward/backward references") {
    std::string source = R"(
            %entry: main
            
            ; forward references
            main:
                jmi init       ; forward reference
                jmi process    ; forward reference  
                jmi cleanup    ; forward reference
                jmi end        ; forward reference
                
            ; backward references and local loops
            init:
                set r0 0
                jmi main_loop  ; forward reference
                
            process:
                adi r0 r0 1
                jmi main_loop  ; forward reference
                
            cleanup:
                set r0 0
                jmi init       ; backward reference
                
            main_loop:
                set r2 10     ; load 10 into r2
                tcu r1 r0 r2  ; compare r0 with 10 (r1 = sign of r0-r2)
                bif r1 end 1  ; if r1 == 1 (r0 >= 10), branch to end
                
                jmi process    ; backward reference
                
            end:
                hlt
        )";

    auto result = asm_engine.assemble(source);
    if (result.is_err()) {
      INFO("Label resolution assembly error: " << result.error().message);
    }
    REQUIRE(result.is_ok());

    auto obj = result.value();
    REQUIRE(obj.code.size() > 0);
    REQUIRE(obj.entry_offset == 0);

    // verify all labels were resolved correctly
    size_t instruction_count = obj.code.size() / 4;
    REQUIRE(instruction_count >= 10);
  }

  SECTION("comprehensive label and symbol resolution test") {
    std::string source = R"(
            %entry: start
            
            ; test various label formats and symbol resolution patterns
            start:
                ; test immediate addressing with labels
                jmi subroutine_a   ; 24-bit immediate jump
                jmi data_section   ; jump to data area
                hlt
                
            ; test labels with underscores and numbers  
            subroutine_a:
                set r0 $42         ; hex immediate
                set r1 #100        ; decimal immediate
                jmi loop_test1     ; forward jump
                
            data_section:
                ; simulate some data with nops
                nop
                nop
                nop
                jmi subroutine_b   ; jump to another function
                
            loop_test1:
                ; test loop with backward reference
                adi r0 r0 1        ; increment
                set r2 #200        ; compare value
                tcu r3 r0 r2       ; r3 = sign(r0 - 200)
                bif r3 exit_point 1 ; if r0 >= 200, exit
                jmi loop_test1     ; backward reference - loop again
                
            subroutine_b:
                ; test nested function calls with lr
                set lr return_point ; manual return address
                jmi nested_call     ; call nested function
            return_point:
                jmi exit_point      ; continue to exit
                
            nested_call:
                set r4 $ff         ; do some work
                jmp lr             ; return via lr register
                
            exit_point:
                ; test conditional branches with symbols
                set r5 skip_section ; load address into register
                set r6 #1          ; test value
                bve r5 r6 1        ; branch to skip_section if r6 == 1
                
            skip_section:
                ; final cleanup
                set r0 #0          ; clear register
                hlt                ; terminate
        )";

    auto result = asm_engine.assemble(source);
    if (result.is_err()) {
      INFO("Comprehensive symbol test error: " << result.error().message);
    }
    REQUIRE(result.is_ok());

    auto obj = result.value();
    REQUIRE(obj.code.size() > 0);
    REQUIRE(obj.entry_offset == 0);

    // verify we have substantial program
    size_t instruction_count = obj.code.size() / 4;
    REQUIRE(instruction_count >= 15);
    REQUIRE(instruction_count <= 30);

    // verify entry point is correctly set to 'start' label (should be at offset 0)
    REQUIRE(obj.entry_offset == 0);
  }
}

TEST_CASE("portable byte i/o", "[util]") {
  SECTION("little-endian u32 write/read") {
    std::vector<byte> buffer;
    uint32_t value = 0x12345678;

    irre::byte_io::write_u32_le(buffer, value);
    REQUIRE(buffer.size() == 4);
    REQUIRE(buffer[0] == 0x78); // lsb first
    REQUIRE(buffer[1] == 0x56);
    REQUIRE(buffer[2] == 0x34);
    REQUIRE(buffer[3] == 0x12); // msb last

    uint32_t read_value = irre::byte_io::read_u32_le(buffer.data());
    REQUIRE(read_value == value);
  }

  SECTION("little-endian u16 write/read") {
    std::vector<byte> buffer;
    uint16_t value = 0x1234;

    irre::byte_io::write_u16_le(buffer, value);
    REQUIRE(buffer.size() == 2);
    REQUIRE(buffer[0] == 0x34); // lsb first
    REQUIRE(buffer[1] == 0x12); // msb last

    uint16_t read_value = irre::byte_io::read_u16_le(buffer.data());
    REQUIRE(read_value == value);
  }

  SECTION("magic write/check") {
    std::vector<byte> buffer;
    irre::byte_io::write_magic(buffer);

    REQUIRE(buffer.size() == 4);
    REQUIRE(buffer[0] == 'R');
    REQUIRE(buffer[1] == 'G');
    REQUIRE(buffer[2] == 'V');
    REQUIRE(buffer[3] == 'M');

    REQUIRE(irre::byte_io::check_magic(buffer.data()));

    // test bad magic
    buffer[0] = 'X';
    REQUIRE_FALSE(irre::byte_io::check_magic(buffer.data()));
  }
}