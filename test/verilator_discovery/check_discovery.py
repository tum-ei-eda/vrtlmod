import argparse
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


def run(command, environment, log_path, expect_success=True):
    result = subprocess.run(
        [str(value) for value in command],
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=120,
    )
    log_path.write_text(shlex.join(str(value) for value in command) + "\n" + result.stdout)
    if (result.returncode == 0) != expect_success:
        raise RuntimeError(f"Unexpected exit {result.returncode}; {log_path}\n{result.stdout[-6000:]}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser()
    for name in ["module-dir", "root", "executable", "version", "cmake", "cc", "cxx", "generator", "make-program",
                 "configuration", "work-dir"]:
        parser.add_argument("--" + name, required=True)
    args = parser.parse_args()
    root = Path(args.root).resolve()
    executable = Path(args.executable).resolve()
    args.configuration = args.configuration or "Release"
    work_dir = Path(args.work_dir)
    work_dir.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="run-", dir=work_dir))
    print(f"Discovery regression evidence: {work}", flush=True)

    selected_path = work / "selected tools"
    selected_path.mkdir()
    (selected_path / "verilator").symlink_to(executable)
    hostile_path = work / "hostile tools"
    hostile_path.mkdir()
    poison = """#!/usr/bin/env python3
import os
from pathlib import Path
Path(os.environ['VRTLMOD_DISCOVERY_POISON_MARKER']).write_text('wrong executable selected')
raise SystemExit(87)
"""
    for name in ["verilator", "verilator_bin"]:
        path = hostile_path / name
        path.write_text(poison)
        path.chmod(0o755)
    hostile_package = work / "hostile package"
    hostile_package.mkdir()
    (hostile_package / "verilator-config.cmake").write_text('message(FATAL_ERROR "Stale package selected")\n')
    missing_root = work / "missing-explicit-root"
    unsupported_root = work / "unsupported-environment-root"
    unsupported_root.mkdir()
    base_environment = os.environ.copy()
    for variable in ["VERILATOR_ROOT", "VERILATOR_BIN", "verilator_DIR", "VERILATOR_EXECUTABLE"]:
        base_environment.pop(variable, None)
    original_path = base_environment.get("PATH", os.defpath)
    fixture = Path(__file__).resolve().parent / "fixture"

    cases = [
        ("path-only", None, None, selected_path, True),
        ("environment-root", None, root, hostile_path, True),
        ("explicit-root", root, unsupported_root, hostile_path, True),
        ("bad-explicit-root", missing_root, root, selected_path, False),
        ("bad-environment-root", None, unsupported_root, selected_path, False),
    ]
    for name, cmake_root, environment_root, path_prefix, success in cases:
        case = work / name
        case.mkdir()
        build = case / "build"
        marker = case / "poison-called"
        environment = base_environment.copy()
        environment["PATH"] = str(path_prefix) + os.pathsep + original_path
        # Cover both unset and poisoned caller variables during configuration.
        if name != "environment-root":
            environment["VERILATOR_BIN"] = str(hostile_path / "verilator_bin")
        environment["VRTLMOD_DISCOVERY_POISON_MARKER"] = str(marker)
        if environment_root is not None:
            environment["VERILATOR_ROOT"] = str(environment_root)
        command = [
            args.cmake, "-S", fixture, "-B", build, "-G", args.generator,
            f"-DCMAKE_C_COMPILER={args.cc}", f"-DCMAKE_CXX_COMPILER={args.cxx}",
            f"-DCMAKE_MAKE_PROGRAM={args.make_program}", f"-DCMAKE_BUILD_TYPE={args.configuration}",
            f"-DDISCOVERY_MODULE_DIR={args.module_dir}",
            f"-DVERILATOR_BIN={hostile_path / 'verilator_bin'}", f"-Dverilator_DIR={hostile_package}",
        ]
        if cmake_root is not None:
            command.append(f"-DVERILATOR_ROOT={cmake_root}")
        output = run(command, environment, case / "configure.log", success)
        if not success:
            rejected_root = cmake_root if cmake_root is not None else environment_root
            if rejected_root.name not in output:
                raise RuntimeError(f"{name} failed without identifying its invalid root; see {case / 'configure.log'}")
            print(f"PASS {name}: rejected without PATH fallback", flush=True)
            continue

        selection = dict(line.split("=", 1) for line in (build / "selection.txt").read_text().splitlines())
        if selection["version"] != args.version:
            raise RuntimeError(f"{name}: package version {selection['version']} != {args.version}")
        if Path(selection["executable"]).resolve() != executable:
            raise RuntimeError(f"{name}: selected a different Verilator executable: {selection['executable']}")
        if Path(selection["headers"]).resolve() != (root / "include").resolve():
            raise RuntimeError(f"{name}: selected headers outside the expected kit: {selection['headers']}")
        if marker.exists():
            raise RuntimeError(f"{name}: configure invoked an executable from hostile PATH/environment")

        # Force the upstream generated-source rule to run after configuration.
        # Unlike touching RTL, this does not request another CMake configure.
        generated_source = build / "model" / "Vdiscovery.cpp"
        if generated_source.exists():
            generated_source.unlink()
        environment["VERILATOR_ROOT"] = str(unsupported_root)
        environment["VERILATOR_BIN"] = str(hostile_path / "verilator_bin")
        environment["PATH"] = str(hostile_path) + os.pathsep + original_path
        run([args.cmake, "--build", build, "--config", args.configuration, "--parallel", "2"],
            environment, case / "build.log")
        model = (build / f"executable-{args.configuration}.txt").read_text()
        run([model], environment, case / "runtime.log")
        if marker.exists():
            raise RuntimeError(f"{name}: build invoked an executable from hostile PATH/environment")
        print(f"PASS {name}: matching executable/headers, forced regeneration, clocked runtime", flush=True)
    print("PASS all five Verilator discovery cases", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"FAIL {error}", file=sys.stderr)
        sys.exit(1)
