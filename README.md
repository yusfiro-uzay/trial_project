# CADU Solve - OQPSK/BPSK Demodulator

Software demodulator for OQPSK and BPSK modulated IQ baseband files with CCSDS frame support.

## Features

- OQPSK and BPSK demodulation
- 16-bit and 32-bit IQ input support
- Configurable RRC matched filtering
- Optional low-pass pre-filtering
- Costas loop carrier recovery
- Mueller & Müller timing recovery
- Auto-tuning of loop parameters (optional)
- Blind processing: Viterbi decoding, NRZ-M, PSR descrambling
- CCSDS frame sync and Reed-Solomon decoding
- UDP bit streaming (optional)

## Build Instructions

### Prerequisites

- CMake 3.15 or higher
- C++11 compatible compiler
- C99 compatible compiler

### Building on Windows

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Building on Linux

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

```bash
cadu_solve [options]
  -i FILE         Input IQ file
  -d NUM          Decimation factor
  --sps NUM       Samples per symbol
  --bpsk          BPSK demodulation mode
  --oqpsk         OQPSK demodulation mode (default)
  --iq16          16-bit IQ input format (default)
  --iq32          32-bit IQ input format
  --help          Show help message
```

## Compatibility

- Windows (MSVC)
- Linux (GCC/Clang)
- Cross-platform CMake build system

## License

See source files for license information.
