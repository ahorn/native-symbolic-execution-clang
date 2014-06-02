set(LLVM_LINK_COMPONENTS support)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_clang_executable(clang-nse
  ClangNse.cpp
  )

add_dependencies(clang-nse clang-modernize
  clang-headers clang-apply-replacements
  )

target_link_libraries(clang-nse
  clang-modernize
  clangAST
  clangASTMatchers
  clangBasic
  clangFormat
  clangFrontend
  clangLex
  clangTooling
  nse
  )

install(TARGETS clang-nse
  RUNTIME DESTINATION bin)
