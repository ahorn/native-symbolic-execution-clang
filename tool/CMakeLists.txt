set(LLVM_LINK_COMPONENTS support)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_clang_executable(clang-crv
  ClangCrv.cpp
  )

add_dependencies(clang-crv clang-modernize
  clang-headers clang-apply-replacements
  )

target_link_libraries(clang-crv
  clang-modernize
  clangAST
  clangASTMatchers
  clangBasic
  clangFormat
  clangFrontend
  clangLex
  clangTooling
  crv
  )

install(TARGETS clang-crv
  RUNTIME DESTINATION bin)
