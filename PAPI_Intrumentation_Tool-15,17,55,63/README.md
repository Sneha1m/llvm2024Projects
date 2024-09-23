# Instrumentation of C/C++ Code Using LLVM/Clang and PAPI for Performance Monitoring


Performance optimization is crucial for high-performance 
computing applications. Instrumenting C/C++ code using LLVM/Clang along with 
the Performance Application Programming Interface (PAPI) provides a 
powerful approach to monitor and analyze performance metrics. This 
process involves integrating LLVM/Clang compiler infrastructure 
with PAPI to collect detailed performance data, allowing developers to 
identify bottlenecks, optimize code, and improve overall system efficiency.




NAME | USN 
--- | --- 
**Aditya Veer Singh** | **1RV21CS15**
**Akshaja V Maiya** | **1RV21CS017**
**Joshua Elias Alva** | **1RV21CS055**
**Khushboo Srivastava** | **1RV21CS063**

## Installation for PAPI
We prefer a linux distro over Windows as PAPI is more stable in linux.  
  
**Step 1**
```bash
sudo apt install papi-tools
git clone --depth 1 https://github.com/icl-utk-edu/papi.git
#check Install.txt
#or do
sudo apt-get install libpapi-dev
```
**Step 2**  
Check installation
```bash
which papi_avail
papi_avail
papi_native_avail
```
This gives you a list of paramters supported by your system.  
  
**Step 3**  
Run papi_test.c in the test folder.  
```bash
gcc papi_test.c -lpapi
./a.out
```
Output must indicate the Total CPU cycles taken.

## Installation for LLVM
You can choose to install clang and llvm using sudo. We built it as per our requirement.  
To build from scratch,  
**Step 1**  
Clone the official LLVM repository.
```bash
git clone --depth 1 https://github.com/llvm/llvm-project.git
``` 
**Step 2**  
Install a cmake version > 3.30.*  
Setup build using ninja if you've limited system resources like us.  
   
Optional Setup for Cmake 3.30.2
```bash
sudo apt install cmake
sudo apt remove --purge cmake
wget https://github.com/Kitware/CMake/releases/download/v3.30.2/cmake-3.30.2-Linux-x86_64.tar.gz
tar -zxvf cmake-3.30.2-Linux-x86_64.tar.gz
sudo mv cmake-3.30.2-linux-x86_64 /opt/cmake
sudo ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake
sudo ln -s /opt/cmake/bin/ctest /usr/local/bin/ctest
sudo ln -s /opt/cmake/bin/cpack /usr/local/bin/cpack
cmake --version
```
Setup Ninja
```bash
sudo apt install ninja-build
```
**Step 3**  
Setup LLVM
```bash
cd llvm-project
mkdir build
cd build
cmake -S ../llvm -B . -G "Ninja" -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld" -DCMAKE_INSTALL_PREFIX=Where_to_install -DCMAKE_BUILD_TYPE=Release -DLLVM_PARALLEL_LINK_JOBS=1 -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_INCLUDE_TESTS=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
``` 
This will generate build files.

**Step 4**  
Build LLVM 
```bash
ninja -j HOW_MANY_PARALLEL_BUILDS
```
**Step 5**  
Install LLVM
```bash
ninja install 
```

**Step 6**  
Include LLVM in PATH.  
Go to .bashrc  
```txt
export LD_LIBRARY_PATH=~/llvm-release/lib:$LD_LIBRARY_PATH
export LIBRARY_PATH=$LIBRARY_PATH:~/llvm-release/lib #optional for CMakeLists.txt
export PATH=~/llvm-release/bin:$PATH
export CPLUS_INCLUDE_PATH=~/llvm-release/include:$CPLUS_INCLUDE_PATH
export C_INCLUDE_PATH=~/llvm-release/include:$C_INCLUDE_PATH
export LLVM_INCLUDE_DIRS=~/llvm-release/include #optional for CMakeLists.txt
export LLVM_LIBS=~/llvm-release/lib  #optional for CMakeLists.txt
```
This concludes the installation.

## Running the program
Build your instrumentor tool by referring commands.txt.  
To check available papi_parameters, use papi_avail.  
We have provided namely two features, `-trace-papi-events="values separated by commas"` and `-output-file="name_of_file.csv"`.  

### Limitations
The runtime library used here for PAPI is not threadsafe and may give errors accordingly.
