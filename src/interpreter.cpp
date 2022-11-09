//
// Created by richard may clarkson on 04/11/2022.
//

#include <interpreter.h>
#include <unordered_map>
#include <stack>
#include <cmath>
#include <sstream>
#include <iostream>

using namespace interpreter;

// 0-25 a-z, 26 jump back
stack_t* interpreter::stacks;
const u32 jump_back = 26;
std::vector<statement_t> interpreter::statements;
std::vector<std::pair<std::string, std::pair<u32, u32>>> resolve;
std::vector<std::pair<std::string, std::pair<u32, u32>>> findEndfs;
std::unordered_map<std::string, u32> endfs;
std::unordered_map<std::string, u32> resolutions;

bool interpreter::is_true(ref(data_t) in) {
   switch (in.type) {
      case integer:
         return any_cast<i64>(in.data) != 0;
      case fp:
         return any_cast<f64>(in.data) != 0;
      case str:
         return !((std::string*) in.data)->empty();
      case chr:
         return *(char*) in.data != 0;
      case array:
         return !((array_t*) in.data)->empty();
   }
   return true;
}

node_t transform_tok(ref(lexer::tok_t) tok, u32 statementNum, u32 withinStmt) {
   node_t node { };
   node.type = tok.type;
   switch (tok.type) {
      case lexer::push:
      case lexer::stack:
      case lexer::pop: {
         node.data = new u32(tok.content[0] - 'a');
         break;
      }
      case lexer::beginf:
      case lexer::label: {
         findEndfs.push_back({ tok.content, { statementNum, withinStmt }});
         resolutions[tok.content] = statementNum + 1;
         break;
      }
      case lexer::endf: {
         endfs[tok.content] = statementNum + 1;
         break;
      }
      case lexer::jump: {
         resolve.push_back({ tok.content, { statementNum, withinStmt }});
         break;
      }
      case lexer::str:
      case lexer::id:
         node.data = new std::string(tok.content);
         break;
      case lexer::chr: {
         node.data = new char(tok.content[0]);
         break;
      }
      case lexer::integer: {
         node.data = new i64(std::strtoll(tok.content.c_str(), nullptr, 10));
         break;
      }
      case lexer::fp: {
         node.data = new f64(std::strtod(tok.content.c_str(), nullptr));
         break;
      }
      case lexer::cast: {
         if (tok.content == "int") {
            node.data = new data_type_e(integer);
         } else if (tok.content == "float") {
            node.data = new data_type_e(fp);
         } else if (tok.content == "string") {
            node.data = new data_type_e(str);
         } else if (tok.content == "char") {
            node.data = new data_type_e(chr);
         } else if (tok.content == "array") {
            node.data = new data_type_e(array);
         }
         break;
      }
   }
   return node;
}

typedef runtime_res_t (* convert)(mutref(data_t), lexer::tok_type_e);

