#include "PhysicsCalculator.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace phy104::PhysicsCalculator {
namespace {
constexpr double kDegToRad = std::numbers::pi / 180.0;
constexpr double kRadToDeg = 180.0 / std::numbers::pi;
/// Tolerance for floating-point comparison at the critical-angle boundary (sin θ₂ ≈ 1).
constexpr double kTirEpsilon = 1e-9;
}

RefractionResult refraction(double n1, double n2, double theta1Deg) {
    RefractionResult r;
    r.reflectedDeg = theta1Deg;
    if (n1 <= 0.0 || n2 <= 0.0) { r.valid = false; return r; }
    const double sinTheta2 = n1 * std::sin(theta1Deg * kDegToRad) / n2;   // Snell's law
    if (sinTheta2 > 1.0 + kTirEpsilon) {
        r.totalInternalReflection = true;
    } else {
        const double clamped = std::max(-1.0, std::min(1.0, sinTheta2));
        r.refractedDeg = std::asin(clamped) * kRadToDeg;    // θ₂ = 90° at the critical limit
    }
    return r;
}

FresnelResult fresnelReflectance(double n1, double n2, double theta1Deg) {
    FresnelResult fr;
    const auto ref = refraction(n1, n2, theta1Deg);
    fr.tir = ref.totalInternalReflection;
    if (fr.tir) {
        fr.Rs = 1.0; fr.Rp = 1.0; fr.R = 1.0;
        return fr;
    }
    const double t1 = theta1Deg * kDegToRad;
    const double t2 = ref.refractedDeg * kDegToRad;
    const double sum = t1 + t2;
    const double diff = t1 - t2;
    // Normal incidence: tan(0)/tan(0) is 0/0, use the known closed form.
    if (std::fabs(theta1Deg) < 1e-9) {
        const double r0 = (n1 - n2) / (n1 + n2);
        fr.Rs = r0 * r0;
        fr.Rp = r0 * r0;
        fr.R = r0 * r0;
        return fr;
    }
    // Fresnel equations for dielectric interface (no absorption).
    double rs = 0.0, rp = 0.0;
    const double sinSum = std::sin(sum);
    if (std::fabs(sinSum) > 1e-12) {
        const double sinDiff = std::sin(diff);
        rs = (sinDiff / sinSum) * (sinDiff / sinSum);
    }
    const double cosSum = std::cos(sum);
    if (std::fabs(cosSum) > 1e-12) {
        const double tanDiff = std::tan(diff);
        const double tanSum = std::tan(sum);
        rp = (tanDiff / tanSum) * (tanDiff / tanSum);
    }
    // Clamp tiny negative values from roundoff.
    fr.Rs = std::max(0.0, std::min(1.0, rs));
    fr.Rp = std::max(0.0, std::min(1.0, rp));
    fr.R = 0.5 * (fr.Rs + fr.Rp);
    return fr;
}

std::optional<double> criticalAngleDeg(double n1, double n2) {
    if (n1 <= n2 || n1 <= 0.0 || n2 <= 0.0) return std::nullopt;   // TIR only for n1 > n2
    return std::asin(n2 / n1) * kRadToDeg;
}

std::optional<double> refractiveIndex(double c, double v) {
    if (v == 0.0) return std::nullopt;
    return c / v;
}

PrismResult prism(double np, double nm, double apexDeg, double i1Deg) {
    PrismResult r;
    r.i1 = i1Deg;
    if (np <= 0.0 || nm <= 0.0 || apexDeg <= 0.0 || apexDeg >= 180.0) { r.valid = false; return r; }
    const double s1 = nm * std::sin(i1Deg * kDegToRad) / np;
    if (std::fabs(s1) > 1.0) { r.entryTir = true; return r; }
    r.r1 = std::asin(s1) * kRadToDeg;
    r.r2 = apexDeg - r.r1;
    const double s2 = np * std::sin(r.r2 * kDegToRad) / nm;
    if (std::fabs(s2) > 1.0) { r.exitTir = true; return r; }
    r.i2 = std::asin(s2) * kRadToDeg;
    r.deviation = r.i1 + r.i2 - apexDeg;
    return r;
}

std::optional<double> minimumDeviationDeg(double np, double nm, double apexDeg) {
    if (np <= 0.0 || nm <= 0.0 || apexDeg <= 0.0 || apexDeg >= 180.0) return std::nullopt;
    const double s = (np / nm) * std::sin(apexDeg * kDegToRad / 2.0);
    if (s > 1.0) return std::nullopt;
    return 2.0 * std::asin(s) * kRadToDeg - apexDeg;
}

std::optional<double> minimumDeviationIncidenceDeg(double np, double nm, double apexDeg) {
    const auto d = minimumDeviationDeg(np, nm, apexDeg);
    if (!d) return std::nullopt;
    return (apexDeg + *d) / 2.0;
}

std::optional<double> indexFromMinimumDeviation(double deltaMinDeg, double apexDeg) {
    const double den = std::sin(apexDeg * kDegToRad / 2.0);
    if (den == 0.0) return std::nullopt;
    return std::sin((apexDeg + deltaMinDeg) * kDegToRad / 2.0) / den;
}

double cauchyIndex(double a, double b, double wavelengthNm) {
    const double um = wavelengthNm / 1000.0;
    return a + b / (um * um);
}

ImageResult imageFormation(double s, double f) {
    ImageResult r;
    if (f == 0.0 || s == 0.0) {
        r.valid = false;
        r.description = f == 0.0 ? "Invalid focal length (f = 0)" : "Invalid object distance (s = 0)";
        return r;
    }
    if (std::fabs(s - f) < 1e-9) {                    // object at the focal point
        r.atInfinity = true;
        r.description = "Image at infinity (object at the focal point)";
        return r;
    }
    r.imageDistance = 1.0 / (1.0 / f - 1.0 / s);      // 1/s + 1/s' = 1/f
    r.magnification = -r.imageDistance / s;           // m = -s'/s
    r.real    = r.imageDistance > 0.0;
    r.upright = r.magnification > 0.0;

    const double absM = std::fabs(r.magnification);
    const char* size = absM > 1.0001 ? "enlarged" : (absM < 0.9999 ? "reduced" : "same size");
    r.description = std::string(r.real ? "Real" : "Virtual") + ", " +
                    (r.upright ? "upright" : "inverted") + ", " + size;
    return r;
}

double fringeSpacing(double lambda, double R, double d) {
    if (d == 0.0) return 0.0;
    return lambda * R / d;
}

double fringePosition(int m, double lambda, double R, double d) {
    if (d == 0.0) return 0.0;
    return m * lambda * R / d;
}

double relativeIntensity(double y, double lambda, double R, double d) {
    if (lambda * R == 0.0) return 0.0;
    const double phase = std::numbers::pi * d * y / (lambda * R);
    const double c = std::cos(phase);
    return c * c;                                      // I = I0 cos²(π d y / λR)
}

} // namespace phy104::PhysicsCalculator
