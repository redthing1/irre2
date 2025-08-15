#pragma once

#include <tao/pegtl.hpp>
#include "../arch/types.hpp"

namespace irre::assembler::grammar {

namespace pegtl = tao::pegtl;
using namespace pegtl;

// basic tokens
struct ws : plus<blank> {};
struct opt_ws : star<blank> {};
struct comment : seq<one<';'>, until<eolf>> {};

// basic tokens
struct label_name : identifier {};
struct label_def : seq<label_name, one<':'>> {};
struct hex_number : seq<one<'$'>, opt<one<'-'>>, plus<xdigit>> {};
struct dec_number : seq<opt<one<'#'>>, opt<one<'-'>>, plus<digit>> {};
struct number : sor<hex_number, dec_number> {};
struct operand : sor<identifier, number> {};

// directives
struct directive_entry : seq<one<'%'>, string<'e', 'n', 't', 'r', 'y'>, opt_ws, one<':'>, opt_ws, identifier> {};
struct directive_section : seq<one<'%'>, string<'s', 'e', 'c', 't', 'i', 'o', 'n'>, ws, identifier> {};
struct directive_data : seq<one<'%'>, string<'d'>, ws, until<eol>> {};

struct directive : sor<directive_entry, directive_section, directive_data> {};

// generic instruction: mnemonic followed by operands
struct instruction : seq<identifier, star<seq<ws, operand>>> {};

// assembly item can be a label definition, instruction, or directive
struct asm_line : sor<label_def, directive, instruction> {};

// whitespace that includes newlines
struct space_or_comment : sor<space, comment> {};

// program structure: asm_line items separated by whitespace/comments, ending with eof
struct program : seq<star<space_or_comment>, star<seq<asm_line, star<space_or_comment>>>, eof> {};

} // namespace irre::assembler::grammar