convert conversions[] = {
      [](mutref(data_t) dat, lexer::tok_type_e op) -> runtime_res_t {
         // switch statement for above
         switch (dat.type) {
            case data_type_e::chr:
               return { dat, "" };
            case data_type_e::integer:
               return { data_t { chr, new char(any_cast<i64>(dat.data)) }, "" };
            case data_type_e::fp:
               return { data_t { chr, new char(any_cast<f64>(dat.data)) }, "" };
            case data_type_e::str:
               return { data_t { chr, new char(std::strtol(((std::string*) dat.data)->c_str(), nullptr, 10)) },
                        "" };
         }
         return { empty_data_t, "interpreter::convert@chr: cannot convert to char" };
      },
      [](mutref(data_t) dat, lexer::tok_type_e op) -> runtime_res_t {
         switch (dat.type) {
            case data_type_e::chr:
               return { data_t { integer, new i64(*(char*) dat.data) }, "" };
            case data_type_e::integer:
               return { dat, "" };
            case data_type_e::fp:
               return { data_t { integer, new i64(*(f64*) dat.data) }, "" };
            case data_type_e::str:
               return { data_t { integer, new i64(std::strtoll(((std::string*) dat.data)->c_str(), nullptr, 10)) },
                        "" };
         }
         return { empty_data_t, "interpreter::convert@integer: cannot convert to integer" };
      },
      [](mutref(data_t) dat, lexer::tok_type_e op) -> runtime_res_t {
         switch (dat.type) {
            case data_type_e::chr:
               return { data_t { fp, new f64(*(char*) dat.data) }, "" };
            case data_type_e::integer:
               return { data_t { fp, new f64(*(i64*) dat.data) }, "" };
            case data_type_e::fp:
               return { dat, "" };
            case data_type_e::str:
               return { data_t { fp, new f64(std::strtod(((std::string*) dat.data)->c_str(), nullptr)) }, "" };
         }
         return { empty_data_t, "interpreter::convert@fp: cannot convert to fp" };
      },
      [](mutref(data_t) dat, lexer::tok_type_e op) -> runtime_res_t {
         switch (dat.type) {
            case data_type_e::chr:
               return { data_t { str, new std::string(1, *(char*) dat.data) }, "" };
            case data_type_e::integer: {
               if (op == lexer::mul) {
                  return { dat, "" };
               }
               return { data_t { str, new std::string(std::to_string(*(i64*) dat.data)) }, "" };
            }
            case data_type_e::fp:
               return { data_t { str, new std::string(std::to_string(*(f64*) dat.data)) }, "" };
            case data_type_e::str:
               return { dat, "" };
         }
         return { empty_data_t, "interpreter::convert@str: cannot convert to str" };
      },
      [](mutref(data_t) dat, lexer::tok_type_e op) -> runtime_res_t {
         return { dat, "" };
      }
};

void interpreter::reset(ref(std::vector<lexer::tok_t>) tokens) {
   statements.clear();
   statement_t temp;
   u32 i = 0;
   u32 statement = 0;
   u32 withinStmt = 0;
   while (i < tokens.size()) {
      if (tokens[i].type == lexer::endl) {
         if (!temp.empty()) {
            statements.push_back(temp);
            statement++;
            temp = { };
            withinStmt = 0;
         }
         i++;
         continue;
      }
      temp.push_back(transform_tok(tokens[i], statement, withinStmt));
      i++;
      withinStmt++;
   }
   for (ref(auto) it : resolve) {
      statements[it.second.first][it.second.second].data = new u32(resolutions[it.first]);
   }
   resolutions.clear();
   resolve.clear();
   for (ref(auto) it : findEndfs) {
      statements[it.second.first][it.second.second].data = new u32(endfs[it.first]);
   }
   findEndfs.clear();
   endfs.clear();

   interpreter::stacks = new stack_t[32];
   for (int j = 0; j < 32; j++) {
      stack_t stack;
      stacks[j] = stack;
   }
}

typedef runtime_res_t (* operation)(mutref(data_t), mutref(data_t), lexer::tok_type_e);

