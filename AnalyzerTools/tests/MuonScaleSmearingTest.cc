#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <memory>
#include <set>
#include <string>

#include "correction.h"

#include "MyCorrection.h"
#include "MuonScaRe.h"

namespace {

// ---------------------------------------------------------------------------
// Crystal Ball: pure maths, no correction files needed.
// ---------------------------------------------------------------------------
TEST(MuonCrystalBall, InverseCdfRoundTrips) {
    // Typical values of the POG fit: core near zero, tails starting ~1.5 sigma.
    const SKNano::MuonCrystalBall cb(0.02, 1.1, 1.6, 3.5);
    for (const double u : {0.001, 0.01, 0.1, 0.3, 0.5, 0.7, 0.9, 0.99, 0.999}) {
        const double x = cb.invcdf(u);
        ASSERT_TRUE(std::isfinite(x)) << "u=" << u;
        EXPECT_NEAR(cb.cdf(x), u, 1e-6) << "u=" << u;
    }
}

TEST(MuonCrystalBall, InverseCdfIsMonotonic) {
    const SKNano::MuonCrystalBall cb(0., 1., 1.2, 2.5);
    double previous = cb.invcdf(1e-4);
    for (double u = 1e-3; u < 1.; u += 1e-3) {
        const double x = cb.invcdf(u);
        EXPECT_GT(x, previous) << "u=" << u;
        previous = x;
    }
    // The core is symmetric around the mean.
    EXPECT_NEAR(cb.invcdf(0.5), 0., 1e-9);
}

TEST(MuonCrystalBall, SanityGuardRejectsArtefacts) {
    EXPECT_TRUE(SKNano::MuonScaReResultIsSane(50.5, 50.));
    EXPECT_FALSE(SKNano::MuonScaReResultIsSane(std::nan(""), 50.));
    EXPECT_FALSE(SKNano::MuonScaReResultIsSane(-1., 50.));
    EXPECT_FALSE(SKNano::MuonScaReResultIsSane(101., 50.));
    EXPECT_FALSE(SKNano::MuonScaReResultIsSane(4., 50.));
}

// ---------------------------------------------------------------------------
// Against the 2024 files. Built once per suite; skips when the correction
// environment is not reachable, as TauCorrectionTest does.
// ---------------------------------------------------------------------------
class MuonScaleSmearing : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!std::getenv("JSONPOG_REPO_PATH") || !std::getenv("SKNANO_DATA") ||
            !std::getenv("ROCCOR_PATH")) {
            skip_ = "correction environment not set; source setup.sh first";
            return;
        }
        try {
            mc_ = std::make_unique<MyCorrection>("2024", "CDE", "DYto2Mu", false);
            data_ = std::make_unique<MyCorrection>("2024", "CDE", "Muon", true);
            if (!mc_->HasMuonScaleSmearing()) {
                skip_ = "muon_scalesmearing is not configured for 2024";
                mc_.reset();
                data_.reset();
            }
        } catch (const std::exception &e) {
            skip_ = std::string("corrections unavailable: ") + e.what();
            mc_.reset();
            data_.reset();
        }
    }

    static void TearDownTestSuite() {
        mc_.reset();
        data_.reset();
    }

    void SetUp() override {
        if (!mc_)
            GTEST_SKIP() << skip_;
    }

    static std::unique_ptr<MyCorrection> mc_;
    static std::unique_ptr<MyCorrection> data_;
    static std::string skip_;
};

std::unique_ptr<MyCorrection> MuonScaleSmearing::mc_;
std::unique_ptr<MyCorrection> MuonScaleSmearing::data_;
std::string MuonScaleSmearing::skip_;

constexpr float kPt = 52.f, kEta = 0.7f, kPhi = 1.1f;
constexpr int kLayers = 11;

TEST_F(MuonScaleSmearing, DataGetsScaleOnlyAndNoVariation) {
    const auto lanes = data_->GetMuonScaleSmearing(+1, kPt, kEta, kPhi, 0, 1001, 7);
    EXPECT_NEAR(lanes.nominal, kPt, 0.02f * kPt); // a percent-level scale
    EXPECT_NE(lanes.nominal, kPt);                // but not a no-op
    EXPECT_FLOAT_EQ(lanes.scaleUp, lanes.nominal);
    EXPECT_FLOAT_EQ(lanes.scaleDown, lanes.nominal);
    EXPECT_FLOAT_EQ(lanes.resUp, lanes.nominal);
    EXPECT_FLOAT_EQ(lanes.resDown, lanes.nominal);
}

