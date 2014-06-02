# Clang CRV Front-end 

This is the front-end to CRV, a research project into fast symbolic execution
of sequential and concurrent C++11 code. CRV consists of two parts: an
instrumentation module and a [runtime library][nse].

This design allows CRV to leverage advanced C++11 features to symbolically
execute code efficiently. Efficiency is particularly important for things
such as constant propagation, a well-known technique for simplifying symbolic
expressions at runtime. This can give [performance gains][performance-tests]
of several orders of magnitude (e.g. up to a million times faster) but
requires certain source-to-source transformations of the user's program.

This is where the CRV frond-end comes in: it takes as input a C++11 program
that the user wants to symbolically execute. Given such a program, the
front-end is responsible for instrumenting it so that it can be linked with
the CRV library. The necessary source-to-source transformations are implemented
with Clang's [LibASTMatchers][clang-ast-matchers] library.

CRV and the front-end evolve together and there is still more work to be done.
If you would like to get involved, feel free to get in touch!

[nse]: https://github.com/ahorn/smt-kit#native-symbolic-execution
[performance-tests]: https://github.com/ahorn/smt-kit/blob/master/test/crv_performance_test.cpp

## Install

For convenience, here is a possible way to build the project:

    $ svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm
    $ cd llvm/tools
    $ svn co http://llvm.org/svn/llvm-project/cfe/trunk clang
    $ cd ../..
    $ cd llvm/projects
    $ svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk compiler-rt
    $ cd ../..
    $ cd llvm/tools/clang/tools
    $ svn co http://llvm.org/svn/llvm-project/clang-tools-extra/trunk extra
    $ cd ../../../..
    $ mkdir -p build/target
    $ cd llvm/tools/clang/tools/extra
    $ git clone https://github.com/ahorn/native-symbolic-execution-clang.git
    $ echo "DIRS += native-symbolic-execution-clang" >> Makefile
    $ echo "add_subdirectory(native-symbolic-execution-clang)" >> CMakeLists.txt
    $ cd ../../../../../build
    $ ../llvm/configure --prefix=`pwd`/target
    $ make
    $ make check
    $ make install

## Getting started

For illustration purposes, let `example.cpp` be the following C++ program:

```C++
short GlobalVariable;

short foo(short a) {
  return a;
}

int main() {
  short i = 0;

  for (; i < 8; i++) {}
  if (i == 9) {
    GlobalVariable = i;
  }

  return 0;
}
```

To see the effects of the source-to-source transformation,
execute the following commands:

    $ /path/to/clang-nse example.cpp --
    $ cat example.cpp

The output looks as follows:

```C++
crv::External<short> GlobalVariable;

crv::Internal<short> foo(crv::Internal<short> a) {
  return a;
}

int main() {
  crv::Internal<short> i = 0;

  for (; crv::tracer().decide_flip(i < 8); crv::post_increment(i)) {}
  if (crv::tracer().decide_flip(i == 9)) {
    GlobalVariable = i;
  }

  return 0;
}
```

Notice that types are not just replaced by textual substitution.
For example, `GlobalVariable` is treated differently from the local
variable `i`. And unlike `foo`, the return type of `main` is unchanged.
Note that branches in control flow statements are instrumented. In fact,
these illustrate that the CRV library overloads many operators to simplify
the task of writing the front-end.

## Clang's AST Matchers

CRV requires source-to-source transformations of C++11 code. But writing a
C++ parser is notoriously hard. So as opposed to creating our own, we use
Clang -- Apple's compiler of choice.

The CRV front-end is built on [LibASTMatchers][clang-ast-matchers] which
provide a domain-specific language to Clang's abstract syntax tree (AST).
Clang's AST is very rich and many corner cases are not considered by the
CRV front-end. This is purely for practical reasons and is not due to any
intrinsic limitations of Clang. Any contributions to fix issues are warmly
invited and can be sent as Github pull request.

In adding support for new features, there are three main sources for help:

Firstly, the AST of a program can be inspected with clang itself. For example,
the following command:

    $ clang -Xclang -ast-dump -fsyntax-only example.cpp

produces the AST dump below:

