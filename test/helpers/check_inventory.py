"""Check semantic destinations, independently of the parser's helper table."""

import argparse
import xml.etree.ElementTree as ET


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("xml")
    parser.add_argument("--three-unpacked", action="store_true")
    args = parser.parse_args()

    destinations = {"clean_dst", "cond_dst", "bit_dst", "wide_dst", "one", "two"}
    bits = {name: 96 for name in destinations}
    bits.update(one=192, two=384, src0=96, src1=96, index=32, bit_source=1)
    if args.three_unpacked:
        destinations.add("three")
        bits["three"] = 768

    fields = ET.parse(args.xml).findall("./modules/module/var")
    actual_bits = {field.attrib["id"]: int(field.attrib["bits"]) for field in fields}
    actual_destinations = {field.attrib["id"] for field in fields if field.attrib.get("inj_loc")}
    if actual_bits != bits or len(fields) != len(bits):
        raise SystemExit(f"Wrong helper fixture inventory: {actual_bits}; expected {bits}")
    if actual_destinations != destinations:
        raise SystemExit(
            f"Wrong writable destinations: {sorted(actual_destinations)}; "
            f"expected {sorted(destinations)}. Sources and array indices must remain read-only."
        )
    print(f"PASS: {len(destinations)} helper destinations; sources and index remain read-only")


if __name__ == "__main__":
    main()
