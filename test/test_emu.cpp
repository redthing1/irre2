#include <catch2/catch_test_macros.hpp>
#include "emu/vm.hpp"
#include "emu/devices.hpp"
#include "assembler/assembler.hpp"

using namespace irre;
using namespace irre::emu;

TEST_CASE("Memory subsystem", "[emu][memory]") {
  memory mem(1024);

  SECTION("word read/write") {
    // test little-endian encoding
    mem.write_word(0, 0x12345678);

    REQUIRE(mem.read_byte(0) == 0x78); // lsb first
    REQUIRE(mem.read_byte(1) == 0x56);
    REQUIRE(mem.read_byte(2) == 0x34);
    REQUIRE(mem.read_byte(3) == 0x12); // msb last

    REQUIRE(mem.read_word(0) == 0x12345678);
  }

  SECTION("byte read/write") {
    mem.write_byte(0, 0xab);
    mem.write_byte(1, 0xcd);

    REQUIRE(mem.read_byte(0) == 0xab);
    REQUIRE(mem.read_byte(1) == 0xcd);
  }

  SECTION("bounds checking") {
    REQUIRE_THROWS_AS(mem.read_word(1021), std::out_of_range);
    REQUIRE_THROWS_AS(mem.write_word(1021, 0), std::out_of_range);
    REQUIRE_THROWS_AS(mem.read_byte(1024), std::out_of_range);
    REQUIRE_THROWS_AS(mem.write_byte(1024, 0), std::out_of_range);
  }
}

TEST_CASE("Register file", "[emu][registers]") {
  register_file regs;

  SECTION("basic read/write") {
    regs.write(reg::r0, 42);
    regs.write(reg::r1, 0xdeadbeef);

    REQUIRE(regs.read(reg::r0) == 42);
    REQUIRE(regs.read(reg::r1) == 0xdeadbeef);
  }

  SECTION("special register accessors") {
    regs.set_pc(0x1000);
    regs.set_lr(0x2000);
    regs.set_sp(0x3000);

    REQUIRE(regs.pc() == 0x1000);
    REQUIRE(regs.lr() == 0x2000);
    REQUIRE(regs.sp() == 0x3000);
  }

  SECTION("clear registers") {
    regs.write(reg::r0, 123);
    regs.write(reg::pc, 456);
    regs.clear();

    REQUIRE(regs.read(reg::r0) == 0);
    REQUIRE(regs.pc() == 0);
  }
}

TEST_CASE("Basic instruction execution", "[emu][execution]") {
  vm machine(1024);

  SECTION("arithmetic instructions") {
    // test add instruction
    machine.set_register(reg::r1, 10);
    machine.set_register(reg::r2, 20);

    // manually create add r0, r1, r2 instruction
    auto add_inst = make::add(reg::r0, reg::r1, reg::r2);
    auto encoded = codec::encode(add_inst);

    std::vector<byte> program;
    auto bytes = codec::encode_bytes(add_inst);
    program.assign(bytes.begin(), bytes.end());

    // add halt
    auto halt_inst = make::hlt();
    auto halt_bytes = codec::encode_bytes(halt_inst);
    program.insert(program.end(), halt_bytes.begin(), halt_bytes.end());

    machine.load_binary(program);
    machine.run();

    REQUIRE(machine.get_register(reg::r0) == 30);
    REQUIRE(machine.get_execution_state() == execution_state::halted);
  }

  SECTION("memory instructions") {
    // test store and load
    machine.set_register(reg::r1, 0x12345678);
    machine.set_register(reg::r2, 100); // address

    std::vector<byte> program;

    // stw r1, r2, 0 - store r1 to memory[r2+0]
    auto store = make::stw(reg::r1, reg::r2, 0);
    auto store_bytes = codec::encode_bytes(store);
    program.insert(program.end(), store_bytes.begin(), store_bytes.end());

    // ldw r0, r2, 0 - load from memory[r2+0] to r0
    auto load = make::ldw(reg::r0, reg::r2, 0);
    auto load_bytes = codec::encode_bytes(load);
    program.insert(program.end(), load_bytes.begin(), load_bytes.end());

    // halt
    auto halt = make::hlt();
    auto halt_bytes = codec::encode_bytes(halt);
    program.insert(program.end(), halt_bytes.begin(), halt_bytes.end());

    machine.load_binary(program);
    machine.run();

    REQUIRE(machine.get_register(reg::r0) == 0x12345678);
    REQUIRE(machine.read_memory_word(100) == 0x12345678);
  }

  SECTION("control flow instructions") {
    std::vector<byte> program;

    // set r0, 42
    auto set_inst = make::set(reg::r0, 42);
    auto set_bytes = codec::encode_bytes(set_inst);
    program.insert(program.end(), set_bytes.begin(), set_bytes.end());

    // jmi 12 (jump to halt instruction at offset 12)
    auto jmp_inst = make::jmi(12);
    auto jmp_bytes = codec::encode_bytes(jmp_inst);
    program.insert(program.end(), jmp_bytes.begin(), jmp_bytes.end());

    // set r0, 99 (should be skipped)
    auto skip_inst = make::set(reg::r0, 99);
    auto skip_bytes = codec::encode_bytes(skip_inst);
    program.insert(program.end(), skip_bytes.begin(), skip_bytes.end());

    // halt (at offset 12)
    auto halt = make::hlt();
    auto halt_bytes = codec::encode_bytes(halt);
    program.insert(program.end(), halt_bytes.begin(), halt_bytes.end());

    machine.load_binary(program);
    machine.run();

    REQUIRE(machine.get_register(reg::r0) == 42); // not 99
    REQUIRE(machine.get_execution_state() == execution_state::halted);
  }
}

