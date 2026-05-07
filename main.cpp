#include <iostream>
#include <chrono>
#include <optional>
#include <vector>
#include "poly.h"

void run_test(const std::string& name, polynomial& p1, polynomial& p2, std::vector<std::pair<power, coeff>> solution) {
    std::cout << "Running Test: " << name << std::endl;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    polynomial p3 = p1 * p2;
    auto result = p3.canonical_form();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000.0;

    if (result == solution) {
        std::cout << "  [PASS] Timing: " << elapsed << " ms" << std::endl;
    } else {
        std::cout << "  [FAIL] Result mismatch." << std::endl;
        p3.print();
    }
}

int main() {
    // TEST 1: Sparse Polynomials (Efficiency Check)
    // (x^1000 + 1) * (x^1000 + 1) = x^2000 + 2x^1000 + 1
    std::vector<std::pair<power, coeff>> sparse_in = {{1000, 1}, {0, 1}};
    std::vector<std::pair<power, coeff>> sparse_sol = {{2000, 1}, {1000, 2}, {0, 1}};
    polynomial s1(sparse_in.begin(), sparse_in.end());
    polynomial s2(sparse_in.begin(), sparse_in.end());
    run_test("Sparse Multiplication", s1, s2, sparse_sol);

    // TEST 2: Modulo (Functionality Check)
    // (x^2 + 2x + 1) % (x + 1) should be 0
    std::cout << "Running Test: Modulo (x^2+2x+1) % (x+1)" << std::endl;
    std::vector<std::pair<power, coeff>> dividend_in = {{2, 1}, {1, 2}, {0, 1}};
    std::vector<std::pair<power, coeff>> divisor_in = {{1, 1}, {0, 1}};
    polynomial p_dividend(dividend_in.begin(), dividend_in.end());
    polynomial p_divisor(divisor_in.begin(), divisor_in.end());
    
    polynomial p_rem = p_dividend % p_divisor;
    auto rem_can = p_rem.canonical_form();
    std::vector<std::pair<power, coeff>> zero_sol = {{0, 0}};
    
    if (rem_can == zero_sol) {
        std::cout << "  [PASS] Modulo result is 0." << std::endl;
    } else {
        std::cout << "  [FAIL] Modulo result: ";
        p_rem.print();
    }

    // TEST 3: Large Dense Polynomial (Threading Stress Test)
    // Create (1x^100 + 1x^99 ... + 1x^0) * (1x^50 + ... + 1x^0)
    std::vector<std::pair<power, coeff>> large_p1_vec, large_p2_vec;
    for(int i=0; i<=100; ++i) large_p1_vec.push_back({(power)i, 1});
    for(int i=0; i<=50; ++i) large_p2_vec.push_back({(power)i, 1});
    
    polynomial lp1(large_p1_vec.begin(), large_p1_vec.end());
    polynomial lp2(large_p2_vec.begin(), large_p2_vec.end());
    
    std::cout << "Running Test: Large Dense Multiplication" << std::endl;
    std::chrono::steady_clock::time_point b = std::chrono::steady_clock::now();
    polynomial lp3 = lp1 * lp2;
    std::chrono::steady_clock::time_point e = std::chrono::steady_clock::now();
    std::cout << "  [DONE] Timing: " << std::chrono::duration_cast<std::chrono::milliseconds>(e - b).count() << " ms" << std::endl;

    return 0;
}