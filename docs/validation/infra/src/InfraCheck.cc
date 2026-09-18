#include <InfraCheck/InfraCheck.h>
#include "Acc.h"
void InfraCheck::acc(const std::string &k, double v) { auto &p = sum_[k]; p.first += v; p.second += v * v; ++num_[k]; }
double InfraCheck::mean(const std::string &k) const { auto it = sum_.find(k); if (it == sum_.end()) return 0; return it->second.first / num_.at(k); }
void InfraCheck::rep(const std::string &) const {}
namespace { float gX = 0.f; int gI = 0; }
void InfraCheck::initializeAnalyzer() {
    // One RNTuple under a directory path (filled every event) and one that is
    // booked but never filled: the two shapes that used to break hadd.
    auto t = Output().Book("Central/SR_Tree");
    t.Field("x", gX).Field("iev", gI);
    auto e = Output().Book("Central/Empty_Tree");
    e.Field("x", gX);
}
void InfraCheck::executeEvent() {
    ++nEv_;
    gX = static_cast<float>(nEv_) * 0.5f;
    gI = static_cast<int>(nEv_);
    Output().Get("Central/SR_Tree").Fill();
    FillHist("Central/nEvents", 0.5f, 1.f, 1, 0.f, 1.f);
}
void InfraCheck::WriteHist() {
    std::cout << "\n===== InfraCheck events " << nEv_ << "\n=====\n";
    AnalyzerCore::WriteHist();
}
