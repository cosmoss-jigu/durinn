# OSDI'22 Durinn Artifact

This repository contains the artifact for the OSDI'22 paper:

*Xinwei Fu, Dongyoon Lee, and Changwoo Min, “Durinn: Adversarial Memory and
Thread Interleaving for Detecting Durable Linearizability Bugs”, In Proceedings
of the 16th USENIX Symposium on Operating Systems Design and Implementation
(OSDI 2022).*

The artifact was tested in a machine with following specifications:
- 64-bit Fedora 29 OS
- two 16-core Intel Xeon Gold 5218 processors (2.30 Ghz)
- 192 GB DRAM
- 512 GB NVM

### Environment

Here we assume we are using Fedora 29, so we use **yum** for package management.

Here we assume we are using bash.
- setup .bashrc
  ```bash
  # vim ~/.bashrc and add following:

  export LLVM9_HOME=/home/usr/llvm/llvm-9.0.1.src
  export LLVM9_BIN=$LLVM9_HOME/build/bin
  export DURINN_HOME=/home/usr/durinn

  export PMEM_MMAP_HINT=600000000000
  export PMEM_IS_PMEM_FORCE=1
  export NDCTL_ENABLE=n

  ulimit -c unlimited
  ```
  ```bash
  source ~/.bashrc
  ```
- setup /etc/sysctl.conf
  ```bash
  # sudo vim /etc/sysctl.conf and add following:

  kernel.core_pattern=/tmp/%e-%p.core
  ```
  ```bash
  sudo sysctl -p
  ```

### Installation of llvm and clang 9.0.1
- install llvm and clang 9.0.1
  - get source
    ```bash
    mkdir $LLVM9_HOME
    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-9.0.1/llvm-9.0.1.src.tar.xz
    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-9.0.1/clang-9.0.1.src.tar.xz
    wget https://github.com/llvm/llvm-project/releases/download/llvmorg-9.0.1/compiler-rt-9.0.1.src.tar.xz
    tar xvf llvm-9.0.1.src.tar.xz && rm -f llvm-9.0.1.src.tar.xz
    tar xvf clang-9.0.1.src.tar.xz && rm -f clang-9.0.1.src.tar.xz
    tar xvf compiler-rt-9.0.1.src.tar.xz && rm -f compiler-rt-9.0.1.src.tar.xz
    mv llvm-9.0.1.src $LLVM9_HOME
    mv clang-9.0.1.src $LLVM9_HOME/tools/clang
    mv compiler-rt-9.0.1.src $LLVM9_HOME/projects/compiler-rt
    ```
  - make
    ```bash
    mkdir -p $LLVM9_HOME/build
    cd $LLVM9_HOME/build
    cmake -DLLVM_ENABLE_RTTI=true ..
    make -j16
    ```
  - update .bashrc
    ```bash
    # vim ~/.bashrc and add following:
    export CC=$LLVM9_BIN/clang
    export CXX=$LLVM9_BIN/clang++
    ```
    ```bash
    source ~/.bashrc
    ```

### Dependencies
```bash
# boost and ...
sudo yum install gcc gcc-c++ vim make cmake tmux git boost-devel python3-pip \
                 tbb-devel libatomic autoconf libevent-devel automake psmisc gdb
```

### Play
```bash
git clone git@github.com:cosmoss-jigu/durinn.git
cd $DURINN_HOME/script

# build durinn
./build.sh

# instrument apps
python3 instrument.py

# reproduce table 3, figure 11 and figure 12 in the paper
python3 table-3.py
python3 fig-11.py
python3 fig-12.py
```

After the script finishes, you can find following table and figures in the
*$DURINN_HOME/script* directory:
- table-2.csv
- fig-11.pdf
- fig-12.pdf