TEST_CASE("End-to-end assembler + emulator", "[emu][e2e]") {
  assembler::assembler asm_engine;
  vm machine(4096);

  SECTION("simple fibonacci program") {
    std::string source = R"(
            %entry: start

            ; Calculate fib(10) = 55 using iterative approach
            start:
                set r0 10       ; n = fibonacci number we want to calculate
                
                ; Handle base cases: fib(0)=0, fib(1)=1
                tcu r1 r0 r2    ; compare n with 0
                set r2 0
                tcu r1 r0 r2    ; r1 = sign(n - 0)
                set ad return_zero
                bve ad r1 0     ; if n == 0, return 0
                
                set r2 1        
                tcu r1 r0 r2    ; r1 = sign(n - 1) 
                set ad return_one
                bve ad r1 0     ; if n == 1, return 1
                
                ; Iterative calculation for n >= 2
                ; We'll compute fib(2), fib(3), ..., fib(n)
                set r1 0        ; prev = fib(0) = 0
                set r2 1        ; curr = fib(1) = 1  
                set r3 2        ; i = current fibonacci index being computed
                
            fib_loop:
                ; Check if we've reached our target
                tcu r4 r3 r0    ; r4 = sign(i - n)
                set ad fib_done
                bve ad r4 1     ; if i > n, we're done
                
                ; Compute fib(i) = fib(i-1) + fib(i-2)  
                add r4 r1 r2    ; next = prev + curr = fib(i-2) + fib(i-1)
                mov r1 r2       ; prev = curr (shift fibonacci values)
                mov r2 r4       ; curr = next (r2 now contains fib(i))
                
                ; Move to next fibonacci number
                adi r3 r3 1     ; i++
                jmi fib_loop
                
            fib_done:
                mov r1 r2       ; move result to r1 (test expects result in r1)
                hlt
                
            return_zero:
                set r1 0        ; fib(0) = 0
                hlt
                
            return_one:
                set r1 1        ; fib(1) = 1
                hlt
        )";

    auto asm_result = asm_engine.assemble(source);
    REQUIRE(asm_result.is_ok());

    auto obj = asm_result.value();
    machine.load_program(obj);
    machine.run(1000); // limit iterations for safety

    // fib(10) = 55
    REQUIRE(machine.get_register(reg::r1) == 55);
    REQUIRE(machine.get_execution_state() == execution_state::halted);
  }
}

TEST_CASE("Device system", "[emu][devices]") {
  vm machine(1024);
  auto console = std::make_unique<console_device>();
  auto* console_ptr = console.get();

  device_registry devices;
  devices.register_device(device_ids::console, std::move(console));

  machine.on_device_access([&devices](word device_id, word command, word argument) {
    return devices.access_device(device_id, command, argument);
  });

  SECTION("console device output") {
    // set r0 to console device id
    machine.set_register(reg::r0, device_ids::console);
    machine.set_register(reg::r1, 0);   // putchar command
    machine.set_register(reg::r2, 'H'); // character

    std::vector<byte> program;

    // snd r0, r1, r2 - send character to console
    auto snd = make::snd(reg::r0, reg::r1, reg::r2);
    auto snd_bytes = codec::encode_bytes(snd);
    program.insert(program.end(), snd_bytes.begin(), snd_bytes.end());

    // halt
    auto halt = make::hlt();
    auto halt_bytes = codec::encode_bytes(halt);
    program.insert(program.end(), halt_bytes.begin(), halt_bytes.end());

    machine.load_binary(program);
    machine.run();

    REQUIRE(console_ptr->get_output() == "H");
  }
}

TEST_CASE("Error handling", "[emu][errors]") {
  vm machine(1024);
  bool error_occurred = false;
  runtime_error last_error;

  machine.on_error([&](runtime_error err) {
    error_occurred = true;
    last_error = err;
  });

  SECTION("division by zero") {
    machine.set_register(reg::r1, 10);
    machine.set_register(reg::r2, 0);

    std::vector<byte> program;

    // div r0, r1, r2 - divide by zero
    auto div_inst = make::div(reg::r0, reg::r1, reg::r2);
    auto div_bytes = codec::encode_bytes(div_inst);
    program.insert(program.end(), div_bytes.begin(), div_bytes.end());

    machine.load_binary(program);
    machine.step();

    REQUIRE(error_occurred);
    REQUIRE(last_error == runtime_error::division_by_zero);
    REQUIRE(machine.get_execution_state() == execution_state::error);
  }

  SECTION("invalid memory access") {
    machine.set_register(reg::r1, 2000); // out of bounds address

    std::vector<byte> program;

    // ldw r0, r1, 0 - load from invalid address
    auto load = make::ldw(reg::r0, reg::r1, 0);
    auto load_bytes = codec::encode_bytes(load);
    program.insert(program.end(), load_bytes.begin(), load_bytes.end());

    machine.load_binary(program);
    machine.step();

    REQUIRE(error_occurred);
    REQUIRE(last_error == runtime_error::invalid_memory_access);
    REQUIRE(machine.get_execution_state() == execution_state::error);
  }
}