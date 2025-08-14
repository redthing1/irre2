#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "irre.hpp"

using namespace irre;

// Helper function to test round-trip encoding/decoding
auto test_round_trip = [](const instruction& inst) {
  word encoded = codec::encode(inst);
  auto decoded = codec::decode(encoded);
  REQUIRE(decoded.is_ok());
  word re_encoded = codec::encode(decoded.value());
  REQUIRE(encoded == re_encoded);

  // Also test that opcodes match
  REQUIRE(get_opcode(inst) == get_opcode(decoded.value()));
};

TEST_CASE("Instruction format: inst_op") {
  SECTION("nop instruction") {
    auto inst = make::nop();
    test_round_trip(inst);

    word encoded = codec::encode(inst);
    REQUIRE(encoded == 0x00000000); // opcode 0x00 in bits 31-24
  }

  SECTION("ret instruction") {
    auto inst = make::ret();
    test_round_trip(inst);

    word encoded = codec::encode(inst);
    REQUIRE(encoded == 0x2b000000); // opcode 0x2b in bits 31-24
  }

  SECTION("hlt instruction") {
    auto inst = make::hlt();
    test_round_trip(inst);

    word encoded = codec::encode(inst);
    REQUIRE(encoded == 0xff000000); // opcode 0xff in bits 31-24
  }
}

TEST_CASE("Instruction format: inst_op_reg") {
  SECTION("jmp with various registers") {
    // Test with different register types
    std::vector<reg> test_regs = {
        reg::r0, reg::r15, reg::r31,                  // GPRs
        reg::pc, reg::lr,  reg::ad,  reg::at, reg::sp // Special registers
    };

    for (auto r : test_regs) {
      auto inst = make::jmp(r);
      test_round_trip(inst);

      word encoded = codec::encode(inst);
      // Check opcode (0x21) and register placement
      REQUIRE(((encoded >> 24) & 0xff) == 0x21);
      REQUIRE(((encoded >> 16) & 0xff) == static_cast<uint8_t>(r));
    }
  }

  SECTION("cal instruction") {
    auto inst = make::cal(reg::r10);
    test_round_trip(inst);

    word encoded = codec::encode(inst);
    REQUIRE(((encoded >> 24) & 0xff) == 0x2a); // opcode
    REQUIRE(((encoded >> 16) & 0xff) == 0x0a); // r10 = 0x0a
  }
}

TEST_CASE("Instruction format: inst_op_imm24") {
  SECTION("jmi with various addresses") {
    std::vector<uint32_t> test_addrs = {
        0x000000, // minimum
        0x123456, // typical
        0xffffff  // maximum 24-bit value
    };

    for (auto addr : test_addrs) {
      auto inst = make::jmi(addr);
      test_round_trip(inst);

      word encoded = codec::encode(inst);
      REQUIRE(((encoded >> 24) & 0xff) == 0x20);          // opcode
      REQUIRE((encoded & 0xffffff) == (addr & 0xffffff)); // 24-bit addr
    }
  }

  SECTION("int instruction") {
    auto inst = make::int_(0x123456);
    test_round_trip(inst);

    word encoded = codec::encode(inst);
    REQUIRE(((encoded >> 24) & 0xff) == 0xf0); // opcode
    REQUIRE((encoded & 0xffffff) == 0x123456); // interrupt code
  }
}

TEST_CASE("Instruction format: inst_op_reg_imm16") {
  SECTION("set instruction") {
    auto inst = make::set(reg::r5, 0x1234);
    test_round_trip(inst);

    word encoded = codec::encode(inst);
    REQUIRE(((encoded >> 24) & 0xff) == 0x0b); // opcode
    REQUIRE(((encoded >> 16) & 0xff) == 0x05); // r5
    REQUIRE((encoded & 0xffff) == 0x1234);     // immediate
  }

  SECTION("set with edge values") {
    std::vector<uint16_t> test_vals = {0x0000, 0xffff, 0x8000};

    for (auto val : test_vals) {
      auto inst = make::set(reg::r0, val);
      test_round_trip(inst);
    }
  }
}

