"""
IRRE2 Object File Parser

This module provides utilities for parsing IRRE RGVM object files,
shared between the Binary Ninja plugin and CLI testing tools.
"""

import struct
from pathlib import Path
from typing import Dict, Optional, Any


class IRREObjectFile:
    """IRRE RGVM object file parser"""

    def __init__(self, file_path: Optional[str] = None, data: Optional[bytes] = None):
        """
        Initialize object file parser

        Args:
            file_path: Path to object file (will be loaded)
            data: Raw file data (alternative to file_path)
        """
        self.file_path = Path(file_path) if file_path else None
        self.raw_data = data
        self.header = None
        self.code_data = None
        self.data_section = None
        self._loaded = False

        if file_path and not data:
            self.load_from_file()
        elif data:
            self.load_from_data(data)

    def load_from_file(self) -> bool:
        """Load object file from disk"""
        if not self.file_path or not self.file_path.exists():
            return False

        try:
            with open(self.file_path, "rb") as f:
                self.raw_data = f.read()
            return self.load_from_data(self.raw_data)
        except Exception:
            return False

    def load_from_data(self, data: bytes) -> bool:
        """Parse object file from raw data"""
        try:
            if len(data) < 24:
                return False

            # Parse RGVM header (24 bytes total)
            magic = data[0:4]
            if magic != b"RGVM":
                return False

            version = struct.unpack("<H", data[4:6])[0]
            reserved1 = struct.unpack("<H", data[6:8])[0]
            entry_offset = struct.unpack("<I", data[8:12])[0]
            code_size = struct.unpack("<I", data[12:16])[0]
            data_size = struct.unpack("<I", data[16:20])[0]
            reserved2 = struct.unpack("<I", data[20:24])[0]

            self.header = {
                "magic": magic,
                "version": version,
                "reserved1": reserved1,
                "entry_offset": entry_offset,
                "code_size": code_size,
                "data_size": data_size,
                "reserved2": reserved2,
            }

            # Validate file size
            expected_size = 24 + code_size + data_size
            if len(data) != expected_size:
                return False

            # Extract sections
            code_start = 24
            if code_size > 0:
                self.code_data = data[code_start : code_start + code_size]
            else:
                self.code_data = b""

            data_start = code_start + code_size
            if data_size > 0:
                self.data_section = data[data_start : data_start + data_size]
            else:
                self.data_section = b""

            self._loaded = True
            return True

        except Exception:
            return False

    @property
    def is_loaded(self) -> bool:
        """Check if object file is successfully loaded"""
        return self._loaded

    def _get_header_field(self, field: str, default=0):
        """Get header field with default value"""
        return self.header[field] if self.header else default

    @property
    def magic(self) -> bytes:
        """Get magic bytes"""
        return self._get_header_field("magic", b"")

    @property
    def version(self) -> int:
        """Get file version"""
        return self._get_header_field("version")

    @property
    def entry_offset(self) -> int:
        """Get entry point offset"""
        return self._get_header_field("entry_offset")

    @property
    def code_size(self) -> int:
        """Get code section size"""
        return self._get_header_field("code_size")

    @property
    def data_size(self) -> int:
        """Get data section size"""
        return self._get_header_field("data_size")

    @property
    def instruction_count(self) -> int:
        """Get number of instructions in code section"""
        return self.code_size // 4 if self.code_size else 0

    def get_instruction_bytes(self, offset: int) -> Optional[bytes]:
        """
        Get 4 bytes for instruction at given offset

        Args:
            offset: Byte offset within code section

        Returns:
            4 bytes for instruction, or None if invalid offset
        """
        if not self.code_data or offset < 0 or offset + 4 > len(self.code_data):
            return None
        return self.code_data[offset : offset + 4]

    def print_info(self):
        """Print object file information"""
        if not self.is_loaded:
            print("No file loaded")
            return

        if self.file_path:
            print(f"IRRE Object File: {self.file_path}")
        else:
            print("IRRE Object File: (from data)")

        print(f"Magic: {self.magic}")
        print(f"Version: {self.version}")
        print(f"Entry offset: 0x{self.entry_offset:x}")
        print(
            f"Code size: {self.code_size} bytes ({self.instruction_count} instructions)"
        )
        print(f"Data size: {self.data_size} bytes")
        print()


def load_object_file(file_path: str) -> Optional[IRREObjectFile]:
    """
    Convenience function to load an object file

    Args:
        file_path: Path to IRRE object file

    Returns:
        IRREObjectFile if successful, None otherwise
    """
    obj_file = IRREObjectFile(file_path)
    return obj_file if obj_file.is_loaded else None