TEST_F(MuonScaleSmearing, DataScaleMatchesThePogFormula) {
    // 1/pt -> m/pt + q*a, read straight from the file.
    const std::string file = std::string(std::getenv("JSONPOG_REPO_PATH")) +
        "/MUO/Run3-24CDEReprocessingFGHIPrompt-Summer24-NanoAODv15/latest/"
        "muon_scalesmearing.json.gz";
    auto cset = correction::CorrectionSet::from_file(file);
    for (const int q : {-1, +1}) {
        const double m = cset->at("m_data")->evaluate({double(kEta), double(kPhi), "nom"});
        const double a = cset->at("a_data")->evaluate({double(kEta), double(kPhi), "nom"});
        const double expected = 1. / (m / kPt + q * a);
        const auto lanes = data_->GetMuonScaleSmearing(q, kPt, kEta, kPhi, 0, 1, 1);
        EXPECT_NEAR(lanes.nominal, expected, 1e-5 * expected) << "q=" << q;
    }
}

TEST_F(MuonScaleSmearing, SimulationVariationsAreCoherentShifts) {
    const auto lanes = mc_->GetMuonScaleSmearing(-1, kPt, kEta, kPhi, kLayers, 424242, 33);
    EXPECT_NEAR(lanes.nominal, kPt, 0.10f * kPt);
    // Scale: symmetric about the nominal by construction.
    EXPECT_GT(lanes.scaleUp, lanes.nominal);
    EXPECT_LT(lanes.scaleDown, lanes.nominal);
    EXPECT_NEAR(lanes.scaleUp + lanes.scaleDown, 2.f * lanes.nominal, 1e-4f * kPt);
    // Resolution: k +- dk with the same draw, so up and down mirror each other
    // around the nominal as well.
    EXPECT_NEAR(lanes.resUp + lanes.resDown, 2.f * lanes.nominal, 1e-4f * kPt);
}

TEST_F(MuonScaleSmearing, SmearingIsDeterministicAndEventDependent) {
    const auto first = mc_->GetMuonScaleSmearing(+1, kPt, kEta, kPhi, kLayers, 99, 5);
    const auto again = mc_->GetMuonScaleSmearing(+1, kPt, kEta, kPhi, kLayers, 99, 5);
    EXPECT_FLOAT_EQ(first.nominal, again.nominal);
    EXPECT_FLOAT_EQ(first.resUp, again.resUp);

    // Different events draw different numbers somewhere in |eta| < 2.4: the
    // residual smearing k may vanish in one eta slice but not in all of them.
    std::size_t mostDistinct = 0;
    for (const float eta : {0.3f, 0.9f, 1.5f, 2.1f}) {
        std::set<float> distinct;
        for (unsigned long long event = 1; event <= 20; ++event)
            distinct.insert(mc_->GetMuonScaleSmearing(+1, kPt, eta, kPhi, kLayers, event, 5).nominal);
        mostDistinct = std::max(mostDistinct, distinct.size());
    }
    EXPECT_GT(mostDistinct, 1u);
}

TEST_F(MuonScaleSmearing, OutsideThePogWindowNothingMoves) {
    for (const float pt : {5.f, 25.f, 210.f, 800.f}) {
        for (MyCorrection *corr : {mc_.get(), data_.get()}) {
            const auto lanes = corr->GetMuonScaleSmearing(+1, pt, kEta, kPhi, kLayers, 7, 7);
            EXPECT_FLOAT_EQ(lanes.nominal, pt);
            EXPECT_FLOAT_EQ(lanes.scaleUp, pt);
            EXPECT_FLOAT_EQ(lanes.scaleDown, pt);
            EXPECT_FLOAT_EQ(lanes.resUp, pt);
            EXPECT_FLOAT_EQ(lanes.resDown, pt);
        }
    }
}

TEST_F(MuonScaleSmearing, EdgeOfAcceptanceDoesNotThrow) {
    // The maps clamp in eta and phi; make sure nothing at the boundary throws.
    for (const float eta : {-2.39f, 2.39f, -2.5f, 2.5f})
        for (const float phi : {-3.1415f, 3.1415f})
            EXPECT_NO_THROW(mc_->GetMuonScaleSmearing(-1, 100.f, eta, phi, 5, 1, 1));
}

} // namespace
