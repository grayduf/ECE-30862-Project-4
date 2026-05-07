#include "poly.h"
#include <algorithm>
#include <iostream>
#include <pthread.h>

// CONSTRUCTORS

polynomial::polynomial() {
    internal_terms.push_back({0, 0});
}

polynomial::polynomial(const polynomial &other) : internal_terms(other.internal_terms) {}

polynomial &polynomial::operator=(const polynomial &other) {
    if (this != &other) {
        internal_terms = other.internal_terms;
    }
    return *this;
}

// CORE UTILITIES 

void polynomial::normalize() {
    // 1. Remove terms with zero coefficients [cite: 27]
    auto it = std::remove_if(internal_terms.begin(), internal_terms.end(), 
                             [](const std::pair<power, coeff>& term) {
                                 return term.second == 0;
                             });
    internal_terms.erase(it, internal_terms.end());

    // 2. Handle Zero Polynomial exception [(0,0)] [cite: 28]
    if (internal_terms.empty()) {
        internal_terms.push_back({0, 0});
        return;
    }

    // 3. Sort by power descending
    std::sort(internal_terms.begin(), internal_terms.end(), 
              [](const auto& a, const auto& b) {
                  return a.first > b.first;
              });
    
    // 4. Consolidate duplicate powers
    std::vector<std::pair<power, coeff>> combined;
    for (const auto& term : internal_terms) {
        if (!combined.empty() && combined.back().first == term.first) {
            combined.back().second += term.second;
        } else {
            combined.push_back(term);
        }
    }
    internal_terms = combined;
}

size_t polynomial::find_degree_of() {
    normalize();
    return internal_terms[0].first;
}

std::vector<std::pair<power, coeff>> polynomial::canonical_form() const {
    return internal_terms;
}

// THREADING LOGIC 

void* polynomial::multiply_worker(void* arg) {
    ThreadArgs* data = static_cast<ThreadArgs*>(arg);
    
    for (size_t i = data->start; i < data->end; ++i) {
        auto const &t1 = (*data->p1)[i];
        for (auto const &t2 : *(data->p2)) {
            power new_pow = t1.first + t2.first;
            coeff new_coeff = t1.second * t2.second;
            
            // Local accumulation to avoid mutex contention
            bool found = false;
            for (auto &res : data->partial_result) {
                if (res.first == new_pow) {
                    res.second += new_coeff;
                    found = true;
                    break;
                }
            }
            if (!found) {
                data->partial_result.push_back({new_pow, new_coeff});
            }
        }
    }
    return nullptr;
}

// OPERATORS 

polynomial polynomial::operator+(const polynomial &other) const {
    polynomial result = *this;
    for (const auto& term : other.internal_terms) {
        result.internal_terms.push_back(term);
    }
    result.normalize();
    return result;
}

polynomial polynomial::operator*(const polynomial &other) const {
    const int NUM_THREADS = 4; // Adjust based on Purdue's lab environment performance
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    
    size_t n = internal_terms.size();
    size_t chunk_size = (n + NUM_THREADS - 1) / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; ++i) {
        args[i].p1 = &internal_terms;
        args[i].p2 = &other.internal_terms;
        args[i].start = i * chunk_size;
        args[i].end = std::min(args[i].start + chunk_size, n);
        
        pthread_create(&threads[i], nullptr, polynomial::multiply_worker, &args[i]);
    }

    polynomial final_result;
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], nullptr);
        // Sequential merge of partial results
        for (const auto& term : args[i].partial_result) {
            final_result.internal_terms.push_back(term);
        }
    }

    final_result.normalize();
    return final_result;
}

// Support for polynomial * int
polynomial polynomial::operator*(int constant) const {
    polynomial result = *this;
    for (auto& term : result.internal_terms) {
        term.second *= constant;
    }
    result.normalize();
    return result;
}

// Polynomial Modulo using Long Division 
polynomial polynomial::operator%(const polynomial &other) const {
    polynomial r = *this;
    const polynomial &d = other;

    while (true) {
        auto r_can = r.canonical_form();
        auto d_can = d.canonical_form();

        // Stop when degree of remainder < degree of divisor
        if ((r_can.size() == 1 && r_can[0].second == 0) || (r_can[0].first < d_can[0].first)) {
            break;
        }

        coeff lead_coeff = r_can[0].second / d_can[0].second;
        power lead_pow = r_can[0].first - d_can[0].first;

        std::vector<std::pair<power, coeff>> term_vec = {{lead_pow, lead_coeff}};
        polynomial term_poly(term_vec.begin(), term_vec.end());
        
        r = r + (term_poly * d * -1);
    }
    return r;
}

// HELPER OVERLOADS

polynomial polynomial::operator+(int constant) const {
    polynomial result = *this;
    result.internal_terms.push_back({0, constant});
    result.normalize();
    return result;
}

polynomial operator+(int constant, const polynomial &poly) { return poly + constant; }
polynomial operator*(int constant, const polynomial &poly) { return poly * constant; }

void polynomial::print() const {
    for (const auto& term : internal_terms) {
        std::cout << term.second << "x^" << term.first << " ";
    }
    std::cout << std::endl;
}