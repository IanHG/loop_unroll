#!/usr/bin/python2

#################################################################################
#
# Generate code for pointer functions that collects a result, e.g. a dot,
# in a way such that chain dependency is minimized.
#
#################################################################################

import optparse
import copy

debug = False

#
#
#
def generate_header_file(file, filename, function_name):
   function_name_upper = function_name.upper()

   file.write("#pragma once\n")
   file.write("#ifndef " + function_name_upper + "_HPP_INCLUDED\n")
   file.write("#define " + function_name_upper + "_HPP_INCLUDED\n")
   file.write("\n")
   file.write("#include \"" + filename + "_decl.hpp\"\n")
   file.write("#include \"" + filename + "_impl.hpp\"\n")
   file.write("\n")
   file.write("#endif /* " + function_name_upper + "_HPP_INCLUDED */\n")


#
#
#
def generate_guard_top(file, function_name, name):
   function_name_upper = function_name.upper()

   file.write("#pragma once\n")
   file.write("#ifndef " + function_name_upper + (name.empty() if "" else "_" + name) + "_HPP_INCLUDED\n")
   file.write("#define " + function_name_upper + (name.empty() if "" else "_" + name) + "_HPP_INCLUDED\n")
   file.write("\n")

#
#
#
def generate_guard_bottom(file, function_name, name):
   function_name_upper = function_name.upper()

   file.write("#endif /* " + function_name_upper + "_" + name + "_HPP_INCLUDED */\n")

#
#
#
def generate_header_code(file, order, function_name, arguments):
   file.write("/* sum a pointer using " + str(order) + "-unrolling */\n")
   file.write("template<class T>\n")
   file.write("T " + function_name + str(order) + "(")
   for arg in arguments:
      file.write("const T* const __restrict " + arg + ", ")
   file.write("int size);\n")
   file.write("\n")

#
#
#
def generate_header_wrapper_code(file, order, function_name, arguments, the_range):
   file.write("\n")
   file.write("#include <climits> /* for CHAR_BIT */\n")
   file.write("\n")
   
   file.write("#if !defined(CACHELINE)\n")
   file.write("#define CACHELINE 256\n")
   file.write("#endif /* CACHELINE */\n")
   file.write("\n")

   file.write("/* sum a pointer */\n")
   file.write("template<class T>\n")
   file.write("inline T " + function_name + "(")
   for arg in arguments:
      file.write("const T* const __restrict " + arg + ", ")
   file.write("int size)\n")
   file.write("{\n")
   file.write("   T result;\n")
   file.write("\n")
   #for i in range(1, order + 1):
   for i in the_range:
      file.write("   ")
      if i == 1:
         file.write("if")
      else:
         file.write("else if")
      file.write(" constexpr ")
      file.write("((sizeof(T) * CHAR_BIT * " + str(i) + ") == CACHELINE)\n")
      file.write("   {\n")
      if debug:
         file.write("      std::cout << \"running " + str(i) + "\" << std::endl;\n")
      file.write("      result = " + function_name + str(i) + "(")
      for arg in arguments:
         file.write(arg + ", ")
      file.write("size);\n")
      file.write("   }\n")
   
   file.write("   else\n")
   file.write("   {\n")
   file.write("      #if defined(NO_HIT_HARD_ERROR)\n")
   file.write("      struct type_sink {};\n")
   file.write("      static_assert(std::is_same_v<T, type_sink>, \"'" + function_name + "': nothing that fits cacheline.\");\n")
   file.write("      #else\n")
   file.write("      /* run the most optimal function we have generated */\n")
   file.write("      result = " + function_name + str(order) + "(")
   for arg in arguments:
      file.write(arg + ", ")
   file.write("size);\n")
   file.write("      #endif /* NO_HIT_HARD_ERROR */\n")
   file.write("   }\n")
   
   file.write("\n")
   file.write("   return result;\n")
   file.write("}\n\n")