runtime_res_t basic_op(mutref(data_t) one, mutref(data_t) two, lexer::tok_type_e op) {
   if (one.type != data_type_e::array && two.type == data_type_e::array) {
      return { empty_data_t, "interpreter::basic_op: cannot add array to non-array" };
   }
   data_t help[] = { one, two };
   data_type_e dominant = std::max(one.type, two.type);
   if (op == lexer::idiv) {
      dominant = data_type_e::integer;
      runtime_res_t cv0 = conversions[dominant](help[0], op);
      if (!cv0.second.empty()) {
         return cv0;
      }
      help[0] = cv0.first;
      runtime_res_t cv1 = conversions[dominant](help[1], op);
      if (!cv1.second.empty()) {
         return cv1;
      }
      help[1] = cv1.first;
   } else {
      u32 didx = dominant == help[0].type ? 0 : 1;
      u32 oidx = didx == 0 ? 1 : 0;
      if (dominant == data_type_e::array) {
         auto* arr = (array_t*) help[0].data;
         arr->push_back(two);
         return { one, "" };
      }
      runtime_res_t conversion = conversions[dominant](help[oidx], op);
      if (!conversion.second.empty()) {
         return conversion;
      }
      help[oidx] = conversion.first;
   }
   switch (op) {
      case lexer::add: {
         switch (dominant) {
            case data_type_e::chr:
               return { data_t { chr, new char(*(char*) help[0].data + *(char*) help[1].data) }, "" };
            case data_type_e::integer:
               return { data_t { integer, new i64(*(i64*) help[0].data + *(i64*) help[1].data) }, "" };
            case data_type_e::fp:
               return { data_t { fp, new f64(*(f64*) help[0].data + *(f64*) help[1].data) }, "" };
            case data_type_e::str:
               return { data_t { str, new std::string(*(std::string*) help[0].data + *(std::string*) help[1].data) },
                        "" };
         }
         break;
      }
      case lexer::sub: {
         switch (dominant) {
            case data_type_e::chr:
               return { data_t { chr, new char(*(char*) help[0].data - *(char*) help[1].data) }, "" };
            case data_type_e::integer:
               return { data_t { integer, new i64(*(i64*) help[0].data - *(i64*) help[1].data) }, "" };
            case data_type_e::fp:
               return { data_t { fp, new f64(*(f64*) help[0].data - *(f64*) help[1].data) }, "" };
            default:
               return { empty_data_t, "interpreter::basic_op@sub: cannot subtract non-numbers" };
         }
      }
      case lexer::mul: {
         switch (dominant) {
            case data_type_e::chr:
               return { data_t { chr, new char(*(char*) help[0].data * *(char*) help[1].data) }, "" };
            case data_type_e::integer:
               return { data_t { integer, new i64(*(i64*) help[0].data * *(i64*) help[1].data) }, "" };
            case data_type_e::fp:
               return { data_t { fp, new f64(*(f64*) help[0].data * *(f64*) help[1].data) }, "" };
            case data_type_e::str: {
               auto* str = (std::string*) help[0].data;
               i64 times = *(i64*) help[1].data;
               std::string res;
               for (i64 i = 0; i < times; i++) {
                  res += *str;
               }
               return { data_t { data_type_e::str, new std::string(res) }, "" };
            }
            default:
               return { empty_data_t, "interpreter::basic_op@mul: cannot multiply non-numbers" };
         }
      }
      case lexer::div: {
         switch (dominant) {
            case data_type_e::chr:
               return { data_t { chr, new char(*(char*) help[0].data / *(char*) help[1].data) }, "" };
            case data_type_e::integer:
               return { data_t { integer, new i64(*(i64*) help[0].data / *(i64*) help[1].data) }, "" };
            case data_type_e::fp:
               return { data_t { fp, new f64(*(f64*) help[0].data / *(f64*) help[1].data) }, "" };
            default:
               return { empty_data_t, "interpreter::basic_op@div: cannot divide non-numbers" };
         }
      }
      case lexer::idiv:
         return { data_t { integer, new i64(*(i64*) help[0].data / *(i64*) help[1].data) }, "" };
      case lexer::mod: {
         switch (dominant) {
            case data_type_e::chr:
               return { data_t { chr, new char(*(char*) help[0].data % *(char*) help[1].data) }, "" };
            case data_type_e::integer:
               return { data_t { integer, new i64(*(i64*) help[0].data % *(i64*) help[1].data) }, "" };
            case data_type_e::fp:
               return { data_t { fp, new f64(fmod(*(f64*) help[0].data, *(f64*) help[1].data)) }, "" };
            default:
               return { empty_data_t, "interpreter::basic_op@mod: cannot mod non-numbers" };
         }
      }
   }
   return { empty_data_t, "interpreter::basic_op: invalid operation" };
}

