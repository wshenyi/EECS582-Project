set(LLVM_LINK_COMPONENTS support)

set(JSON_BuildTests OFF CACHE INTERNAL "")

add_clang_executable(insert_tx
        insert_tx.cpp
        nlohmann/json.hpp
        )

clang_target_link_libraries(insert_tx PRIVATE
        clangTooling
        clangBasic
        clangASTMatchers
        clangFrontend
        clangLex
        clangAST
        clangDriver
        )