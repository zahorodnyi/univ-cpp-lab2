/*
 * Compiler: GCC 15
 * Standard: C++20
 */

#include <iostream>
#include <vector>
#include <numeric>
#include <execution>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>
#include <limits>

using DataType = long long;

std::vector<DataType> generate_data(size_t size) {
    std::vector<DataType> data(size);
    std::mt19937 gen(42);
    std::uniform_int_distribution<DataType> dist(1, 10);
    for (auto& val : data) {
        val = dist(gen);
    }
    return data;
}

template <typename Func>
double measure_ms(Func func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

void custom_exclusive_scan(const std::vector<DataType>& input, std::vector<DataType>& output, int K) {
    size_t n = input.size();
    if (n == 0) return;
    if (K <= 0) K = 1;
    if (static_cast<size_t>(K) > n) K = static_cast<int>(n);

    std::vector<std::thread> threads;
    std::vector<DataType> chunk_sums(K);
    size_t chunk_size = n / K;

    threads.reserve(K);
    for (int i = 0; i < K; ++i) {
        threads.emplace_back([&, i]() {
            auto start = input.begin() + i * chunk_size;
            auto end = (i == K - 1) ? input.end() : input.begin() + (i + 1) * chunk_size;
            chunk_sums[i] = std::reduce(std::execution::seq, start, end, DataType{0});
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    threads.clear();

    std::vector<DataType> offsets(K);
    std::exclusive_scan(std::execution::seq, chunk_sums.begin(), chunk_sums.end(), offsets.begin(), DataType{0});

    for (int i = 0; i < K; ++i) {
        threads.emplace_back([&, i]() {
            auto start_in = input.begin() + i * chunk_size;
            auto end_in = (i == K - 1) ? input.end() : input.begin() + (i + 1) * chunk_size;
            auto start_out = output.begin() + i * chunk_size;
            std::exclusive_scan(std::execution::seq, start_in, end_in, start_out, offsets[i]);
        });
    }
    for (auto& t : threads) {
        t.join();
    }
}

void run_test(size_t size) {
    std::cout << "\nDataset Size: " << size << "\n";
    auto input = generate_data(size);
    std::vector<DataType> output(size);
    
    double t_none = measure_ms([&]() {
        std::exclusive_scan(input.begin(), input.end(), output.begin(), DataType{0});
    });
    std::cout << "No policy:           " << t_none << " ms\n";

    double t_seq = measure_ms([&]() {
        std::exclusive_scan(std::execution::seq, input.begin(), input.end(), output.begin(), DataType{0});
    });
    std::cout << "std::execution::seq: " << t_seq << " ms\n";

    double t_par = measure_ms([&]() {
        std::exclusive_scan(std::execution::par, input.begin(), input.end(), output.begin(), DataType{0});
    });
    std::cout << "std::execution::par: " << t_par << " ms\n";

    std::cout << "Custom Algorithm:\n";
    std::cout << "K\tTime(ms)\tSpeedup\n";

    int hw = std::thread::hardware_concurrency();
    std::vector<int> k_values;
    for (int i = 1; i <= hw * 2; ++i) {
        k_values.push_back(i);
    }
    k_values.push_back(hw * 4);

    double min_time = std::numeric_limits<double>::max();
    int best_k = 1;

    for (int k : k_values) {
        double t_custom = measure_ms([&]() {
            custom_exclusive_scan(input, output, k);
        });

        if (t_custom < min_time) {
            min_time = t_custom;
            best_k = k;
        }

        std::cout << k << "\t" << std::fixed << std::setprecision(2) << t_custom 
                  << "\t\t" << (t_seq / t_custom) << "\n";
    }

    std::cout << "Best K: " << best_k << "\n";
    std::cout << "Ratio (Best K / Threads): " << (double)best_k / hw << "\n";
}

int main() {
    run_test(100'000);
    run_test(10'000'000);
    run_test(50'000'000);
    return 0;
}