```
TranslationUnitDecl 0x102021cd0 <<invalid sloc>>
|-TypedefDecl 0x102022210 <<invalid sloc>> __int128_t '__int128'
|-TypedefDecl 0x102022270 <<invalid sloc>> __uint128_t 'unsigned __int128'
|-TypedefDecl 0x102022630 <<invalid sloc>> __builtin_va_list '__va_list_tag [1]'
|-VarDecl 0x102022690 <test/crv-test.cpp:1:1, col:7> GlobalVariable 'short'
|-FunctionDecl 0x1020227c0 <line:3:1, line:5:1> foo 'short (short)'
| |-ParmVarDecl 0x102022700 <line:3:11, col:17> a 'short'
| `-CompoundStmt 0x1020228c8 <col:20, line:5:1>
|   `-ReturnStmt 0x1020228a8 <line:4:3, col:10>
|     `-ImplicitCastExpr 0x102022890 <col:10> 'short' <LValueToRValue>
|       `-DeclRefExpr 0x102022868 <col:10> 'short' lvalue ParmVar 0x102022700 'a' 'short'
`-FunctionDecl 0x102022940 <line:7:1, line:16:1> main 'int (void)'
  `-CompoundStmt 0x102053a28 <line:7:12, line:16:1>
    |-DeclStmt 0x102053710 <line:8:3, col:14>
    | `-VarDecl 0x102053680 <col:3, col:13> i 'short'
    |   `-ImplicitCastExpr 0x1020536f8 <col:13> 'short' <IntegralCast>
    |     `-IntegerLiteral 0x1020536d8 <col:13> 'int' 0
    |-ForStmt 0x102053828 <line:10:3, col:23>
    | |-<<<NULL>>>
    | |-<<<NULL>>>
    | |-BinaryOperator 0x1020537a0 <col:10, col:14> '_Bool' '<'
    | | |-ImplicitCastExpr 0x102053788 <col:10> 'int' <IntegralCast>
    | | | `-ImplicitCastExpr 0x102053770 <col:10> 'short' <LValueToRValue>
    | | |   `-DeclRefExpr 0x102053728 <col:10> 'short' lvalue Var 0x102053680 'i' 'short'
    | | `-IntegerLiteral 0x102053750 <col:14> 'int' 8
    | |-UnaryOperator 0x1020537f0 <col:17, col:18> 'short' postfix '++'
    | | `-DeclRefExpr 0x1020537c8 <col:17> 'short' lvalue Var 0x102053680 'i' 'short'
    | `-CompoundStmt 0x102053810 <col:22, col:23>
    |-IfStmt 0x1020539b8 <line:11:3, line:13:3>
    | |-<<<NULL>>>
    | |-BinaryOperator 0x1020538e0 <line:11:7, col:12> '_Bool' '=='
    | | |-ImplicitCastExpr 0x1020538c8 <col:7> 'int' <IntegralCast>
    | | | `-ImplicitCastExpr 0x1020538b0 <col:7> 'short' <LValueToRValue>
    | | |   `-DeclRefExpr 0x102053868 <col:7> 'short' lvalue Var 0x102053680 'i' 'short'
    | | `-IntegerLiteral 0x102053890 <col:12> 'int' 9
    | |-CompoundStmt 0x102053998 <col:15, line:13:3>
    | | `-BinaryOperator 0x102053970 <line:12:5, col:22> 'short' lvalue '='
    | |   |-DeclRefExpr 0x102053908 <col:5> 'short' lvalue Var 0x102022690 'GlobalVariable' 'short'
    | |   `-ImplicitCastExpr 0x102053958 <col:22> 'short' <LValueToRValue>
    | |     `-DeclRefExpr 0x102053930 <col:22> 'short' lvalue Var 0x102053680 'i' 'short'
    | `-<<<NULL>>>
    `-ReturnStmt 0x102053a08 <line:15:3, col:10>
      `-IntegerLiteral 0x1020539e8 <col:10> 'int' 0
```

Secondly, there is `clang-query` that is an interactive tool for
writing and testing AST matchers. The following illustrates this:

    $ clang-query example.cpp --
    $ clang-query> match functionDecl(returns(qualType(isInteger())))
    
    Match #1:

    example.cpp:3:1: note: "root" binds here
    short foo(short a) {
    ^~~~~~~~~~~~~~~~~~~~

    Match #2:

    example.cpp:7:1: note: "root" binds here
    int main() {
    ^~~~~~~~~~~~
    2 matches.

Thirdly, there is a lot of [documentation][clang-doc] including
the [AST Matcher Reference][clang-ast-matchers-ref]. When in doubt
how to use the various kinds of matchers, the source code of
[clang-modernize][clang-modernize] is good starting point.

[clang-ast-matchers]: http://clang.llvm.org/docs/LibASTMatchers.html
[clang-ast-matchers-ref]: http://clang.llvm.org/docs/LibASTMatchersReference.html
[clang-doc]: http://clang.llvm.org/docs/
[clang-modernize]: http://clang.llvm.org/extra/clang-modernize.html
