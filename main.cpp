#include <iostream>
#include <lexer.h>
#include <interpreter.h>
#include <fstream>
#include <sstream>

int main() {
   std::ifstream file("test.stack");
   std::stringstream buf;
   clock_t entire = clock();
   buf << file.rdbuf();
   lexer::reset(buf.str());
   auto toks = lexer::lex();
   file.close();
   interpreter::reset(toks);
   clock_t runtime = clock();
   interpreter::runtime_res_t res = interpreter::run();
   clock_t now = clock();
   if (!res.second.empty()) {
      std::cout << res.second;
      return 1;
   }
   std::cout << '\n' << "completed successfully (lex+runtime: " << now - entire << ", runtime: " << now - runtime << ")";
   return 0;
}
