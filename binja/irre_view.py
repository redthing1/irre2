"""
IRRE2 Binary View for Binary Ninja

This module implements support for IRRE object files (.irre) in Binary Ninja,
providing file format detection, parsing, and setup.
"""

from binaryninja import (
    BinaryView,
    BinaryViewType,
    Architecture,
    SegmentFlag,
    SectionSemantics,
    Symbol,
    SymbolType,
    log_error,
    log_info,
    log_warn,
)
from typing import Optional

from .irre_types import INSTRUCTION_SIZE
from .irre_object import IRREObjectFile


class IRREBinaryView(BinaryView):
    """Binary view for IRRE object files"""

    name = "IRRE"
    long_name = "IRRE Object"

    def __init__(self, data):
        BinaryView.__init__(self, parent_view=data, file_metadata=data.file)
        self.platform = Architecture["irre2"].standalone_platform
        self.obj_file = None

    @classmethod
    def is_valid_for_data(cls, data: BinaryView) -> bool:
        """
        Check if the data represents an IRRE object file

        IRRE object files use the RGVM format with magic bytes "RGVM"
        """
        if data.length < 24:
            return False

        try:
            # Check for RGVM magic bytes
            magic = data.read(0, 4)
            return magic == b"RGVM"
        except Exception:
            return False

    def init(self) -> bool:
        """Initialize the binary view"""
        try:
            # Parse the RGVM object file using shared parser
            raw_data = self.parent_view.read(0, self.parent_view.length)
            self.obj_file = IRREObjectFile(data=raw_data)

            if not self.obj_file.is_loaded:
                log_error("[IRRE] Failed to parse IRRE object file")
                return False

            log_info(
                f"[IRRE] Loaded object file: entry_offset={self.obj_file.entry_offset:x}, "
                f"code_size={self.obj_file.code_size}, data_size={self.obj_file.data_size}"
            )

            # Set up the binary segments and sections
            if not self._setup_segments():
                return False

            # Define symbols if available
            self._define_symbols()

            # Set entry point
            self._set_entry_point()

            return True

        except Exception as e:
            log_error(f"[IRRE] Failed to initialize IRRE binary view: {e}")
            return False

    def _setup_segments(self) -> bool:
        """Set up memory segments for the IRRE program"""
        try:
            # Code segment starts after 24-byte header
            code_file_offset = 24

            if self.obj_file.code_size > 0:
                # Code is mapped starting at address 0x0
                self.add_auto_segment(
                    0x0,  # Virtual address - code starts at 0x0
                    self.obj_file.code_size,
                    code_file_offset,  # File offset after header
                    self.obj_file.code_size,
                    SegmentFlag.SegmentReadable | SegmentFlag.SegmentExecutable,
                )

                # Add code section
                self.add_auto_section(
                    ".text",
                    0x0,
                    self.obj_file.code_size,
                    SectionSemantics.ReadOnlyCodeSectionSemantics,
                )

            # Data segment if present - mapped right after code
            if self.obj_file.data_size > 0:
                data_file_offset = code_file_offset + self.obj_file.code_size
                data_virtual_addr = (
                    self.obj_file.code_size
                )  # Data starts where code ends

                self.add_auto_segment(
                    data_virtual_addr,  # Virtual address right after code
                    self.obj_file.data_size,
                    data_file_offset,  # File offset after code
                    self.obj_file.data_size,
                    SegmentFlag.SegmentReadable | SegmentFlag.SegmentWritable,
                )

                # Add data section
                self.add_auto_section(
                    ".data",
                    data_virtual_addr,
                    self.obj_file.data_size,
                    SectionSemantics.ReadWriteDataSectionSemantics,
                )

            return True

        except Exception as e:
            log_error(f"[IRRE] Failed to setup segments: {e}")
            return False

    def _define_symbols(self):
        """Define symbols - RGVM format doesn't include symbol table"""
        try:
            # Define entry point symbol if there's an entry offset
            if self.obj_file and self.obj_file.entry_offset >= 0:
                entry_symbol = Symbol(
                    SymbolType.FunctionSymbol, self.obj_file.entry_offset, "_start"
                )
                self.define_auto_symbol(entry_symbol)

        except Exception as e:
            log_error(f"[IRRE] Failed to define symbols: {e}")

    def _set_entry_point(self):
        """Set the program entry point"""
        try:
            if self.obj_file and self.obj_file.entry_offset >= 0:
                # Entry offset is relative to code start (which is at address 0x0)
                entry_addr = self.obj_file.entry_offset
                self.add_entry_point(entry_addr)

                # Add function at entry point
                self.add_function(entry_addr)

        except Exception as e:
            log_error(f"[IRRE] Failed to set entry point: {e}")

    def perform_is_executable(self) -> bool:
        """Check if this is an executable file"""
        return True

    def perform_get_entry_point(self) -> int:
        """Get the entry point address"""
        if self.obj_file and self.obj_file.entry_offset >= 0:
            return self.obj_file.entry_offset
        return 0

    def perform_get_address_size(self) -> int:
        return 4  # 32-bit architecture


# Register the binary view
IRREBinaryView.register()
