#!python
# -*- mode: python; Encoding: utf-8; coding: utf-8 -*-
# Last updated: <2024/03/20 07:04:48 +0900>
"""
spritesheet uv psition csv to C language table

Usage: python csv2c.py INPUT.csv > tmp.txt

Windows10 x64 22H2 + Python 3.10.10 64bit
"""

import sys
import csv

IMGW = 4096.0
IMGH = 4096.0


def main():
    if len(sys.argv) != 2:
        print("Usage: %s INFILE" % (sys.argv[0]))
        sys.exit()

    infile = sys.argv[1]
    symtbl = []
    uvtbl = []

    with open(infile) as f:
        reader = csv.reader(f)

        for i, row in enumerate(reader):
            name = row[1]
            w = int(row[2])
            h = int(row[3])
            u = float(row[4]) / IMGW
            v = float(row[5]) / IMGH
            uw = float(row[6]) / IMGW
            vh = float(row[7]) / IMGH
            sname = "SPR_" + name.upper()
            symtbl.append(sname)
            uvtbl.append(
                {"w": w, "h": h, "u": u, "v": v, "uw": uw, "vh": vh, "name": sname}
            )

    msg = """\
// ----------------------------------------
// sprite type
typedef enum sprtype
{
"""
    print(msg)
    for i, s in enumerate(symtbl):
        print("  %s, // %d" % (s, i))
    print("} SPRTYPE;\n")

    msg = """\
// ----------------------------------------
// sprites size and uv table
typedef struct sprtbl
{
  float w;
  float h;
  float u;
  float v;
  float uw;
  float vh;
} SPRTBL;
"""
    print(msg)

    print("static SPRTBL spr_tbl[%d] = {" % len(uvtbl))
    print("    // poly w, poly h, u, v, uw, vh")

    for i, d in enumerate(uvtbl):
        print(
            "    {%d, %d, %f, %f, %f, %f},  // %d %s"
            % (d["w"], d["h"], d["u"], d["v"], d["uw"], d["vh"], i, d["name"])
        )

    print("};")


if __name__ == "__main__":
    main()
