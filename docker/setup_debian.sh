#!/usr/bin/env bash
########################################################################################################################
# Package dependencies
get_apt_deps() {
  verilator_apt_dep="autoconf automake autotools-dev bison curl flex g++ git xz-utils wget"
  vrtlmod_apt_dep="cmake libboost-filesystem-dev libboost-date-time-dev libfl-dev build-essential ccache python3 python3-virtualenv python3-dev"
  echo "${verilator_apt_dep} ${vrtlmod_apt_dep}"
}
setup_env() {
  apt-get update
  for pkg in "$(get_apt_deps)"
  do 
    apt-get install --no-install-recommends -y ${pkg}
  done

}
########################################################################################################################
# Python virtual env
setup_pyvenv() {
  venv_dir="$1"
  req_file="$2"

  echo "[setup] venv"
  python3 -m virtualenv "${venv_dir}"
  . "${venv_dir}/bin/activate"
  pip install --upgrade pip
  if [ -f "${req_file}" ]; then
    pip install -r "${req_file}"
  fi
}
########################################################################################################################
# LLVM
fetch_llvm() {
  _home_=$PWD
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"
  llvm_patches_dir="${5}"

  llvm_prefix="llvm-project"
  llvm_tag="llvmorg-${version}"
  llvm_url="https://github.com/llvm/llvm-project"
  llvm_patch_file="${llvm_tag}.patch"

  echo "[fetch] llvm"
  git clone --depth 1 --branch "${llvm_tag}" ${llvm_url}.git ${src_dir}

  echo "[patch?] llvm"
  ls -la "${llvm_patches_dir}"

  if [ -f "${llvm_patches_dir}/${llvm_patch_file}" ]; then
    echo "[patch] llvm"
    cd ${src_dir}
    echo "applying patch: ${llvm_patch_file} from [${llvm_patches_dir}]."
    git apply "${llvm_patches_dir}/${llvm_patch_file}"
    cd ${_home_}
  fi
}
configure_llvm() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"

  echo "[configure] llvm"
  cmake \
    -S "${src_dir}/llvm" \
    -B "${build_dir}" \
    -D "CMAKE_BUILD_TYPE=Release" \
    -D "LLVM_ENABLE_PROJECTS=clang;clang-tools-extra" \
    -D "LLVM_TARGETS_TO_BUILD=Native" \
    -D "CMAKE_INSTALL_PREFIX=${install_dir}"
}
build_llvm() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"

  echo "[build] llvm"
  cmake --build "${build_dir}" --parallel "$(nproc)"
  "${build_dir}/bin/clang" --version
}
install_llvm() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"

  echo "[install] llvm"
  cmake --build "${build_dir}" --parallel "$(nproc)" --target install
  "${install_dir}/bin/clang" --version
}
cleanup_llvm() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"

  echo "[clean-up] llvm"
  rm -rf "${src_dir}" "${build_dir}"
}
setup_llvm() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"
  patch_dir=$5

  if [ ! -f "${install_dir}/bin/clang" ]; then
    fetch_llvm "$1" "$2" "$3" "${4}" ${5} && \
    configure_llvm "$1" "$2" "$3" "${4}" && \
    build_llvm "$1" "$2" "$3" "${4}" && \
    install_llvm "$1" "$2" "$3" "${4}" && \
    cleanup_llvm "$1" "$2" "$3" "${4}"
  fi
}
########################################################################################################################
# SystemC
fetch_systemc() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"

  systemc_tag="${version}"
  systemc_url="https://github.com/accellera-official/systemc.git"

  echo "[fetch] systemc"
  echo "git clone --depth 1 --branch "${systemc_tag}" "${systemc_url}" "${src_dir}""
  git clone --depth 1 --branch "${systemc_tag}" "${systemc_url}" "${src_dir}"
}
configure_systemc() {
  _home_=$PWD
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"

  echo "[configure] systemc"
  cmake -S "${src_dir}" -B "${build_dir}" \
    -D CMAKE_INSTALL_PREFIX="${install_dir}" \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_CXX_STANDARD=17
}
build_systemc() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"

  echo "[build] systemc"
  cmake --build "${build_dir}" --parallel $(nproc)
}
install_systemc() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"

  echo "[install] systemc"
  cmake --build "${build_dir}" --parallel $(nproc) --target install
}
cleanup_systemc() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"

  echo "[clean-up] systemc"
  rm -rf "${src_dir}"
  rm -rf "${build_dir}"
}
setup_systemc() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"

  if [ ! -f "${install_dir}/lib/libsystemc.so" ]; then
    fetch_systemc "$1" "$2" "$3" "${4}" && \
    configure_systemc "$1" "$2" "$3" "${4}" && \
    build_systemc "$1" "$2" "$3" "${4}" && \
    install_systemc "$1" "$2" "$3" "${4}" && \
    cleanup_systemc "$1" "$2" "$3" "${4}"
  fi
}
########################################################################################################################
# Verilator
fetch_verilator() {
  _home_=${PWD}
  src_dir="$1"
  #build_dir="$2"
  install_dir="$3"
  version="${4}"

  verilator_tag="v${version}"
  verilator_url="https://github.com/verilator/verilator.git"

  echo "[fetch] verilator"
  git clone "${verilator_url}" "${src_dir}"
  cd "${src_dir}"
  git checkout "${verilator_tag}"
  cd "${_home_}"
}
configure_verilator() {
  _home_=$PWD
  src_dir="$1"
  #build_dir="$2"
  install_dir="$3"
  version="${4}"

  echo "[configure] verilator"
  cd "${src_dir}"
  autoconf
  ./configure --prefix "${install_dir}"
  cd "${_home_}"
}
build_verilator() {
  src_dir="$1"
  #build_dir="$2"
  install_dir="$3"
  version="${4}"

  echo "[build] verilator"
  make -C "${src_dir}" -j $(nproc)
}
install_verilator() {
  src_dir="$1"
  #build_dir="$2"
  install_dir="$3"

  echo "[install] verilator"
  make -C "${src_dir}" -j $(nproc) install
}
cleanup_verilator() {
  src_dir="$1"
  #build_dir="$2"
  install_dir="$3"

  echo "[clean-up] verilator"
  rm -rf "${src_dir}"
}
setup_verilator() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"
  version="${4}"
  if [ ! -f "${install_dir}/bin/verilator" ]; then
    fetch_verilator "$1" "$2" "$3" "${4}" && \
    configure_verilator "$1" "$2" "$3" "${4}" && \
    build_verilator "$1" "$2" "$3" "${4}" && \
    install_verilator "$1" "$2" "$3" "${4}" && \
    cleanup_verilator "$1" "$2" "$3" "${4}"
  fi
}
########################################################################################################################
# vRTLmod
fetch_vrtlmod() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"

  echo "[fetch] vRTLmod"
  echo "nothing to do."
  echo "SYSTEMC_HOME=${SYSTEMC_HOME}"
  ls -la "${SYSTEMC_HOME}"
}
configure_vrtlmod() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"

  echo "[configure] vRTLmod"
  cmake \
    -S "${src_dir}" \
    -B "${build_dir}" \
    -D CMAKE_BUILD_TYPE:STRING="${VRTLMOD_BUILD_CONFIG}" \
    -D CMAKE_BUILD_TYPE=${VRTLMOD_BUILD_CONFIG} \
    -D CMAKE_INSTALL_PREFIX=${install_dir} \
    -D SystemCLanguage_DIR="${SYSTEMC_HOME}/lib/cmake/SystemCLanguage" \
    -D BUILD_TESTING=On
}
build_vrtlmod() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"

  echo "[build] vRTLmod"
  cmake --build ${build_dir} --parallel $(nproc)
}
test_vrtlmod() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"

  echo "[test] vRTLmod"
  echo "SYSTEMC_HOME=${SYSTEMC_HOME}"
  cmake --build ${build_dir} --parallel $(nproc) --target test
}
install_vrtlmod() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"

  echo "[install] vRTLmod"
  cmake --build ${build_dir} --parallel $(nproc) --target install
}
cleanup_vrtlmod() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"

  echo "[clean-up] vRTLmod"
  echo "nothing to do."
  #rm -r ${build_dir}
}
setup_vrtlmod() {
  src_dir="$1"
  build_dir="$2"
  install_dir="$3"

  if [ ! -f "${install_dir}/bin/vrtlmod" ]; then
    fetch_vrtlmod "${src_dir}" "${build_dir}" "${install_dir}" && \
    configure_vrtlmod "${src_dir}" "${build_dir}" "${install_dir}" && \
    build_vrtlmod "${src_dir}" "${build_dir}" "${install_dir}" && \
    install_vrtlmod "${src_dir}" "${build_dir}" "${install_dir}" && \
    test_vrtlmod "${src_dir}" "${build_dir}" "${install_dir}" && \
    cleanup_vrtlmod "${src_dir}" "${build_dir}" "${install_dir}"
  fi
}

setup() {
  _what_=$1
  setup_${_what_} $2 $3 $4 $5 $6
}
