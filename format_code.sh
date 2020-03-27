#!/bin/bash

#clang-format --style=llvm -i ./tests/*.cpp ./tests/*.h
#clang-format --style=llvm -i ./msg_order_relax_checker/*.cpp ./msg_order_relax_checker/*.h
#clang-format --style=llvm -i ./dump_ir_pass/*.cpp ./dump_ir_pass/*.h

clang-format --style=llvm -i ./tests/*.c
clang-format --style=llvm -i ./tests/two_messages/*.c
clang-format --style=llvm -i ./msg_order_relax_checker/*.cpp ./msg_order_relax_checker/*.h
clang-format --style=llvm -i ./dump_ir_pass/*.cpp