runtime_res_t comp_op(mutref(data_t) one, mutref(data_t) two, lexer::tok_type_e op) {
   std::string error;
   if (op == lexer::eq || op == lexer::neq) {
      if (one.type != two.type) {
         std::cout << one.type << ' ' << two.type << '\n';
         error = "interpreter::handle_comp_op: cannot compare different types";
         goto Tail;
      }
      switch (one.type) {
         case data_type_e::chr:
            if (op == lexer::eq) {
               return { data_t { data_type_e::chr, new char(any_cast<char>(one.data) == any_cast<char>(two.data)) },
                        "" };
            } else {
               return { data_t { data_type_e::chr, new char(any_cast<char>(one.data) != any_cast<char>(two.data)) },
                        "" };
            }
         case data_type_e::integer:
            if (op == lexer::eq) {
               return { data_t { data_type_e::chr, new char(any_cast<i64>(one.data) == any_cast<i64>(two.data)) }, "" };
            } else {
               return { data_t { data_type_e::chr, new char(any_cast<i64>(one.data) != any_cast<i64>(two.data)) }, "" };
            }
         case data_type_e::fp:
            if (op == lexer::eq) {
               return { data_t { data_type_e::chr, new char(any_cast<f64>(one.data) == any_cast<f64>(two.data)) }, "" };
            } else {
               return { data_t { data_type_e::chr, new char(any_cast<f64>(one.data) != any_cast<f64>(two.data)) }, "" };
            }
            break;
         case data_type_e::str:
            if (op == lexer::eq) {
               return { data_t { data_type_e::chr,
                                 new char(any_cast<std::string>(one.data) == any_cast<std::string>(two.data)) }, "" };
            } else {
               return { data_t { data_type_e::chr,
                                 new char(any_cast<std::string>(one.data) != any_cast<std::string>(two.data)) }, "" };
            }
            break;
         case data_type_e::array:
            if (op == lexer::eq) {
               return { data_t { data_type_e::chr, new char(one.data == two.data) }, "" };
            } else {
               return { data_t { data_type_e::chr, new char(one.data != two.data) }, "" };
            }
            break;
      }
   }
   if (one.type > 2 || two.type > 2) {
      error = "interpreter::handle_comp_op: one of the two arguments was not a number; cannot compare";
      goto Tail;
   }

   f64 f1, f2;
   switch (one.type) {
      case data_type_e::integer:
         f1 = (f64) any_cast<i64>(one.data);
         break;
      case data_type_e::fp:
         f1 = any_cast<f64>(one.data);
         break;
      case data_type_e::chr:
         f1 = (f64) any_cast<char>(one.data);
         break;
      default:
         error = "interpreter::handle_comp_op: first argument was not a number; cannot compare";
         goto Tail;
   }
   switch (two.type) {
      case data_type_e::integer:
         f2 = (f64) any_cast<i64>(two.data);
         break;
      case data_type_e::fp:
         f2 = any_cast<f64>(two.data);
         break;
      case data_type_e::chr:
         f2 = (f64) any_cast<char>(two.data);
         break;
      default:
         error = "interpreter::handle_comp_op: second argument was not a number; cannot compare";
         goto Tail;
   }

   switch (op) {
      case lexer::gt:
         one = data_t { data_type_e::integer, new i64(f1 > f2) };
         break;
      case lexer::lt:
         one = data_t { data_type_e::integer, new i64(f1 < f2) };
         break;
      case lexer::gte:
         one = data_t { data_type_e::integer, new i64(f1 >= f2) };
         break;
      case lexer::lte:
         one = data_t { data_type_e::integer, new i64(f1 <= f2) };
         break;
      default:
         error = "interpreter::handle_comp_op: invalid comparison operator";
         goto Tail;
   }

   Tail:
   return { empty_data_t, error };
}

runtime_res_t logic_op(mutref(data_t) one, mutref(data_t) two, lexer::tok_type_e op) {
   switch (op) {
      case lexer::and_: return { data_t { data_type_e::chr, new char(is_true(one) && is_true(two)) }, "" };
      case lexer::or_: return { data_t { data_type_e::chr, new char(is_true(one) || is_true(two)) }, "" };
      case lexer::xor_: return { data_t { data_type_e::chr, new char(is_true(one) != is_true(two)) }, "" };
      default: return { empty_data_t, "interpreter::bool_op: invalid operator" };
   }
}

runtime_res_t node_to_data(ref(node_t) in) {
   switch (in.type) {
      case lexer::integer:
         return { data_t { data_type_e::integer, in.data }, "" };
      case lexer::chr:
         return { data_t { data_type_e::chr, in.data }, "" };
      case lexer::fp:
         return { data_t { data_type_e::fp, in.data }, "" };
      case lexer::str:
         return { data_t { data_type_e::str, in.data }, "" };
      case lexer::array:
         return { data_t { data_type_e::array, in.data }, "" };
      case lexer::stack:
         stack_t stack = interpreter::stacks[any_cast<u32>(in.data)];
         if (stack.empty()) {
            return { empty_data_t, "interpreter::node_to_data: stack is empty" };
         }
         return { stack.top(), "" };
   }
   return { empty_data_t, "interpreter::node_to_data: invalid node type" };
}