TEST_CASE("Instruction format: inst_op_reg_reg") {
  SECTION("mov instruction") {
    auto inst = make::mov(reg::r1, reg::r2);
    test_round_trip(inst);

    word encoded = codec::encode(inst);
    REQUIRE(((encoded >> 24) & 0xff) == 0x0c); // opcode
    REQUIRE(((encoded >> 16) & 0xff) == 0x01); // r1
    REQUIRE(((encoded >> 8) & 0xff) == 0x02);  // r2
  }
}

TEST_CASE("Instruction format: inst_op_reg_reg_imm8") {
  SECTION("ldw instruction") {
    auto inst = make::ldw(reg::r3, reg::r4, 0x10);
    test_round_trip(inst);

    word encoded = codec::encode(inst);
    REQUIRE(((encoded >> 24) & 0xff) == 0x0d); // opcode
    REQUIRE(((encoded >> 16) & 0xff) == 0x03); // r3
    REQUIRE(((encoded >> 8) & 0xff) == 0x04);  // r4
    REQUIRE((encoded & 0xff) == 0x10);         // offset
  }

  SECTION("bve instruction") {
    auto inst = make::bve(reg::r1, reg::r2, 0x05);
    test_round_trip(inst);

    word encoded = codec::encode(inst);
    REQUIRE(((encoded >> 24) & 0xff) == 0x24); // opcode
    REQUIRE(((encoded >> 16) & 0xff) == 0x01); // r1 (target addr reg)
    REQUIRE(((encoded >> 8) & 0xff) == 0x02);  // r2 (comparison reg)
    REQUIRE((encoded & 0xff) == 0x05);         // comparison value
  }
}

TEST_CASE("Instruction format: inst_op_reg_imm8x2") {
  SECTION("sia instruction") {
    // Test with various shift values
    std::vector<std::pair<uint8_t, uint8_t>> test_vals = {{0x01, 0x00}, {0x0f, 0x04}, {0xff, 0x1f}};

    for (auto [v0, v1] : test_vals) {
      auto inst = make::sia(reg::r7, v0, v1);
      test_round_trip(inst);

      word encoded = codec::encode(inst);
      REQUIRE(((encoded >> 24) & 0xff) == 0x40); // opcode
      REQUIRE(((encoded >> 16) & 0xff) == 0x07); // r7
      REQUIRE(((encoded >> 8) & 0xff) == v0);    // v0
      REQUIRE((encoded & 0xff) == v1);           // v1
    }
  }
}

TEST_CASE("Instruction format: inst_op_reg_reg_reg") {
  SECTION("add instruction") {
    auto inst = make::add(reg::r1, reg::r2, reg::r3);
    test_round_trip(inst);

    word encoded = codec::encode(inst);
    REQUIRE(((encoded >> 24) & 0xff) == 0x01); // opcode
    REQUIRE(((encoded >> 16) & 0xff) == 0x01); // r1
    REQUIRE(((encoded >> 8) & 0xff) == 0x02);  // r2
    REQUIRE((encoded & 0xff) == 0x03);         // r3
  }

  SECTION("arithmetic operations") {
    std::vector<std::function<instruction(reg, reg, reg)>> ops = {
        make::add, make::sub, make::mul, make::div, make::mod
    };

    for (auto op : ops) {
      auto inst = op(reg::r10, reg::r11, reg::r12);
      test_round_trip(inst);
    }
  }
}

