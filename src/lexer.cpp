//
// Created by richard may clarkson on 03/11/2022.
//

#include <lexer.h>
#include <sstream>
#include <iostream>
#include <unordered_map>

using namespace lexer;

std::string lexer::file, lexer::error;
file_pos_t lexer::pos;
char lexer::prev, lexer::cur, lexer::next;
static std::unordered_map<char, tok_type_e> opMap = {
      {';', tok_type_e::endl},
      {'\n', tok_type_e::endl},
      {'$', tok_type_e::print},
      {'*', tok_type_e::mul},
      {'+', tok_type_e::add},
      {'-', tok_type_e::sub},
      {'?', tok_type_e::read},
      {',', tok_type_e::comma},
      {'.', tok_type_e::dot},
      {'[', tok_type_e::begina},
      {']', tok_type_e::enda},
      {'%', tok_type_e::mod},
      {'!', tok_type_e::not_},
      {'&', tok_type_e::and_},
      {'|', tok_type_e::or_},
      {'#', tok_type_e::xor_},
};

std::string lexer::to_string(ref(file_pos_t) in) {
   std::stringstream ss;
   ss << "file_pos_t{sLine:" << in.sLine << ", eLine:" << in.eLine
      << ", sCol:" << in.sCol << ", eCol:" << in.eCol << ", idx:" << in.idx << "}";
   return ss.str();
}

std::string lexer::to_string(ref(tok_t) in) {
   std::stringstream ss;
   ss << "tok_t{type:" << to_string(in.type) << ", content:" << in.content << ", filePos:" << to_string(in.filePos) << "}";
   return ss.str();
}

file_pos_t lexer::copy_pos(ref(file_pos_t) in) {
   return file_pos_t { .sLine = in.sLine, .eLine = in.eLine, .sCol = in.sCol, .eCol = in.eCol, .idx = in.idx };
}

file_pos_t lexer::start_end_pos(ref(file_pos_t) start, ref(file_pos_t) end) {
   return file_pos_t { .sLine = start.sLine, .eLine = end.eLine, .sCol = start.sCol, .eCol = end.eCol, .idx = start.idx };
}

bool lexer::is_stack(ref(std::string) id) {
   return id.size() == 1 && std::isalpha(id[0]) && std::islower(id[0]);
}

bool firstRun = false;

void lexer::reset(ref(std::string) content) {
   firstRun = true;
   // replace all '\r' and all "\r\n" with '\n'
   std::stringstream ss;
   for (int i = 0; i < content.size(); i++) {
      if (content[i] == '\r' && i != content.size() - 1) {
         if (content[i + 1] != '\n') {
            ss << '\n';
         }
         continue;
      }
      ss << content[i];
   }
   file = ss.str();
   // starting pos
   pos = file_pos_t { .sLine = 1, .eLine = 1, .sCol = 1, .eCol = 1, .idx = 0 };
   advance();
}

bool lexer::advance() {
   if (firstRun) {
      firstRun = false;
      prev = cur;
      cur = file[pos.idx];
      next = pos.idx + 1 >= file.size() ? '\0' : file[pos.idx + 1];
      return true;
   }
   pos.idx++;
   if (pos.idx >= file.size()) {
      prev = cur;
      next = cur = '\0';
      return false;
   }
   prev = cur;
   cur = file[pos.idx];
   next = pos.idx + 1 >= file.size() ? '\0' : file[pos.idx + 1];
   pos.sCol++;
   pos.eCol++;
   if (prev == '\n') {
      pos.sCol = pos.eCol = 1;
      pos.sLine++;
      pos.eLine++;
   }
   return true;
}

i32 type_of(char c) {
   if (opMap.count(c) != 0) {
      return opMap[c];
   }
   if (isalpha(c)) {
      return tok_type_e::id;
   }
   return -1;
}

std::pair<std::string, file_pos_t> get_id() {
   std::stringstream ss;
   file_pos_t start = copy_pos(pos);
   while (isalnum(cur)) {
      ss << cur;
      advance();
   }
   return { ss.str(), start_end_pos(start, pos) };
}

