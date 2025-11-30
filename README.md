# Parallel Exclusive Scan Benchmark

This project implements and benchmarks the `exclusive_scan` operation using C++20. It compares the Standard Library's sequential and parallel policies against a custom multi-threaded Map-Reduce implementation.

## Build and Run

**Requirements:** C++20 compiler (GCC/Clang/MSVC) and CMake.

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./univ_cpp_lab2
```

# Benchmark Analysis & Configuration

## 1. Benchmark Results

The following results were obtained on a **MacBook M1** (8 Cores). The tests compare the Standard Library sequential/parallel policies against a custom Map-Reduce implementation with varying thread counts ($K$).

### Performance Summary
| Dataset Size | `std::execution::seq` | `std::execution::par` | Custom Algo (Best) | Speedup (vs Seq) |
| :--- | :--- | :--- | :--- | :--- |
| **100,000** | 0.034 ms | **0.034 ms** | 0.09 ms | ~1.0x (None) |
| **10,000,000** | 10.52 ms | **4.32 ms** | 4.65 ms | 2.43x |
| **50,000,000** | 106.14 ms | **19.49 ms** | 23.92 ms | 5.44x |

### Detailed Analysis by Dataset

**Small Dataset (100k elements)**
* **Result:** The overhead of thread management outweighs the computational benefit.
* **Observation:** The sequential and parallel STL implementations performed identically (0.034 ms), while the custom algorithm was significantly slower (0.09 ms) due to the cost of spawning threads for a small workload.

**Medium Dataset (10M elements)**
* **Result:** Parallelization begins to show clear benefits.
* **Observation:** Both the STL Parallel policy and the Custom Algorithm achieved a ~2x speedup over sequential execution. The Custom Algorithm (Best $K=3$) was nearly as fast as the optimized STL parallel policy.

**Large Dataset (50M elements)**
* **Result:** Maximum scalability achieved.
* **Observation:** `std::execution::par` is the clear winner with a **5.4x speedup**. The Custom Algorithm peaked at a **4.4x speedup** with $K=3$.
* **Scaling Drop-off:** Performance for the custom algorithm degrades as $K$ increases beyond the physical core count (performance drops significantly after $K=8$), confirming the cost of context switching when oversubscribing threads.

### Custom Algorithm Scaling (50M Dataset)
The custom implementation uses a "K chunks" approach. The "sweet spot" for this machine appears to be between **3 and 7 threads**.

| Threads (K) | Time (ms) | Speedup | Note |
| :--- | :--- | :--- | :--- |
| 1 | 32.57 | 3.26 | Baseline threaded overhead |
| **3** | **23.92** | **4.44** | **Peak Performance** |
| 4 | 24.36 | 4.36 | Stable |
| 8 | 28.52 | 3.72 | Efficiency begins to drop |
| 16 | 26.41 | 4.02 | High context switching |
| 32 | 28.31 | 3.75 | Oversubscription |

> **Note:** The "Ratio (Best K / Threads)" metric suggests optimal efficiency is reached when using approximately 30-40% of the available hardware concurrency for this specific memory-bound task.

---
