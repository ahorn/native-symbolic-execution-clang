set(LLVM_LINK_COMPONENTS support)

add_clang_library(crv
  IntegralTypeReplacer.cpp
  )
target_link_libraries(crv
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangLex
  clangTooling
  )

add_subdirectory(tool)
