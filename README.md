# Temporal Password Generator

A C++11 experiment that derives a printable password by evaluating a configurable, tick-by-tick computation. The generated source contains the roller parameters and the algorithm, **not a precomputed password string**. It produces the password only after running the configured number of ticks.

The tick count measures the amount of work requested; it does not promise a fixed wall-clock delay. Actual runtime depends on the computer, compiler, and math library.

## How it works

1. Choose the number of rollers (5–20) and ticks (1,000–100,000,000,000,000).
2. The generator creates roller speeds and writes `temporal_password_exe.cpp` in the current directory. The generated source embeds the parameters with enough decimal precision to reproduce the values.
3. In interactive mode or with `--generate`, the generator evaluates the computation once and prints the password when it finishes.
4. The emitted C++ source can be compiled separately. When run, that program performs the same computation and prints its result.

The generator does **not** invoke a shell, compiler, or generated executable. It does not automatically compile or run the emitted source.

## Algorithm

All roller positions start at zero. At each tick, the program first snapshots the previous positions, then updates each roller using its own speed and the next roller's previous position:

```text
For each tick:
  old_positions = positions
  For each roller i:
    position[i] = old_positions[i] + speed[i]
    position[i] += 0.3 * sin(old_positions[next] * 3.14159)
    position[i] = round(position[i] * 100) / 100
```

After the final tick, the 64-bit representations of the positions are mixed into a 64-bit FNV-style state. Each output character is selected from printable ASCII values 33–126. The password length equals the number of rollers.

For identical parameters and speeds, the computation is deterministic. The implementation advances ticks in order; this describes how this program evaluates the recurrence and is not a proof that no alternative implementation or acceleration is possible. Updates within one tick read the old state and write separate positions.

## Dependencies and requirements

- C++11-compatible compiler, such as g++ or clang++.
- C++ standard library only; no third-party libraries.
- A compiler is required to build the program and, separately, to compile the generated source. The running generator itself does not launch a compiler.
- The implementation requires 64-bit IEEE-754 `double` values; this is checked at compile time.

## Build the generator

```bash
g++ -std=c++11 -O2 -Wall -Wextra -Wpedantic \
    temporal_password_generator.cpp -o temporal_password_generator
```

Or with Clang:

```bash
clang++ -std=c++11 -O2 -Wall -Wextra -Wpedantic \
    temporal_password_generator.cpp -o temporal_password_generator
```

## Usage

### Interactive mode

```bash
./temporal_password_generator
```

The program prompts for the roller count and tick count, writes `temporal_password_exe.cpp`, estimates the runtime, then computes and prints the password. The estimate is rough; the calculation itself takes the time required by the selected hardware.

### Generate source and calculate the password

```bash
./temporal_password_generator --generate 5 1000 12345
```

Arguments are `--generate R T [SEED]`: roller count, tick count, and an optional 32-bit seed. Supplying a seed makes the generated roller speeds reproducible. Without a seed, the program obtains one from `std::random_device`.

### Generate source only

```bash
./temporal_password_generator --emit 5 1000 12345
```

`--emit` writes `temporal_password_exe.cpp` but does not calculate or print the password in the generator process. The emitted program still performs the full computation when compiled and run.

### Compile and run the emitted source

```bash
g++ -std=c++11 -O2 temporal_password_exe.cpp -o temporal_password_exe
./temporal_password_exe
```

### Help and self-test

```bash
./temporal_password_generator --help
./temporal_password_generator --self-test
```

The self-test checks that fixed inputs produce a deterministic, correctly sized password using printable ASCII. It is a smoke test, not a benchmark or a cryptographic audit.

## Runtime estimates

The generator estimates runtime using a fixed reference rate of about 11.45 million roller updates per second. This is a rough estimate, not an automatic benchmark of the current machine.

| Approximate target | Rollers | Ticks | Password length |
|---|---:|---:|---:|
| ~6.5 seconds | 15 | 5,000,000 | 15 characters |
| ~2.2 minutes | 15 | 100,000,000 | 15 characters |
| ~30 days | 20 | 1,500,000,000,000 | 20 characters |
| ~1 year | 20 | 18,000,000,000,000 | 20 characters |
| ~5.5 years (maximum tick count) | 20 | 100,000,000,000,000 | 20 characters |

The generator's calculation and the emitted program each perform their own complete computation when run. Thus, if you use `--generate` and later run the emitted program, each run incurs the configured work once.

## Output and reproducibility

- `temporal_password_exe.cpp` is written to the current working directory and contains hardcoded roller speeds and tick count.
- The generator prints the speeds at round-trip precision so they can be reproduced from the generated source.
- The output is deterministic for identical roller speeds, tick count, compiler/runtime behavior, and floating-point math results.

## Scope and security notes

This project demonstrates a configurable computational delay. Its generated source does not contain the password as a literal; the program calculates and prints the output after the ticks. The algorithm and parameters are visible in the generated source.

The parameter generator uses `std::mt19937`, which is not a cryptographic random-number generator, and the final mixing uses FNV-style arithmetic, which is not a cryptographic hash. Do not use this implementation to generate production passwords, wallet keys, encryption keys, or other security-critical secrets. The project does not implement a verifiable delay function (VDF), provide a proof of work, or establish a formal guarantee against shortcuts.

