"""Prepare scoped SystemC declarations and check the real tool's XML inventory."""

import pathlib
import re
import sys
import xml.etree.ElementTree as ET


def prepare(destination, sources):
    destination.mkdir(parents=True, exist_ok=True)
    for source in sources:
        text = source.read_text()
        text = re.sub(r"(?<![:\w])(sc_in|sc_out)<", r"sc_core::\1<", text)
        text = re.sub(r"(?<![:\w])sc_bv<", "sc_dt::sc_bv<", text)
        text = re.sub(r"(?<![:\w])uint32_t\b", "std::uint32_t", text)
        if source.suffix in (".h", ".hpp"):
            text = "#include <cstdint>\n" + text
        (destination / source.name).write_text(text)


def verify_named(path):
    xml = ET.parse(path).getroot()
    cells = xml.findall("./top/cell")
    assert len(cells) == 1 and cells[0].get("id") == "TOP", "Wrong top-cell inventory"
    assert cells[0].get("cxx_member_prefix") == "vlInst.", "Lost symbol-table wrapper"
    modules = xml.findall("./modules/module")
    assert len(modules) == 1, "Wrong module inventory"
    variables = {node.get("id"): node for node in modules[0]}
    expected = {"state": ("var", 32), "nested": ("var", 1), "port": ("out", 65), "flag": ("out", 1)}
    assert len(modules[0]) == len(expected), "Duplicate variable inventory"
    assert variables.keys() == expected.keys(), f"Wrong variable inventory: {variables.keys()}"
    for name, (kind, bits) in expected.items():
        node = variables[name]
        assert node.tag == kind and node.get("bits") == str(bits), f"Wrong type/width for {name}"
        assert node.get("inj_loc"), f"No write found for {name}"
        prefix = "__VprimaryPorts." if kind == "out" else None
        assert node.get("cxx_member_prefix") == prefix, f"Wrong member prefix for {name}"
    header = (path.parent / "Vlayout.h").read_text()
    for fragment in ("// }; a comment", "/* } }; another", "('}' == '}')", 'sizeof("}; quoted \\\" {")'):
        assert fragment in header, f"Anonymous-block restoration damaged {fragment}"
    assert "anon-struct-" not in header, "Temporary anonymous-block markers leaked"


def verify_ports(path):
    xml = ET.parse(path).getroot()
    expected = {
        "clk": ("in", 1, "sc_core::sc_in<bool>"),
        "r": ("in", 1, "sc_core::sc_in<bool>"),
        "e": ("in", 32, "sc_core::sc_in<std::uint32_t>"),
        "o": ("out", 1, "sc_core::sc_out<bool>"),
        "t": ("out", 32, "sc_core::sc_out<std::uint32_t>"),
        "wide": ("out", 65, "sc_core::sc_out<sc_dt::sc_bv<65>>"),
    }
    ports = [
        node for module in xml.findall("./modules/module") for node in module if node.tag in ("in", "out")
    ]
    assert len(ports) == len(expected), "Duplicate SystemC port inventory"
    assert {node.get("id") for node in ports} == expected.keys(), "Wrong SystemC port inventory"
    for node in ports:
        name = node.get("id")
        kind, bits, source_type = expected[name]
        assert node.tag == kind and node.get("bits") == str(bits), f"Wrong port type/width for {name}"
        assert node.get("bases", "").replace(" ", "") == f"[{source_type}]", (
            f"Truncated source type for {name}: {node.get('bases')}"
        )
    state_nodes = [node for node in xml.findall("./modules/module/var") if node.get("inj_loc")]
    assert len(state_nodes) == 3, "Wrong number of clocked state targets"
    states = {node.get("id").split("__DOT__")[-1]: node for node in state_nodes}
    state_bits = {"state_bit": 1, "state_word": 32, "state_wide": 65}
    assert states.keys() == state_bits.keys(), f"Wrong clocked state inventory: {states.keys()}"
    for name, bits in state_bits.items():
        assert states[name].get("bits") == str(bits), f"Wrong state width for {name}"


if __name__ == "__main__":
    mode, filename, *sources = sys.argv[1:]
    path = pathlib.Path(filename)
    if mode == "prepare":
        prepare(path, map(pathlib.Path, sources))
    elif mode == "named":
        verify_named(path)
    elif mode == "ports":
        verify_ports(path)
    else:
        raise ValueError(f"Unknown fixture mode: {mode}")
