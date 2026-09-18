#ifndef FixCheck_FixCheck_h
#define FixCheck_FixCheck_h
#include <AnalyzerFramework/AnalyzerCore.h>
#include <cstddef>
#include <map>
class FixCheck final : public AnalyzerCore {
public:
    void initializeAnalyzer() override;
    void executeEvent() override;
    void WriteHist() override;
private:
    struct A { double s = 0, s2 = 0, mn = 1e300, mx = -1e300; std::size_t n = 0; };
    void add(A &a, double v);
    void rep(const char *l, const A &a) const;
    std::size_t nEv_ = 0, nJets_ = 0, nJesZero_ = 0, nJesDataMismatch_ = 0;
    A jesUp_, jesDown_;
    std::map<int, std::size_t> nLHE_;
    std::size_t nLHEev_ = 0, nMuFUpIsOne_ = 0, nMuFUpNotSlot5_ = 0, nSlot4NotOne_ = 0, nOldMuFUpIsOne_ = 0;
    A muFUp_, muFDown_, muRUp_, muRDown_;
};
#endif
