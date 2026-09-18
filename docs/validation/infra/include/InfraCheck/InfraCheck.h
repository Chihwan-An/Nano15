#ifndef InfraCheck_h
#define InfraCheck_h
#include <AnalyzerFramework/AnalyzerCore.h>
#include <cstddef>
#include <fstream>
#include <map>
#include <memory>
#include <string>
class InfraCheck final : public AnalyzerCore {
public:
    void initializeAnalyzer() override;
    void executeEvent() override;
    void WriteHist() override;
private:
    std::size_t nEv_ = 0;
    std::map<std::string, std::size_t> cnt_;
    std::map<std::string, std::pair<double, double>> sum_; // sum, sumsq
    std::map<std::string, std::size_t> num_;
    std::shared_ptr<std::ofstream> dump_;
    void acc(const std::string &k, double v);
    double mean(const std::string &k) const;
    void rep(const std::string &k) const;
    std::size_t c(const std::string &k) const { auto it = cnt_.find(k); return it == cnt_.end() ? 0 : it->second; }
};
#endif
