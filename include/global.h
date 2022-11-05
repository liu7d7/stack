//
// Created by richard may clarkson on 03/11/2022.
//

#ifndef STACK_GLOBAL_H
#define STACK_GLOBAL_H

// makes a const reference
#define ref(x) const x&
#define mutref(x) x&

// utility functions
inline std::string replace_all(ref(std::string) str, ref(std::string) from, ref(std::string) to) {
   size_t start_pos = 0;
   std::string copy = str;
   while((start_pos = str.find(from, start_pos)) != std::string::npos) {
      copy.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
   }
   return copy;
}

// general purpose typedefs, so I don't have to type out uint32_t and such
typedef unsigned long long u64;
typedef unsigned int u32;
typedef long long i64;
typedef int i32;
typedef double f64;
typedef float f32;

#endif //STACK_GLOBAL_H