tok_t handle_gt() {
   file_pos_t start = copy_pos(pos);
   advance();
   switch(type_of(cur)) {
      case -1:
         if (cur == '=') {
            return tok_t { .type = tok_type_e::gte, .content = ">=", .filePos = start_end_pos(start, pos) };
         }
         return tok_t { .type = tok_type_e::gt, .content = ">", .filePos = start_end_pos(start, pos) };
      case tok_type_e::id:
         std::pair<std::string, file_pos_t> ident = get_id();
         if (is_stack(ident.first)) {
            return tok_t { .type = tok_type_e::push, .content = ident.first, .filePos = ident.second };
         }
         return tok_t { .type = tok_type_e::beginf, .content = ident.first, .filePos = ident.second };
   }
   error = "lexer::handle_gt: char after '>' was neither a stack, id, nor a newline or ';', got " + std::to_string(cur);
   return tok_t {};
}

tok_t handle_lt() {
   file_pos_t start = copy_pos(pos);
   advance();
   switch(type_of(cur)) {
      case -1:
         if (cur == '>') {
            advance();
            return tok_t { .type = tok_type_e::neq, .content = "<>", .filePos = start_end_pos(start, pos) };
         }
         if (cur == '=') {
            return tok_t { .type = tok_type_e::lte, .content = "<=", .filePos = start_end_pos(start, pos) };
         }
         return tok_t { .type = tok_type_e::lt, .content = "<", .filePos = start_end_pos(start, pos) };
      case tok_type_e::id:
         std::pair<std::string, file_pos_t> ident = get_id();
         if (is_stack(ident.first)) {
            return tok_t { .type = tok_type_e::pop, .content = ident.first, .filePos = ident.second };
         }
         return tok_t { .type = tok_type_e::endf, .content = ident.first, .filePos = ident.second };
   }
   error = "lexer::handle_lt: char after '<' was neither a stack, id, nor a newline or ';'";
   return tok_t {};
}

tok_t handle_eq() {
   file_pos_t start = copy_pos(pos);
   advance();
   if (cur == '=') {
      advance();
      return tok_t { .type = tok_type_e::eq, .content = "==", .filePos = start_end_pos(start, pos) };
   }
   error = "lexer::handle_eq: char after '=' wasn't a '='";
   return tok_t {};
}

tok_t handle_div() {
   file_pos_t start = copy_pos(pos);
   advance();
   if (cur == '/') {
      advance();
      return tok_t { .type = tok_type_e::idiv, .content = "//", .filePos = start_end_pos(start, pos) };
   }
   return tok_t { .type = tok_type_e::div, .content = "/", .filePos = start_end_pos(start, pos) };
}

tok_t handle_str() {
   file_pos_t start = copy_pos(pos);
   advance();
   std::stringstream ss;
   while (cur != '"') {
      if (cur == '\\') {
         advance();
         switch (cur) {
            case 'a':
               ss << '\a';
               break;
            case 'b':
               ss << '\b';
               break;
            case 'f':
               ss << '\f';
               break;
            case 'n':
               ss << '\n';
               break;
            case 'r':
               ss << '\r';
               break;
            case 't':
               ss << '\t';
               break;
            case 'v':
               ss << '\v';
               break;
            case '"':
               ss << '"';
               break;
            case '\\':
               ss << '\\';
               break;
            default:
               error = "lexer::handle_str: unexpected escape sequence (\\" + std::to_string(cur) + ")";
               return tok_t {};
         }
         advance();
         continue;
      }
      ss << cur;
      advance();
   }
   advance();
   return tok_t { .type = tok_type_e::str, .content = ss.str(), .filePos = start_end_pos(start, pos) };
}

tok_t handle_chr() {
   file_pos_t start = copy_pos(pos);
   advance();
   char ch = cur;
   advance();
   if (ch == '\\') {
      switch (cur) {
         case 'a':
            ch = '\a';
            break;
         case 'b':
            ch = '\b';
            break;
         case 'f':
            ch = '\f';
            break;
         case 'n':
            ch = '\n';
            break;
         case 'r':
            ch = '\r';
            break;
         case 't':
            ch = '\t';
            break;
         case 'v':
            ch = '\v';
            break;
         case '"':
            ch = '"';
            break;
         case '\\':
            ch = '\\';
            break;
         default:
            error = "lexer::handle_chr: unexpected escape sequence (\\" + std::to_string(cur) + ")";
            return tok_t {};
      }
      advance();
   }
   if (cur != '\'') {
      error = "lexer::handle_chr: expected \"'\", got (" + std::to_string(cur) + ")";
      return tok_t {};
   }
   advance();
   return tok_t { .type = tok_type_e::chr, .content = std::to_string(ch), .filePos = start_end_pos(start, pos) };
}

