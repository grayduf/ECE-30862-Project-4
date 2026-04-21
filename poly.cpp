#include "poly.h"
#include <algorithm>
#include <iostream>

// Default Constructor: Represents 0*x^0
polynomial::polynomial() {
    internal_terms.push_back({0, 0});
}

// Copy Constructor
polynomial::polynomial(const polynomial &other) : internal_terms(other.internal_terms) {}

// Assignment Operator
polynomial &polynomial::operator=(const polynomial &other) {
    if (this != &other) {
        internal_terms = other.internal_terms;
    }
    return *this;
}

// Normalize the polynomial by removing zero-coefficient terms and sorting by power
void polynomial::normalize() {
    // Remove zero-coefficient terms except for the zero polynomial case
    auto it = std::remove_if(internal_terms.begin(), internal_terms.end(), 
                             [](const std::pair<power, coeff>& term) {
                                 return term.second == 0;
                             });
    internal_terms.erase(it, internal_terms.end());

    // If all terms were removed, we need to add back the zero polynomial term
    if (internal_terms.empty()) {
        internal_terms.push_back({0, 0});
        return;
    }

    // Sort terms by power in descending order
    std::sort(internal_terms.begin(), internal_terms.end(), 
              [](const auto& a, const auto& b) {
                  return a.first > b.first;
              });
}

size_t polynomial::find_degree_of() {
    normalize();
    return internal_terms[0].first;
}

// polynomial + int
polynomial polynomial::operator+(int constant) const {
    std::vector<std::pair<power, coeff>> vec = {{0, constant}};
    return *this + polynomial(vec.begin(), vec.end());
}

// int + polynomial is handled by the friend function
polynomial operator+(int constant, const polynomial &poly) {
    return poly + constant;
}

// int * polynomial is handled by the friend function
polynomial operator*(int constant, const polynomial &poly) {
    return poly * constant;
}

std::vector<std::pair<power, coeff>> polynomial::canonical_form() const {
    // Return a copy of the internal terms, which are already normalized and sorted
    return internal_terms;
}

polynomial polynomial::operator+(const polynomial &other) const {
    polynomial result = *this;
    
    // Add terms from the other polynomial to the result
    for (const auto& term : other.internal_terms) {
        bool found = false;
        for (auto& res_term : result.internal_terms) {
            if (res_term.first == term.first) {
                res_term.second += term.second;
                found = true;
                break;
            }
        }
        if (!found) {
            result.internal_terms.push_back(term);
        }
    }
    
    result.normalize(); // Ensure the result is in canonical form
    return result;
}

polynomial polynomial::operator*(const polynomial &other) const {
    std::vector<std::pair<power, coeff>> combined;
    
    for (const auto& t1 : this->internal_terms) {
        for (const auto& t2 : other.internal_terms) {
            power new_pow = t1.first + t2.first;
            coeff new_coeff = t1.second * t2.second;
            
            // Combine like terms in the combined vector
            bool found = false;
            for (auto& c_term : combined) {
                if (c_term.first == new_pow) {
                    c_term.second += new_coeff;
                    found = true;
                    break;
                }
            }
            if (!found) {
                combined.push_back({new_pow, new_coeff});
            }
        }
    }
    
    // Use the iterator constructor we built to create the result
    polynomial result(combined.begin(), combined.end());
    return result;
}

// Multiplication by integer implementation
polynomial polynomial::operator*(int constant) const {
    polynomial result = *this;
    for (auto& term : result.internal_terms) {
        term.second *= constant;
    }
    result.normalize();
    return result;
}

// Modulo Implementation 
polynomial polynomial::operator%(const polynomial &other) const {
    polynomial r = *this;
    const polynomial &d = other;

    while (true) {
        auto r_can = r.canonical_form();
        auto d_can = d.canonical_form();

        // Stop if remainder is zero or degree(r) < degree(d)
        if ((r_can.size() == 1 && r_can[0].second == 0) || (r_can[0].first < d_can[0].first)) {
            break;
        }

        // Calculate leading term for division 
        coeff lead_coeff = r_can[0].second / d_can[0].second;
        power lead_pow = r_can[0].first - d_can[0].first;

        std::vector<std::pair<power, coeff>> term_vec = {{lead_pow, lead_coeff}};
        polynomial term_poly(term_vec.begin(), term_vec.end());
        
        // r = r - (term_poly * d)
        r = r + (term_poly * d * -1);
    }
    return r;
}

void polynomial::print() const {
    for (const auto& term : internal_terms) {
        std::cout << term.second << "x^" << term.first << " ";
    }
    std::cout << std::endl;
}
