#  Transaction Macro Insertion based on Clang Frontend

## What we want to do?

We want to insert transaction macro to fix reordering bugs founded by Squint.

## Assumption

1. The test programs only have one persistent pool file.
2. When two interval boundaries *TX_BEGIN* and *TX_END* appear in different scope/function/file, our program will insert these boundaries in an upper position of program stack.  

## Program

The source code is in `tools/insert-tx`

The sample file is at `test/InsertTX` and the generated file is also in this dir.

After executable file `insert_tx` has been built in `cmake-build/bin` dir, run code below will generate modified source file.

```

./cmake-build/bin/insert_tx ../../test/InsertTX/writer.c

```
