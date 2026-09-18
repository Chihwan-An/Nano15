#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
// Running statistics for validation printouts.
struct VAcc {
    double s = 0., s2 = 0., mn = 1e300, mx = -1e300;
    std::size_t n = 0;
    void add(double v) { s += v; s2 += v * v; ++n; mn = std::min(mn, v); mx = std::max(mx, v); }
    double mean() const { return n ? s / n : 0.; }
    double rms() const { if (n < 2) return 0.; double m = mean(), v = s2 / n - m * m; return v > 0. ? std::sqrt(v) : 0.; }
};
inline void vreport(const std::string &label, const VAcc &a) {
    std::cout << "  " << std::left << std::setw(46) << label << std::right << std::fixed << std::setprecision(5)
              << " mean " << std::setw(9) << a.mean() << " rms " << std::setw(8) << a.rms()
              << " min " << std::setw(9) << (a.n ? a.mn : 0.) << " max " << std::setw(9) << (a.n ? a.mx : 0.)
              << " n " << a.n << "\n";
}
inline void vcheck(const std::string &label, bool ok, const std::string &detail = "") {
    std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << label;
    if (!detail.empty()) std::cout << "  (" << detail << ")";
    std::cout << "\n";
}
inline bool relEq(double a, double b, double tol = 2e-5) { return std::fabs(a - b) <= tol * std::max(1., std::fabs(b)); }
