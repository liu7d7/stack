//
// Created by richard may clarkson on 03/11/2022.
//

#ifndef STACK_LEXER_H
#define STACK_LEXER_H

#include <string>
#include <global.h>
#include <vector>

namespace lexer {
   // token type
   enum tok_type_e {
      endl,
      add,
      sub,
      mul,
      div,
      idiv,
      gt,
      gte,
      lt,
      lte,
      push,
      pop,
      beginf,
      endf,
      print,
      jump,
      id,
      stack,
      str,
      chr,
      integer,
      fp,
      eq,
      neq,
      read,
      comma,
      dot,
      begina,
      enda,
      colon,
      label
   };

   inline std::string to_string(tok_type_e in) {
      return (std::string[]) {
         "endl", "add", "sub", "mul", "div", "idiv", "gt", "gte", "lt", "lte", "push", "pop",
         "beginf", "endf", "print", "jump", "id", "stack", "str", "chr", "integer", "fp", "eq",
         "neq", "read", "comma", "dot", "begina", "enda", "colon", "label"
      }[in];
   }

   // file position
   struct file_pos_t {
      u32 sLine, eLine, sCol, eCol, idx;
   };

   // token
   struct tok_t {
      tok_type_e type;
      std::string content;
      file_pos_t filePos;
   };

   // contents of file currently being lexed
   extern std::string file, error;
   // current position of lexer
   extern file_pos_t pos;
   // current char, next char
   extern char prev, cur, next;

   // reset the lexer with file content
   void reset(ref(std::string));
   // advances the lexer by one character
   bool advance();
   // tokenizes the file that has been reset(ref(std::string))'d
   std::vector<tok_t> lex();
   // copies a file_pos_t struct
   file_pos_t copy_pos(ref(file_pos_t));
   // ends the start pos with the end pos
   file_pos_t start_end_pos(ref(file_pos_t) start, ref(file_pos_t) end);
   // to_string for lexer structs
   std::string to_string(ref(file_pos_t));
   std::string to_string(ref(tok_t));
   // is this identifier the id of a stack?
   bool is_stack(ref(std::string) id);
}

#endif //STACK_LEXER_H
