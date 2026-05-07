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
    if (internal_terms.empty()) {
        internal_terms = {{0, 0}};
        return;
    }

    std::sort(internal_terms.begin(), internal_terms.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });

    std::vector<std::pair<power, coeff>> combined;
    combined.reserve(internal_terms.size());

    for (const auto& term : internal_terms) {
        if (term.second == 0) continue;
        if (!combined.empty() && combined.back().first == term.first) {
            combined.back().second += term.second;
        } else {
            combined.push_back(term);
        }
    }

    internal_terms.clear();
    for (const auto& t : combined) {
        if (t.second != 0) internal_terms.push_back(t);
    }

    if (internal_terms.empty()) internal_terms = {{0, 0}};
}

size_t polynomial::find_degree_of() {
    normalize();
    return internal_terms[0].first;
}

std::vector<std::pair<power, coeff>> polynomial::canonical_form() const {
    return internal_terms;
}

std::vector<std::pair<power, coeff>> merge_sorted_terms(
    const std::vector<std::pair<power, coeff>>& a, 
    const std::vector<std::pair<power, coeff>>& b) {
    
    std::vector<std::pair<power, coeff>> merged;
    merged.reserve(a.size() + b.size());
    size_t i = 0, j = 0;

    while (i < a.size() && j < b.size()) {
        if (a[i].first > b[j].first) {
            merged.push_back(a[i++]);
        } else if (a[i].first < b[j].first) {
            merged.push_back(b[j++]);
        } else {
            coeff sum = a[i].second + b[j].second;
            if (sum != 0) merged.push_back({a[i].first, sum});
            i++; j++;
        }
    }
    while (i < a.size()) merged.push_back(a[i++]);
    while (j < b.size()) merged.push_back(b[j++]);

    return merged;
}

// THREADING LOGIC 

void* polynomial::multiply_worker_hybrid(void* arg) {
    ThreadArgs* data = static_cast<ThreadArgs*>(arg);
    bool is_dense = !data->local_accumulator.empty();

    for (size_t i = data->start; i < data->end; ++i) {
        auto const &t1 = (*data->p1)[i];
        for (auto const &t2 : *(data->p2)) {
            power p = t1.first + t2.first;
            coeff c = t1.second * t2.second;
            if (is_dense) {
                data->local_accumulator[p] += c;
            } else {
                data->partial_vec.push_back({p, c});
            }
        }
    }
    return nullptr;
}

// OPERATORS 

polynomial polynomial::operator+(const polynomial &other) const {
    polynomial result = *this;
    result.internal_terms.insert(result.internal_terms.end(), 
                                 other.internal_terms.begin(), 
                                 other.internal_terms.end());
    result.normalize();
    return result;
}

polynomial polynomial::operator*(const polynomial &other) const {
    // Zero Check
    if ((internal_terms.size() == 1 && internal_terms[0].second == 0) ||
        (other.internal_terms.size() == 1 && other.internal_terms[0].second == 0)) {
        return polynomial();
    }

    size_t max_pow = internal_terms[0].first + other.internal_terms[0].first;
    const int NUM_THREADS = 4;
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    
    size_t n = internal_terms.size();
    size_t chunk_size = (n + NUM_THREADS - 1) / NUM_THREADS;

    // Determine Strategy: Sparse vs Dense 
    bool use_dense = (max_pow < 1000000);

    for (int i = 0; i < NUM_THREADS; ++i) {
        args[i].p1 = &internal_terms;
        args[i].p2 = &other.internal_terms;
        args[i].start = i * chunk_size;
        args[i].end = std::min(args[i].start + chunk_size, n);
        if (use_dense) {
            args[i].local_accumulator.assign(max_pow + 1, 0);
        }
        pthread_create(&threads[i], nullptr, multiply_worker_hybrid, &args[i]);
    }

    // Join and Combine
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], nullptr);
    }

    polynomial final_result;
    if (use_dense) {
        // Combine the 4 local accumulators into one final vector
        for (size_t p = 0; p <= max_pow; ++p) {
            coeff total_c = 0;
            for (int i = 0; i < NUM_THREADS; ++i) {
                total_c += args[i].local_accumulator[p];
            }
            if (total_c != 0) {
                // Building from max_pow down ensures we are already sorted
                final_result.internal_terms.push_back({(power)(max_pow - (max_pow - p)), total_c});
            }
        }
        std::reverse(final_result.internal_terms.begin(), final_result.internal_terms.end());
    } else {
        // Sparse Fallback
        for (int i = 0; i < NUM_THREADS; ++i) {
            final_result.internal_terms.insert(final_result.internal_terms.end(), 
                                               args[i].partial_vec.begin(), 
                                               args[i].partial_vec.end());
        }
    }

    final_result.normalize(); // Finalize canonical form
    return final_result;
}

polynomial polynomial::operator*(int constant) const {
    if (constant == 0) return polynomial();
    polynomial result = *this;
    for (auto& term : result.internal_terms) {
        term.second *= constant;
    }
    result.normalize();
    return result;
}

polynomial polynomial::operator%(const polynomial &other) const {
    polynomial r = *this;
    const polynomial &d = other;

    while (true) {
        auto r_can = r.canonical_form();
        auto d_can = d.canonical_form();

        // Degree condition for polynomial long division [cite: 31-33]
        if ((r_can.size() == 1 && r_can[0].second == 0) || (r_can[0].first < d_can[0].first)) {
            break;
        }

        coeff lead_coeff = r_can[0].second / d_can[0].second;
        power lead_pow = r_can[0].first - d_can[0].first;

        std::vector<std::pair<power, coeff>> term_vec = {{lead_pow, lead_coeff}};
        polynomial term_poly(term_vec.begin(), term_vec.end());
        
        // Subtract by adding the negative product
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