#
#
#
def generate_code_for_branching(file, order, function_name, arguments, operation_str):
   """Function definition"""
   function_name_upper = function_name.upper()

   if isinstance(operation_str, str):
      operation_str = [ operation_str ]

   def operation(oper_str, rindex, pindex):
      operation_str_local = oper_str
      if rindex != -1:
         operation_str_local = operation_str_local.replace("result", "result_array")
         operation_str_local = operation_str_local.replace("<ridx>", str(rindex))
      else:
         operation_str_local = operation_str_local.replace("[<ridx>]", "")
         operation_str_local = operation_str_local.replace("<ridx>", "")
      operation_str_local = operation_str_local.replace("<pidx>", str(pindex))
      if not operation_str_local.endswith(";"):
         operation_str_local = operation_str_local + ";"
      return operation_str_local
   
   register  = ""
   #register  = "register "

   file.write("/**\n")
   file.write(" * "+ function_name + str(order) + "\n")
   file.write(" **/\n")
   file.write( "template<class T>\n"
               "T "+ function_name + str(order) + "(")
   for arg in arguments:
      file.write("const T* const __restrict " + arg + ", ")
   file.write("int size)\n")
   file.write(
               "{\n"
             )

   if order != 1:
      #for i in range(0, order):
      file.write("   " + register + "T result_array[" + str(order) + "] = {static_cast<T>(0.0)")
      for i in range(1, order):
         file.write(", static_cast<T>(0.0)")
      file.write("};\n")
      
      file.write("\n")
      file.write("   auto modulo = size % " + str(order) + ";\n")
      file.write("\n")

      for i in range(0, order):
         if i == 0:
            file.write("   if")
         elif i == order - 1:
            file.write("   else\n")
         else:
            file.write("   else if")

         if i != order - 1:
            file.write("(modulo == " + str(i) + ")\n")

         file.write("   {\n")

         for j in range(i, 0, -1):
            file.write("      auto sizem" + str(j) + " = size - " + str(j) + ";\n")

         if i == 0:
            file.write("      for(int i = 0; i < size; i += " + str(order) + ")\n")
         else:
            file.write("      for(int i = 0; i < sizem" + str(i) + "; i += " + str(order) + ")\n")
         file.write("      {\n")

         for oper_str in operation_str:
            file.write("         " + operation(oper_str, str(0), "i") + "\n")
            for k in range(1, order):
               file.write("         " + operation(oper_str, str(k), "i + " + str(k)) + "\n")
         file.write("      }\n")
         
         for oper_str in operation_str:
            for j in range(0, i):
               file.write("      " + operation(oper_str, str(j), "sizem" + str(j + 1)) + "\n")
         file.write("   }\n")
      
      file.write("\n")
      
      file.write("   auto result = result_array[0]")
      for i in range(1, order):
         file.write(" + result_array[" + str(i) +"]")
      file.write(";\n")
   else:
      file.write("   " + register + "T result = static_cast<T>(0.0);\n")
      file.write("\n")
      file.write("   for(int i = 0; i < size; i += 1)\n")
      file.write("   {\n")
      for oper_str in operation_str:
         file.write("      " + operation(oper_str, -1, "i") + "\n")
      file.write("   }\n")

   
   file.write("\n")
   file.write("   return result;\n")

   file.write( "}\n"
               "\n"
             )
   

#
#
#
def main():
   parser = optparse.OptionParser("usage%prog -H <target_host> -p <target_port[s]>")
   parser.add_option("-f",dest="filename",type="string",help="specify filename")
   parser.add_option("-o",dest="order",type="int",help="specify max combination order")

   (options,args) = parser.parse_args()
   filename = options.filename
   order    = options.order or 2
   
   # Data
   function_name = "ptr_" + filename
   arguments     = ["ptr1", "ptr2"]
   operation_str = "result[<ridx>] += ptr1[<pidx>] * ptr2[<pidx>];"
   #operation_str = "result[<ridx>] += (ptr1[<pidx>] - ptr2[<pidx>]) * (ptr1[<pidx>] - ptr2[<pidx>]);"
   #operation_str = [  "auto intermed<ridx> = ptr1[<pidx>] - ptr2[<pidx>];",
   #                   "result[<ridx>] += intermed<ridx> * intermed<ridx>;"
   #                ]

   # open file
   #header_file = open(filename + ".hpp", 'w')
   #declar_file = open(filename + "_decl.hpp", 'w')
   source_file = open(filename + ".hpp", 'w')

   the_range = []
   idx = 1
   while True:
      if idx > order:
         break
      the_range.append(idx)
      idx = idx * 2

   #generate_header_file(header_file, filename, function_name)

   generate_guard_top(source_file, function_name, "")
   #generate_guard_top(source_file, function_name, "IMPL")
   # make individual dirprod function
   #for i in range(1, order + 1):
   for i in the_range:
      generate_code_for_branching(source_file, i, function_name, arguments, operation_str)
   #generate_guard_bottom(source_file, function_name, "IMPL")
   
   #for i in range(1, order + 1):
   #for i in the_range:
   #   generate_header_code(declar_file, i, function_name, arguments)
   generate_header_wrapper_code(source_file, order, function_name, arguments, the_range)
   generate_guard_bottom(source_file, function_name, "")
   
if __name__ == "__main__":
   main()
