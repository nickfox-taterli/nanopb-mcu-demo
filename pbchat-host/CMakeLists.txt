cmake_minimum_required(VERSION 3.16)
project(pbchat-host C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(pbchat-host
    src/main.c
    src/proto/chat.pb.c
    third_party/nanopb/pb_common.c
    third_party/nanopb/pb_decode.c
    third_party/nanopb/pb_encode.c
)

target_include_directories(pbchat-host PRIVATE
    src
    src/proto
    third_party/nanopb
)

target_compile_options(pbtest PRIVATE -Wall -Wextra -Werror)
