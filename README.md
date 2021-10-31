#  Transaction Macro Insertion based on Clang Frontend

## Aim

We want to insert transaction macro to fix reordering bugs founded by Squint.

## Program

The source code is in `tools/insert-tx`

The sample file is at `test/InsertTX` and the generated file is also in this dir.

After build executable file `insert_tx` in `cmake-build/bin` dir, run code below will generate file.

```

./cmake-build/bin/insert_tx ../../test/InsertTX/writer.c

```
