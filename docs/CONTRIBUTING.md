# Contributing to vRTLmod

vRTLmod uses LLVM/Clang to add fault injection and differential analysis APIs to Verilator-generated C++ and SystemC models. Contributions should preserve simulation behavior when injection is disabled, make target discovery and injection timing observable, and keep clear contracts between model analysis, rewriting and API generation.

## Design and coding conventions

Start with the [README](../README.adoc), the relevant interfaces in [include/vrtlmod](../include/vrtlmod/), and the implementation and tests for the behavior being changed. Follow agreed requirements; proposals and deferred features remain explicitly identified as such. Update the relevant documentation, interface comments and tests when changing a command-line option, XML representation, target dictionary or generated API contract.

Use the configured C++ standard, currently C++17 by default, and follow the repository's [.clang-format](../.clang-format). Format changed project code consistently with its surroundings and preserve intentional `clang-format off` regions in AST matchers. Keep assumptions about Verilator output and Clang AST shapes explicit, including version-specific behavior. RTL fixtures may use SystemVerilog as needed to exercise supported model forms.

Model analysis and target discovery live in [src/core](../src/core/), transformation passes in [src/passes](../src/passes/), and generated API templates and runtime support in [src/vapi](../src/vapi/). Keep their corresponding interfaces in `include/vrtlmod/` aligned. Build integration belongs in [cmake](../cmake/), regression fixtures and reusable injection checks in [test](../test/), and Doxygen documentation in [doc](../doc/). Register source files and test dependencies through CMake. Preserve upstream conventions and notices in [third_party](../third_party/).

## Development and validation

Install the dependencies and supported toolchain versions described in the [README](../README.adoc): CMake, a C++ compiler, LLVM/Clang development libraries, Verilator and Boost. The test suite also requires SystemC and its `SystemCLanguage` CMake package. CMake locates these dependencies; environment setup and container builds are provided separately under [docker](../docker/).

Initialize submodules, then configure and build from the repository root, replacing the installation paths below:

```sh
git submodule update --init --recursive
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=17 \
  -DBUILD_TESTING=ON \
  -DVERILATOR_ROOT=/path/to/verilator/install \
  -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm \
  -DSystemCLanguage_DIR=/path/to/systemc/lib/cmake/SystemCLanguage
cmake --build build --parallel
ctest --test-dir build --output-on-failure -j 1
```

Build vRTLmod against the same Verilator version used to generate the input model. Use separate build directories for different Verilator or LLVM installations. Record the C++ standard and `VRTLMOD_STDLIB` setting when they affect parsing or compatibility.

Run checks appropriate to the behavior and configuration being changed. List the available tests or select a relevant subset with:

```sh
ctest --test-dir build -N
ctest --test-dir build --output-on-failure -j 1 -R fiapp-cc
ctest --test-dir build --output-on-failure -j 1 -R fiapp-sc
ctest --test-dir build --output-on-failure -j 1 -R fiapp-cmake
ctest --test-dir build --output-on-failure -j 1 -R scheduling
```

The filters above include the associated build/configure and runtime checks. The [fiapp tests](../test/fiapp/) exercise C++, SystemC, state dimensions, fault injection and differential APIs, including downstream CMake integration. The [scheduling tests](../test/scheduling/) cover split and inlined functions, asynchronous reset, derived clocks and evaluation without a clock event. Keep reproducers small and record random seeds where applicable.

For behavioral changes, cover the affected cases and relevant toolchain versions. Check expected target coverage and dimensions, injection timing and counts, and observable state or differential results against an independent expectation. Verify that disabling injection preserves the original model's behavior. Include held registers, enables, reset, dynamic indices and packed/unpacked arrays where the contract needs them. State which checks ran and which remain untested; documentation-only changes need link and formatting checks rather than simulation.

For performance or benchmark claims, compare matched RTL, tool versions, Verilator/compiler flags, workloads and fault campaign settings. Keep target coverage and injection behavior comparable. Report transformation time, generated model build time and simulation performance separately; include memory or code size when making claims about them. Larger benchmark integration uses the external `tum-ei-eda/vrtlmod-test` repository; see the benchmark job in [CI](../.github/workflows/ci.yml) for its additional setup. Keep generated models, APIs and logs under `build/` or another ignored build directory, and include concise results and reproduction commands in the contribution.

For API documentation changes, install Doxygen and Graphviz before configuring, then run `cmake --build build --target doc`. The generated documentation belongs under `build/doc/`.

## Issues and pull requests

For a bug report, include the expected and observed behavior, vRTLmod revision, Verilator and LLVM/Clang versions, C++ compiler and standard library, exact model-generation and vRTLmod commands, relevant flags or seeds, and diagnostics. Include a small RTL or generated C++ reproducer and any whitelist XML needed to reproduce the issue. A small reproducer helps distinguish analysis or rewriting defects from model-generation and integration issues.

Keep each commit focused on one logical change. A pull request should explain the motivation, resulting behavior, validation and material limitations. Link related issues and document architecture or interface decisions where they are defined. Preserve upstream notices when importing code; see [LICENSE](../LICENSE) for the repository's license text.

## Commit messages

Use an emoji prefix, a brief lower-case subject ending in a colon, a blank line, then three or four lines summarizing the motivation/cause, change and effect:

```text
<emoji> <brief>:

<details>
```

Start the first detail sentence in lower case as a continuation of the subject's colon. Capitalize subsequent sentences normally. Preserve conventional casing for identifiers and acronyms such as `LLVM`, `AST`, `XML`, `ClangTool` and `VRTLMOD_` options. Hard line breaks are allowed. The details are optional for sufficiently small and clear commits.

| Prefix | Type |
| --- | --- |
| 🐛 | Bug fix |
| 🔥 | Hotfix |
| 💡 | Feature |
| 📝 | Documentation |
| ⚙️ | CI and build automation |

For example:

```text
🐛 avoid stale header metadata during rewriting:

rewritten headers changed size while later passes reused cached metadata.
Create a fresh ClangTool for each rewritten file.
Propagate frontend errors so failed transformations return a nonzero status.
```
