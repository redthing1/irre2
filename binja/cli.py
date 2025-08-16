#!/usr/bin/env python3
"""
IRRE2 CLI Testing Utility

This module provides command-line tools for testing the IRRE2 Binary Ninja plugin
infrastructure without requiring Binary Ninja to be installed.
"""

import argparse
import sys
import struct
from pathlib import Path
from typing import Optional, List

from .irre_types import Opcode, Reg, Format, REG_NAMES, OPCODE_INFO
from .irre_decoder import InstructionDecoder, DecodedInstruction
from .irre_object import IRREObjectFile, load_object_file


def disassemble_command(args):
    """Disassemble an IRRE object file"""
    obj_file = load_object_file(args.file)
    if not obj_file:
        print(f"Error: Failed to load {args.file}")
        return 1

    if args.info:
        obj_file.print_info()

    if not obj_file.code_data:
        print("No code section to disassemble")
        return 0

    print(f"; IRRE object file disassembly")
    print(f"; entry point: 0x{obj_file.entry_offset:x}")
    print(
        f"; code size: {obj_file.code_size} bytes ({obj_file.instruction_count} instructions)"
    )
    print()

    # Disassemble code section
    addr = 0

    while addr < obj_file.code_size:
        if addr + 4 > obj_file.code_size:
            print(
                f"0x{addr:04x}: [incomplete instruction - {obj_file.code_size - addr} bytes remaining]"
            )
            break

        # Get instruction bytes
        inst_bytes = obj_file.get_instruction_bytes(addr)
        if not inst_bytes:
            break

        # Decode instruction
        decoded = InstructionDecoder.decode_bytes(inst_bytes, 0)

        if decoded:
            # Format instruction
            asm_text = InstructionDecoder.format_instruction(decoded)

            # Show bytes in the order they appear in the file (little-endian)
            raw_hex = "".join(f"{b:02x}" for b in inst_bytes)

            # Print with address, hex bytes, and assembly
            print(f"0x{addr:04x}: {raw_hex}  {asm_text}")

            # Mark entry point
            if addr == obj_file.entry_offset:
                print(f"      ; <-- entry point")

        else:
            # Invalid instruction
            raw_hex = "".join(f"{b:02x}" for b in inst_bytes)
            print(f"0x{addr:04x}: {raw_hex}  ??? (invalid instruction)")

        addr += 4

    return 0


def decode_command(args):
    """Decode a single instruction from hex"""
    try:
        # Parse hex string
        hex_str = args.hex.replace("0x", "").replace(" ", "")
        if len(hex_str) != 8:
            print(f"Error: Expected 8 hex characters, got {len(hex_str)}")
            return 1

        # Parse hex string as file-order bytes (little-endian file format)
        # E.g., "1800010b" -> bytes [0x18, 0x00, 0x01, 0x0b]
        bytes_data = bytes.fromhex(hex_str)
        word = struct.unpack("<I", bytes_data)[0]

        # Decode instruction
        decoded = InstructionDecoder.decode_bytes(bytes_data, 0)

        if decoded:
            asm_text = InstructionDecoder.format_instruction(decoded)
            print(f"0x{word:08x}: {asm_text}")
            print(f"Opcode: {decoded.opcode.name} ({decoded.opcode.value:#04x})")
            print(f"Format: {decoded.format.name}")
            if decoded.operands:
                print(f"Operands: {decoded.operands}")
        else:
            print(f"0x{word:08x}: ??? (invalid instruction)")

        return 0

    except ValueError as e:
        print(f"Error parsing hex: {e}")
        return 1


def test_command(args):
    """Run test suite on sample files"""
    sample_dir = Path("sample/asm")
    if not sample_dir.exists():
        print("Error: sample/asm directory not found")
        return 1

    # Find all .o files
    object_files = list(sample_dir.glob("*.o"))
    if not object_files:
        print("No .o files found in sample/asm/")
        return 1

    print(f"Testing {len(object_files)} object files...")
    print()

    success_count = 0
    for obj_path in sorted(object_files):
        print(f"Testing {obj_path.name}...")

        obj_file = load_object_file(str(obj_path))
        if obj_file:
            print(f"  ✓ Loaded successfully")
            print(f"  ✓ Code size: {obj_file.code_size} bytes")
            print(f"  ✓ Entry offset: 0x{obj_file.entry_offset:x}")

            # Try to disassemble first few instructions
            if obj_file.code_data:
                instruction_count = 0
                for addr in range(0, min(16, obj_file.code_size), 4):
                    inst_bytes = obj_file.get_instruction_bytes(addr)
                    if inst_bytes:
                        decoded = InstructionDecoder.decode_bytes(inst_bytes, 0)
                        if decoded:
                            instruction_count += 1
                        else:
                            print(f"  ⚠ Invalid instruction at offset 0x{addr:x}")
                            break

                print(f"  ✓ Decoded {instruction_count} instructions successfully")
                success_count += 1
            else:
                print(f"  ⚠ No code section")
        else:
            print(f"  ✗ Failed to load")

        print()

    print(f"Results: {success_count}/{len(object_files)} files processed successfully")
    return 0 if success_count == len(object_files) else 1


def main():
    """Main CLI entry point"""
    parser = argparse.ArgumentParser(
        description="IRRE2 CLI testing utility", prog="python -m binja.cli"
    )

    subparsers = parser.add_subparsers(dest="command", help="Available commands")

    # Disassemble command
    disasm_parser = subparsers.add_parser(
        "disasm", help="Disassemble an IRRE object file"
    )
    disasm_parser.add_argument("file", help="Path to IRRE object file (.o)")
    disasm_parser.add_argument(
        "--info", "-i", action="store_true", help="Show file information"
    )

    # Decode command
    decode_parser = subparsers.add_parser(
        "decode", help="Decode a single instruction from hex"
    )
    decode_parser.add_argument(
        "hex", help="Instruction as 8-character hex string (e.g., 0b01002a)"
    )

    # Test command
    test_parser = subparsers.add_parser("test", help="Test disassembly on sample files")

    args = parser.parse_args()

    if args.command == "disasm":
        return disassemble_command(args)
    elif args.command == "decode":
        return decode_command(args)
    elif args.command == "test":
        return test_command(args)
    else:
        parser.print_help()
        return 1


if __name__ == "__main__":
    sys.exit(main())