runtime_res_t negate(ref(data_t) in) {
   if (in.type == data_type_e::integer) {
      return { data_t { data_type_e::integer, new i64(-any_cast<i64>(in.data)) }, "" };
   }
   if (in.type == data_type_e::fp) {
      return { data_t { data_type_e::fp, new f64(-any_cast<f64>(in.data)) }, "" };
   }
   return { empty_data_t, "interpreter::negate: tried to negate non-number" };
}

runtime_res_t not_(ref(data_t) in) {
   if (is_true(in)) {
      return { data_t { data_type_e::chr, new char(0) }, "" };
   }
   return { data_t { data_type_e::chr, new char(1) }, "" };
}

runtime_res_t handle_op(ref(statement_t) stmt, lexer::tok_type_e op, operation todo) {
   std::string error;
   // pointers for the two values to be used in the operation
   data_t v1;
   data_t v2;
   u32 sidx = 1;

   {
      // get the first value, negate it if necessary
      if (stmt[0].type == lexer::sub) {
         sidx++;
         runtime_res_t cv0 = node_to_data(stmt[1]);
         if (!cv0.second.empty()) {
            error = cv0.second;
            goto Tail;
         }
         runtime_res_t negated = negate(cv0.first);
         v1 = negated.first;
      } else {
         runtime_res_t cv0 = node_to_data(stmt[0]);
         if (!cv0.second.empty()) {
            error = cv0.second;
            goto Tail;
         }
         v1 = cv0.first;
      }

      // get the second value, negate it if necessary
      if (stmt[sidx - 1].type == lexer::sub) {
         runtime_res_t cv0 = node_to_data(stmt[1]);
         if (!cv0.second.empty()) {
            error = cv0.second;
            goto Tail;
         }
         runtime_res_t negated = negate(cv0.first);
         v2 = negated.first;
      } else {
         runtime_res_t cv0 = node_to_data(stmt[1]);
         if (!cv0.second.empty()) {
            error = cv0.second;
            goto Tail;
         }
         v2 = cv0.first;
      }

      // get the reg to store to & set top as the value
      runtime_res_t res = todo(v1, v2, op);
      if (!res.second.empty()) {
         error = res.second;
         goto Tail;
      }
      node_t stack = stmt[stmt.size() - 2];
      if (stack.type != lexer::stack) {
         error = "interpreter::run@basic_op: last argument was not a stack";
         goto Tail;
      }
      u32 idx = any_cast<u32>(stack.data);
      if (interpreter::stacks[idx].empty()) {
         error = "interpreter::run@basic_op: specified stack is empty";
         goto Tail;
      }
      interpreter::stacks[idx].top() = res.first;
   }

   Tail:
   return { empty_data_t, error };
}

std::string to_string(ref(data_t) in) {
   switch (in.type) {
      case data_type_e::str:
         return any_cast<std::string>(in.data);
      case data_type_e::chr:
         return std::to_string(any_cast<char>(in.data));
      case data_type_e::integer:
         return std::to_string(any_cast<i64>(in.data));
      case data_type_e::fp:
         return std::to_string(any_cast<f64>(in.data));
      case data_type_e::array: {
         std::stringstream ss;
         ss << '[';
         u32 idx = 0;
         auto arr = any_cast<array_t>(in.data);
         for (data_t dat : arr) {
            ss << to_string(dat);
            if (idx != arr.size() - 1) {
               ss << ", ";
            }
            idx++;
         }
         ss << ']';
         return ss.str();
      }
   }
}

u32 current = 0;

