# Temporal Password Generator

Small C++11 program that generates roller parameters, writes a standalone C++ source file, and can calculate the corresponding password.

## Dependencies

- No third-party libraries.
- The program uses only the C++11 standard library.
- A C++11 compiler is needed to build the generator and, separately, to compile the emitted source into an executable. The generator itself does **not** invoke a compiler, a shell, or another program at runtime.

The generated file, `temporal_password_exe.cpp`, is self-contained apart from the standard C++ library.

## Build

```bash
g++ -std=c++11 -O2 -Wall -Wextra -Wpedantic \
    temporal_password_generator.cpp -o temporal_password_generator
```

Clang can be used instead of g++:

```bash
clang++ -std=c++11 -O2 temporal_password_generator.cpp \
    -o temporal_password_generator
```

## Run

Interactive mode writes the generated source and calculates the password once in the generator process:

```bash
./temporal_password_generator
```

The accepted ranges are 5–20 rollers and 1,000–100,000,000,000,000 ticks.

To write the source without running the tick loop:

```bash
./temporal_password_generator --emit 5 1000 12345
```

To write the source and calculate the password once:

```bash
./temporal_password_generator --generate 5 1000 12345
```

The optional third argument is a 32-bit seed, useful for reproducing tests. Omit it to seed parameter generation from `std::random_device`.

The emitted program is compiled separately:

```bash
g++ -std=c++11 -O2 -Wall -Wextra -Wpedantic \
    temporal_password_exe.cpp -o temporal_password_exe
./temporal_password_exe
```

## Tests

The built-in smoke test checks determinism and output length/character range for fixed inputs:

```bash
./temporal_password_generator --self-test
```

For an end-to-end test, use a small tick count, compile the emitted source, and compare its `PASSWORD:` line with the generator's output. A fixed seed makes the parameters reproducible.

## Algorithm

For each tick, each roller is updated from the previous tick's positions using its fixed speed, a coupling term based on the next roller, and rounding to two decimal places. The final positions are mixed into a 64-bit FNV-style state and mapped to printable ASCII characters.

The update recurrence and hash behavior in this version have been kept unchanged. Time estimates are approximate and depend on compiler, math library, and hardware; the configured tick count is a work parameter, not a guaranteed wall-clock duration.

## License

MIT

## Author

xdanielex
