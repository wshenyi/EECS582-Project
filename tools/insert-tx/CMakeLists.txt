set(LLVM_LINK_COMPONENTS support)

add_clang_executable(insert_tx
        insert_tx.cpp
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