runtime_res_t interpreter::run() {
   std::string error;
   while (current < statements.size()) {
      statement_t stmt = statements[current];
      lexer::tok_type_e op = stmt[stmt.size() - 1].type;
      switch (op) {
         case lexer::add:
         case lexer::sub:
         case lexer::mul:
         case lexer::div:
         case lexer::idiv:
         case lexer::mod: {
            runtime_res_t res = handle_op(stmt, op, basic_op);
            if (!res.second.empty()) {
               error = res.second;
               goto Tail;
            }
            current++;
            break;
         }
         case lexer::eq:
         case lexer::neq:
         case lexer::gt:
         case lexer::lt:
         case lexer::gte:
         case lexer::lte: {
            runtime_res_t res = handle_op(stmt, op, comp_op);
            if (!res.second.empty()) {
               error = res.second;
               goto Tail;
            }
            current++;
            break;
         }
         case lexer::and_:
         case lexer::or_:
         case lexer::xor_: {
            runtime_res_t res = handle_op(stmt, op, logic_op);
            if (!res.second.empty()) {
               error = res.second;
               goto Tail;
            }
            current++;
            break;
         }
         case lexer::print: {
            for (int i = 0; i < stmt.size() - 1; i++) {
               runtime_res_t res = node_to_data(stmt[i]);
               if (!res.second.empty()) {
                  error = res.second;
                  goto Tail;
               }
               data_t dat = res.first;
               std::cout << to_string(dat);
            }
            current++;
            break;
         }
         case lexer::stack: {
            // replace top value of stack with constant
            runtime_res_t dat = node_to_data(stmt[0]);
            if (!dat.second.empty()) {
               error = dat.second;
               goto Tail;
            }
            u32 idx = any_cast<u32>(stmt[stmt.size() - 1].data);
            if (stacks[idx].empty()) {
               error = "interpreter::run@stack: stack trying to be set is empty!";
               goto Tail;
            }
            interpreter::stacks[idx].pop();
            interpreter::stacks[idx].push(dat.first);
            current++;
            break;
         }
         case lexer::push: {
            // new slot in stack
            interpreter::stacks[any_cast<u32>(stmt[0].data)].push(data_t { });
            current++;
            break;
         }
         case lexer::pop: {
            // remove top value of stack
            interpreter::stacks[any_cast<u32>(stmt[0].data)].pop();
            current++;
            break;
         }
         case lexer::jump: {
            runtime_res_t dat;
            bool neg = false;
            if (stmt[0].type == lexer::not_) { // handle negation
               neg = true;
            }
            dat = node_to_data(stmt[neg]);
            if (neg) {
               dat = not_(dat.first);
            }
            if (!dat.second.empty()) {
               error = dat.second;
               goto Tail;
            }
            if (is_true(dat.first)) {
               interpreter::stacks[jump_back].push({ data_type_e::integer, new u32(current + 1) });
               current = any_cast<u32>(stmt[stmt.size() - 1].data);
            } else {
               current++;
            }
            break;
         }
         case lexer::beginf: {
            // jump to after the corresponding endf
            u32 endfLoc = any_cast<u32>(stmt[0].data);
            current = endfLoc;
            break;
         }
         case lexer::endf: {
            // jump to the top of the jumpback stack & pop it
            if (interpreter::stacks[jump_back].empty()) {
               error = "interpreter@endf: jump_back stack is empty!";
               goto Tail;
            }
            current = any_cast<u32>(interpreter::stacks[jump_back].top().data);
            interpreter::stacks[jump_back].pop();
            break;
         }
         case lexer::cast: {
            runtime_res_t dat = node_to_data(stmt[0]);
            if (!dat.second.empty()) {
               error = dat.second;
               goto Tail;
            }
            data_type_e type = any_cast<data_type_e>(stmt[2].data);
            runtime_res_t res = conversions[type](dat.first, lexer::nop);
            if (!res.second.empty()) {
               error = res.second;
               goto Tail;
            }
            interpreter::stacks[any_cast<u32>(stmt[1].data)].top() = res.first;
            current++;
            break;
         }
         case lexer::read: {
            std::string in;
            std::cin >> in;
            data_t dat = { data_type_e::str, new std::string(in) };
            interpreter::stacks[any_cast<u32>(stmt[0].data)].top() = dat;
            current++;
            break;
         }
         default:
            current++;
      }
   }
   Tail:
   return { data_t { }, error };
}