C++ implementation of a simple programming language called KuroLang.

> This is a simple programming language that I am creating to learn more about compilers and interpreters.
> It produces x86-64 assembly code at the moment.

To run / setup:

setup:
```bash
cmake -S . -B build
```

compile:
```bash
cmake --build ./build/
```

run:
```bash
./build/kurolang ./test.k
```