tok_t handle_jmp() {
   advance();
   std::pair<std::string, file_pos_t> ident = get_id();
   if (ident.first.empty()) {
      error = "lexer::handle_jmp: expected identifier after jmp, got (" + std::to_string(cur) + ")";
      return tok_t {};
   }
   return tok_t { .type = tok_type_e::jump, .content = ident.first, .filePos = ident.second };
}

tok_t handle_cast() {
   advance();
   std::pair<std::string, file_pos_t> ident = get_id();
   if (ident.first.empty() || cur != ')') {
      error = "lexer::handle_cast: expected identifier | ')' after '(', got '" + std::string(1, cur) + "'";
      return tok_t {};
   }
   advance();
   return tok_t { .type = tok_type_e::cast, .content = ident.first, .filePos = ident.second };
}

void handle_comment() {
   advance();
   while (cur != '`') {
      advance();
   }
   advance();
   if (cur == '\n') {
      advance();
   }
}

void sanitize(mutref(std::vector<tok_t>) toks) {
   for (mutref(tok_t) tok : toks) {
      if (tok.type == lexer::endl)
         tok.content = replace_all(tok.content, "\n", "endl");
   }
}

std::vector<tok_t> lexer::lex() {
   std::vector<tok_t> toks;
   while (cur != '\0') {
      switch (cur) {
         case '`':
            // comment
            handle_comment();
            break;
         case '>':
            // push, beginf
            toks.push_back(handle_gt());
            break;
         case '<':
            // pop, endf, neq
            toks.push_back(handle_lt());
            break;
         case '=':
            // eq
            toks.push_back(handle_eq());
            break;
         case '/':
            // div, idiv
            toks.push_back(handle_div());
            break;
         case '"':
            // str
            toks.push_back(handle_str());
            break;
         case '\'':
            // chr
            toks.push_back(handle_chr());
            break;
         case '^':
            // jmp
            toks.push_back(handle_jmp());
            break;
         case '(':
            // cast
            toks.push_back(handle_cast());
            break;
         default:
            file_pos_t start = copy_pos(pos);
            if (cur == ' ' || cur == '\t') {
               advance();
               goto Tail;
            }
            if (opMap.count(cur) != 0) {
               toks.push_back(tok_t { .type = opMap[cur], .content = std::string(1, cur), .filePos = start_end_pos(start, pos) });
               advance();
               goto Tail;
            }
            if (isalpha(cur)) {
               std::pair<std::string, file_pos_t> id = get_id();
               if (cur == ':') {
                  advance();
                  toks.push_back(tok_t { .type = tok_type_e::label, .content = id.first, .filePos = id.second });
                  goto Tail;
               } else if (is_stack(id.first)) {
                  toks.push_back(tok_t { .type = tok_type_e::stack, .content = id.first, .filePos = id.second });
                  goto Tail;
               } else {
                  toks.push_back(tok_t { .type = tok_type_e::id, .content = id.first, .filePos = id.second });
                  goto Tail;
               }
            } else if (std::isdigit(cur)) { // handle_num
               std::stringstream ss;
               bool fp = false;
               while (std::isdigit(cur) || cur == '.') {
                  if (cur == '.') {
                     fp = true;
                  }
                  ss << cur;
                  advance();
               }
               std::string res = ss.str();
               if (res[res.size() - 1] == '.') {
                  error = "lexer::lex@handle_num: number ended with '.'";
               }
               if (fp) {
                  toks.push_back(tok_t { .type = tok_type_e::fp, .content = res, .filePos = start_end_pos(start, pos) });
                  goto Tail;
               } else {
                  toks.push_back(tok_t { .type = tok_type_e::integer, .content = res, .filePos = start_end_pos(start, pos) });
                  goto Tail;
               }
            }
            Tail:
            break;
      }
      if (!error.empty()) {
         std::cout << error << '@' << to_string(pos) << '\n';
         return toks;
      }
   }
   toks.push_back(tok_t { .type = tok_type_e::endl, .content = "\n", .filePos = pos });
   sanitize(toks);
   return toks;
}