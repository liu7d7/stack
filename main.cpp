#include <iostream>
#include <lexer.h>
#include <fstream>
#include <sstream>

int main() {
   std::ifstream file("syntax.stack");
   std::stringstream buf;
   buf << file.rdbuf();
   std::cout << buf.str() << std::endl;
   lexer::reset(buf.str());
   for (ref(lexer::tok_t) tok : lexer::lex()) {
      std::cout << lexer::to_string(tok) << '\n';
   }
   file.close();
   return 0;
}
