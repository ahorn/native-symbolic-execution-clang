set(LLVM_LINK_COMPONENTS support)

add_clang_library(nse
  IntegralTypeReplacer.cpp
  )
target_link_libraries(nse
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangLex
  clangTooling
  )

add_subdirectory(tool)
