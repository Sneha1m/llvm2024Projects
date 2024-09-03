# Project Name: Instrumentation of C/C++ Code Using LLVM/Clang and PAPI for Performance Monitoring

## Team Details
| **Name** |  **USN**  |
|:-----|:--------:|
| Srinidhi V  | 1RV21CS167 |
| T Varshith Chowdhary   |  1RV21CS174  |
| Tejas Kumar V   | 1RV21CS179 |
| Vikram R Patel   | 1RV21CS187 |

## Project Overview
This project involves the development of a tool to automatically instrument C/C++ code in order to gather Performance Monitoring Counters (PMC) data using the Performance Application Programming Interface (PAPI). The tool is designed to capture specific PMC events during function execution, specified via command-line arguments, and output the data in CSV format, including timestamps of the start and finish of each function's execution.

## Features
- **Automatic Instrumentation**: The tool parses the input C/C++ code to identify function entry and exit points and inserts calls to a runtime library that manages PAPI events.
- **PAPI Integration**: The tool uses PAPI to monitor hardware events such as instruction counts, cache misses, etc.
- **Customizable Events**: Users can specify which PMC events to track through command-line arguments.
- **CSV Output**: Outputs captured data per function invocation to a CSV file with timestamps and event counts.

## Prerequisites
- **LLVM/Clang**: Required for parsing and rewriting C/C++ code.
- **PAPI**: The Performance Application Programming Interface library.

## Installation

### 1. Install LLVM/Clang:
```bash
git clone https://github.com/llvm/llvm-project.git
```
Clones the official LLVM project repository from GitHub to your local machine.

```bash
cd llvm-project/ 
```
Changes the directory to the cloned llvm-project folder.

```bash
mkdir build/
```
Creates a build directory where the LLVM build files will be generated.

```bash
cd build/
```
Changes the directory to the newly created build directory.

```bash
cmake -G "Unix Makefiles" -DLLVM_ENABLE_PROJECTS="clang;" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../install -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++  ../llvm
```
Configures the LLVM build system using CMake, specifying Clang as the project to build, setting the build type to Release, and defining GCC as the C and C++ compilers.

```bash
make
```
Compiles the LLVM and Clang projects using the generated Makefiles.

```bash
make install
```
Installs the compiled LLVM and Clang binaries and libraries to the specified installation directory (../install).

```bash
export PATH=/path/to/llvm-project/install/bin:$PATH
export LD_LIBRARY_PATH=/path/to/llvm-project/install/lib:$LD_LIBRARY_PATH
```
Adds LLVM to your PATH and LD_LIBRARY_PATH environment variables.


### 2. PAPI:
```bash
git clone https://github.com/icl-utk-edu/papi.git
```
Clones the official PAPI (Performance Application Programming Interface) repository from GitHub to your local machine.

```bash
cd papi/src
```
Changes the directory to the src folder within the cloned papi directory, where the PAPI source code is located.

```bash
./configure 
```
Runs the configure script to detect the system environment and set up the build configuration for PAPI.

```bash
make 
```
Compiles the PAPI source code using the configured build settings.

```bash
sudo make install
```
Installs the compiled PAPI libraries and binaries to the system, requiring sudo for elevated permissions.

```bash
papi_avail
```
Verifies the installation by listing the available PAPI events on your system. If the installation is successful, this command will display a list of supported events.
## File Descriptions

- **runtime_library.h**:
  - **Purpose**: This header file defines the interface for the runtime library that interacts with PAPI (Performance Application Programming Interface) to collect hardware performance metrics.
  - **Contents**: It contains declarations for the functions that manage PAPI event sets, such as initializing PAPI, starting and stopping event counters, and recording performance data. It also defines any necessary data structures, such as the `EventData` structure, which holds information about the events being monitored and the results of the monitoring.

- **runtime_library.c**:
  - **Purpose**: This source file provides the implementation of the runtime library declared in `runtime_library.h`. It is responsible for executing the logic required to gather performance data using PAPI.
  - **Contents**: The file includes code to initialize PAPI, set up and manage event sets, and handle the start and stop of event counters at function entry and exit points. It also allocates memory for storing event data and writes the collected data to a CSV file. This file is crucial for the actual runtime measurement of performance metrics during the execution of the instrumented program.

- **InstrumentationTool.cpp**:
  - **Purpose**: This C++ source file implements the tool that automates the instrumentation of C/C++ code using LLVM and Clang. It modifies the target source code to insert calls to the runtime library at appropriate points.
  - **Contents**: The file contains the LLVM-based logic to parse the Abstract Syntax Tree (AST) of the target C/C++ code. It identifies function entry and exit points and injects calls to the runtime library to start and stop PAPI event counters. Additionally, it handles command-line arguments, allowing users to specify which PMC (Performance Monitoring Counter) events to monitor. This file is central to the project's ability to modify source code automatically, enabling detailed performance monitoring without manual intervention.

## Usage
**1.Compile the Runtime Library:**
```bash
gcc -shared -o libruntime.so -fPIC runtime_library.c -lpapi
```
**2.Compile the Instrumentation Tool:**
```bash
clang++ -o InstrumentationTool InstrumentationTool.cpp $(llvm-config --cxxflags --ldflags --libs all) -lclang-cpp -lclang
```
**3.Instrument the Target Code:**
```bash
./InstrumentationTool target_code.c > instrumented_code.c
```
**4.Compile the Instrumented Program:**
```bash
clang -o instrumented_program instrumented_code.c -L. -lruntime -lpapi
```
**5.Run the Instrumented Program**
```bash
./instrumented_program -trace-papievents="PAPI_TOT_CYC,PAPI_L1_DCM,PAPI_TOT_INS"
```
Specify the required events to be monitored as comma-separated arguments after -trace-papievents.

## Expected Output
The output will be a CSV file that logs the function name, start timestamp, end timestamp, and the captured PAPI event counts per function invocation. The format of this file will look something like this:
```bash
function_name,start_timestamp,end_timestamp,PAPI_TOT_CYC,PAPI_L1_DCM,PAPI_TOT_INS
foo,1623651123.123,1623651123.456,50000,100,3000
bar,1623651123.789,1623651124.012,75000,150,4500
```