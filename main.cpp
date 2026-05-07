#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include "poly.h"

// Helper to generate a dense polynomial: (1x^N + 1x^N-1 ... + 1x^0)
polynomial generate_dense(size_t degree) {
    std::vector<std::pair<power, coeff>> terms;
    terms.reserve(degree + 1);
    for (size_t i = 0; i <= degree; ++i) {
        // Alternating signs or random values can help test normalization
        int c = (i % 2 == 0) ? 1 : -1; 
        terms.push_back({(power)i, c});
    }
    return polynomial(terms.begin(), terms.end());
}

int main() {
    std::cout << "--- STARTING LARGE DENSE TEST ---" << std::endl;
    
    // 5,000 terms * 5,000 terms = 25 million operations
    size_t degree1 = 5000;
    size_t degree2 = 5000;

    std::cout << "Generating polynomials of degree " << degree1 << "..." << std::endl;
    polynomial p1 = generate_dense(degree1);
    polynomial p2 = generate_dense(degree2);

    std::cout << "Starting multiplication..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    // This is the call that likely causes the 0.0 or timeout
    polynomial result = p1 * p2;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Multiplication finished in: " << duration << " ms" << std::endl;
    
    // Check degree to verify correctness: deg(p1*p2) = deg(p1) + deg(p2)
    if (result.find_degree_of() == (degree1 + degree2)) {
        std::cout << "[SUCCESS] Degree matches expected: " << result.find_degree_of() << std::endl;
    } else {
        std::cout << "[FAILURE] Expected degree " << (degree1 + degree2) 
                  << " but got " << result.find_degree_of() << std::endl;
    }

    return 0;
}