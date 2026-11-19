#pragma once
// The six wave-optics screens of the second Figma batch: Newton's rings, Michelson interferometer,
// thin film, diffraction grating, single slit, Malus / Brewster. Pure C++: registers, notices and chart
// data are returned as plain structs; the pages only paint them.
#include "ExperimentReports.h"

#include <string>
#include <utility>
#include <vector>

namespace phy104 {

/// What the chart panel draws: a polyline, an optional highlighted sample and its axes.
struct ChartData {
    std::string yLabel, xLabel, legend;
    double xMin = 0, xMax = 1, yMin = 0, yMax = 1;
    int xTicks = 4, yTicks = 2, xDecimals = 1, yDecimals = 1;
    std::vector<std::pair<double, double>> curve;
    std::vector<std::pair<double, double>> curve2;      // second curve (e.g. λ₂ intensity)
    std::vector<std::pair<double, double>> marker;      // 0 or 1 points (selected order, λ₁)
    std::string legend2;                                // legend for second curve
};

/// A report plus its chart (the diagram is drawn by the page from the same inputs).
struct ModelReport {
    Report report;
    ChartData chart;
};

/// `transmitted` picks the second mode tab: reflected (false) or transmitted (true) light.
struct NewtonInput    {
    double lambdaNm = 600, radiusM = 1.0, order = 4, n = 1.0;
    double sigmaLambdaNm = 0.0, sigmaRadiusM = 0.0, sigmaN = 0.0;   // independent 1σ uncertainties
    bool transmitted = false;
};
/// `fringes` selects the spacing/count view; `piezo` selects the movable mirror's actuator (otherwise a micrometer).
/// The classroom stage models mirror travel from −2 to +2 µm; moving it changes round-trip OPD by 2nΔx.
struct MichelsonInput { double lambdaNm = 632.8, displacementUm = 0.75, n = 1.0, visibility = 1.0; bool fringes = false, piezo = false; };
/// `transmission` picks the second mode tab: reflected (false) or transmitted (true) light.
/// `colours` is the third tab: every wavelength at once - the perceived colour of the film.
/// `n2` is the substrate index; phase reversal depends on n₀ < n vs n vs n₂ relations.
/// `thetaDeg` is the angle of incidence from normal; OPD becomes 2nt·cos(θₜ) via Snell's law.
struct ThinFilmInput  { double lambdaNm = 600, n = 1.5, thicknessNm = 100, n2 = 1.5, thetaDeg = 0.0; bool transmission = false; bool colours = false; };

/// One entry of the colour tab's readout: a spectral line, its local reflectance and its swatch.
struct FilmColourSample {
    double wavelengthNm = 0;
    double reflectance = 0;             // 0..1 at this wavelength
    std::string name;                   // "Blue 450 nm"
};

/// The perceived colour of a film for a given thickness (reflectance mode).
struct FilmColour {
    std::string hex;                    // "#...", the swatch to paint
    double hueNm = 0;                   // wavelength of the dominant reflected colour
    double meanReflectance = 0;         // 0..1 over 400..700 nm
    std::vector<FilmColourSample> lines;// 400/450/.../700 nm, for the readout
};

/// Reflectance of the film at one wavelength: one phase reversal, equal amplitudes.
double thinFilmReflectance(double thicknessNm, double n, double wavelengthNm);
/// Reflectance with substrate index n2 and angle of incidence thetaDeg.
double thinFilmReflectance(double thicknessNm, double n0, double n, double n2, double wavelengthNm, double thetaDeg);
/// Visibility of the interference colours: 1 when the order difference across the visible range is large
/// (thin films), 0 when many orders overlap and the colours wash out to grey.
double thinFilmColourVisibility(double thicknessNm, double n);
/// The film colour; `visibility = 0` returns the neutral grey of a washed-out film.
FilmColour thinFilmColour(double thicknessNm, double n);
FilmColour thinFilmColour(double thicknessNm, double n, double n2);
/// `orderList` is the second tab: every propagating order with its angle instead of the intensity fan.
/// `lambda2Nm` is the second wavelength for spectral separation (item 12); when > 0, both λ₁ and λ₂ are shown.
struct GratingInput   { double linesPerCm = 5000, lambdaNm = 600, lambda2Nm = 0.0, order = 1, slits = 20, incidenceDeg = 0; bool orderList = false; };
/// `geometry` is the second tab: where the minima land on the screen instead of the intensity envelope.
/// `envelope` is the third tab: this slit multiplied by a double-slit pattern (finite slit separation).
struct SingleSlitInput{ double widthMm = 0.1, lambdaNm = 500, screenM = 1.0; bool geometry = false; bool envelope = false; double separationMm = 0.4; };
/// `slitWidthMm > 0` adds the single-slit envelope  sinc²(π a sin θ / λ)  to the N-slit interference pattern.
struct GratingResolutionInput { double linesPerCm = 5000, lambdaNm = 600, order = 1, slits = 120; double slitWidthMm = 0.0; };
/// `brewster` is the second tab: the interface at θB with the Fresnel reflectances instead of Malus' law.
struct PolarizationInput { double incidentIntensity = 100, angleDeg = 60, n1 = 1.0, n2 = 1.5; bool unpolarized = true; bool brewster = false; };

namespace WaveExperiments {

ModelReport newtonRings(const NewtonInput& in, const ReportContext& ctx = {});
ModelReport michelson(const MichelsonInput& in, const ReportContext& ctx = {});
ModelReport thinFilm(const ThinFilmInput& in, const ReportContext& ctx = {});
ModelReport grating(const GratingInput& in, const ReportContext& ctx = {});
ModelReport singleSlit(const SingleSlitInput& in, const ReportContext& ctx = {});
ModelReport polarization(const PolarizationInput& in, const ReportContext& ctx = {});
/// Third tab of the single slit: two slits of finite width - the double-slit fringes inside the slit envelope.
ModelReport doubleSlitEnvelope(const SingleSlitInput& in, const ReportContext& ctx = {});
/// Third tab of the grating: resolving power R = mN, the Rayleigh separation and the free spectral range.
ModelReport gratingResolution(const GratingResolutionInput& in, const ReportContext& ctx = {});

/// Orders m with a real angle: |m λ/d + sin θi| <= 1, in increasing order.
std::vector<int> allowedGratingOrders(double linesPerCm, double lambdaNm, double incidenceDeg);
/// I/Imax of an ideal N-slit grating without envelope at angle θ (degrees).
double gratingIntensity(double thetaDeg, double linesPerCm, double lambdaNm, double slits, double incidenceDeg = 0);
/// (sin β / β)², β = π a sin θ / λ; 1 at β = 0
double singleSlitIntensity(double thetaDeg, double widthMm, double lambdaNm);
/// sin²Nx / sin²x of any periodic phasor pattern with N sources, x = phase step / 2 (1 at every principal maximum).
double multiSlitFactor(double phaseStepRad, double sources);
/// Combined two-slit intensity: slit envelope  sinc²  ×  fringe factor  cos², both from the screen position.
double doubleSlitEnvelopeIntensity(double thetaDeg, double widthMm, double separationMm, double lambdaNm);

} // namespace WaveExperiments
} // namespace phy104