TEST_CASE("All 35 IRRE instructions") {
  // Test every instruction from the spec
  SECTION("Control flow instructions") {
    test_round_trip(make::nop());
    test_round_trip(make::ret());
    test_round_trip(make::hlt());
    test_round_trip(make::jmp(reg::r0));
    test_round_trip(make::jmi(0x123456));
    test_round_trip(make::cal(reg::lr));
    test_round_trip(make::bve(reg::r1, reg::r2, 0x05));
    test_round_trip(make::bvn(reg::r1, reg::r2, 0x05));
  }

  SECTION("Arithmetic operations") {
    test_round_trip(make::add(reg::r1, reg::r2, reg::r3));
    test_round_trip(make::sub(reg::r1, reg::r2, reg::r3));
    test_round_trip(make::mul(reg::r1, reg::r2, reg::r3));
    test_round_trip(make::div(reg::r1, reg::r2, reg::r3));
    test_round_trip(make::mod(reg::r1, reg::r2, reg::r3));
  }

  SECTION("Memory operations") {
    test_round_trip(make::ldw(reg::r1, reg::r2, 0x10));
    test_round_trip(make::stw(reg::r1, reg::r2, 0x10));
    test_round_trip(make::ldb(reg::r1, reg::r2, 0x10));
    test_round_trip(make::stb(reg::r1, reg::r2, 0x10));
  }

  SECTION("Data movement") {
    test_round_trip(make::mov(reg::r1, reg::r2));
    test_round_trip(make::set(reg::r1, 0x1234));
  }
}

TEST_CASE("Error handling") {
  SECTION("Invalid opcodes") {
    std::vector<uint8_t> invalid_opcodes = {0xfe, 0x99, 0x50, 0x11};

    for (auto op : invalid_opcodes) {
      word w = static_cast<word>(op) << 24;
      auto result = codec::decode(w);
      REQUIRE(result.is_err());
      REQUIRE(result.error() == decode_error::invalid_opcode);
    }
  }

  SECTION("Invalid registers") {
    // Test register values > 0x24 (sp is max at 0x24)
    std::vector<uint8_t> invalid_regs = {0x25, 0x30, 0x80, 0xff};

    for (auto reg_val : invalid_regs) {
      // Test with jmp instruction (op_reg format)
      word w = (0x21 << 24) | (reg_val << 16);
      auto result = codec::decode(w);
      REQUIRE(result.is_err());
      REQUIRE(result.error() == decode_error::invalid_register);
    }
  }
}

TEST_CASE("Byte-level encoding") {
  SECTION("Little-endian byte ordering") {
    auto inst = make::add(reg::r1, reg::r2, reg::r3);
    word w = codec::encode(inst);

    auto bytes = codec::encode_bytes(inst);
    REQUIRE(bytes[0] == (w & 0xff));         // bits 7-0
    REQUIRE(bytes[1] == ((w >> 8) & 0xff));  // bits 15-8
    REQUIRE(bytes[2] == ((w >> 16) & 0xff)); // bits 23-16
    REQUIRE(bytes[3] == ((w >> 24) & 0xff)); // bits 31-24

    // Test round-trip through bytes
    auto decoded = codec::decode_bytes(bytes);
    REQUIRE(decoded.is_ok());
    REQUIRE(codec::encode(decoded.value()) == w);
  }
}

TEST_CASE("Register validation") {
  struct TestCase {
    uint8_t value;
    bool valid;
    const char* name;
  };

  std::vector<TestCase> cases = {{0x00, true, "r0"},       {0x1f, true, "r31"},      {0x20, true, "pc"},
                                 {0x21, true, "lr"},       {0x22, true, "ad"},       {0x23, true, "at"},
                                 {0x24, true, "sp"},       {0x25, false, "invalid"}, {0x30, false, "invalid"},
                                 {0xff, false, "max_byte"}};

  for (const auto& tc : cases) {
    INFO("Testing register value: " << static_cast<int>(tc.value) << " (" << tc.name << ")");

    // Test with mov instruction (requires 2 registers)
    word w = (0x0c << 24) | (tc.value << 16) | (0x01 << 8); // mov rX, r1
    auto result = codec::decode(w);

    if (tc.valid) {
      REQUIRE(result.is_ok());
    } else {
      REQUIRE(result.is_err());
      REQUIRE(result.error() == decode_error::invalid_register);
    }
  }
}