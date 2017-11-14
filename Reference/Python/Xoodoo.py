# -*- coding: utf-8 -*-

# Preview of the Xoodoo permutation, as presented by Joan Daemen at the ECC
# workshop in Nijmegen, The Netherlands, on November 14th, 2017.

# Implementation by Seth Hoffert, hereby denoted as "the implementer".

# To the extent possible under law, the implementer has waived all copyright
# and related or neighboring rights to the source code in this file.
# http://creativecommons.org/publicdomain/zero/1.0/

class Xoodoo:
    rc_s = []
    rc_p = []

    def __init__(self):
        s = 1
        p = 1

        for i in range(6):
            self.rc_s.append(s)
            s = (s * 5) % 7

        for i in range(7):
            self.rc_p.append(p)
            p = p ^ (p << 2)
            if (p & 0b10000) != 0: p ^= 0b10110
            if (p & 0b01000) != 0: p ^= 0b01011

    def Permute(self, A, r):
        for i in range(1 - r, 1):
            A = self.__Round(A, i)

        return A

    def __Round(self, A, i):
        # θ
        P = A[0] ^ A[1] ^ A[2]
        E = CyclicShiftPlane(P, 1, 5) ^ CyclicShiftPlane(P, 1, 14)
        for y in range(3): A[y] = A[y] ^ E

        # ρ_west
        A[1] = CyclicShiftPlane(A[1], 1, 0)
        A[2] = CyclicShiftPlane(A[2], 0, 11)

        # ι
        rc = (self.rc_p[-i % 7] ^ 0b1000) << self.rc_s[-i % 6]
        A[0][0] = A[0][0] ^ rc

        # χ
        B = State()
        B[0] = ~A[1] & A[2]
        B[1] = ~A[2] & A[0]
        B[2] = ~A[0] & A[1]
        for y in range(3): A[y] = A[y] ^ B[y]

        # ρ_east
        A[1] = CyclicShiftPlane(A[1], 0, 1)
        A[2] = CyclicShiftPlane(A[2], 2, 8)

        return A

def load32(b):
    return sum((b[i] << (8 * i)) for i in range(4))

def ReduceX(x):
    return ((x % 4) + 4) % 4

def ReduceZ(z):
    return ((z % 32) + 32) % 32

def CyclicShiftLane(a, dz):
    dz = ReduceZ(dz)

    if dz == 0:
        return a
    else:
        return ((a >> (32 - (dz % 32))) | (a << (dz % 32))) % (1 << 32)

class Plane:
    lanes = []

    def __init__(self, lanes = None):
        if lanes is None:
            lanes = [0] * 4
        self.lanes = lanes

    def __getitem__(self, i):
        return self.lanes[i]

    def __setitem__(self, i, v):
        self.lanes[i] = v

    def __str__(self):
        return ' '.join("0x{:08x}".format(x) for x in self.lanes)

    def __xor__(self, other):
        return Plane([self.lanes[x] ^ other.lanes[x] for x in range(4)])

    def __and__(self, other):
        return Plane([self.lanes[x] & other.lanes[x] for x in range(4)])

    def __invert__(self):
        return Plane([~self.lanes[x] for x in range(4)])

def CyclicShiftPlane(A, dx, dz):
    p = Plane()

    for i in range(4):
        index = ReduceX(i - dx)
        p[i] = CyclicShiftLane(A[index], dz)

    return p

class State:
    planes = []

    def __init__(self, state = None):
        if state is None:
            state = bytearray(48)
        self.planes = [Plane([load32(state[4*(x+4*y):4*(x+4*y)+4]) for x in range(4)]) for y in range(3)]

    def __getitem__(self, i):
        return self.planes[i]

    def __setitem__(self, i, v):
        self.planes[i] = v

    def __str__(self):
        return ' '.join(str(x) for x in self.planes)

xp = Xoodoo()
A = State()
for i in range(384): A = xp.Permute(A, 12)
print(A)
