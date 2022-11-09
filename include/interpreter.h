//
// Created by richard may clarkson on 04/11/2022.
//

#ifndef STACK_INTERPRETER_H
#define STACK_INTERPRETER_H

#include <vector>
#include <lexer.h>
#include <stack>

namespace interpreter {
   struct node_t {
      lexer::tok_type_e type;
      void* data;
   };

   enum data_type_e {
      chr,
      integer,
      fp,
      str,
      array
   };

   struct data_t {
      data_type_e type;
      void* data;
   };
   const data_t empty_data_t = data_t {};

   typedef std::vector<data_t> array_t;
   typedef std::stack<data_t> stack_t;
   typedef std::vector<node_t> statement_t;
   typedef std::pair<data_t, std::string> runtime_res_t;

   extern stack_t * stacks;
   extern std::vector<statement_t> statements;

   bool is_true(ref(data_t));
   void reset(ref(std::vector<lexer::tok_t>));
   runtime_res_t run();
}

#endif //STACK_INTERPRETER_H
