#pragma once
#include <iostream>
namespace valor {
struct logline {
    enum class level {
        LDEBUG,
        LINFO,
        LWARN,
        LCRIT,
    };
    static constexpr std::pair<std::string_view,std::string_view> name_and_tag[] = {
        {"DEBUG", "DEBG"},
        {"INFO", "INFO"},
        {"WARN", "WARN"},
        {"CRIT", "CRIT"},
    };
    static level loglevel;
    static bool need(level l) { return l >= loglevel; }
    static auto lvl(level l) { return name_and_tag[int(l)].second; }
    std::ostream& o;
    logline& operator<<(auto const& t) { o << t; return *this; }
    logline(std::ostream& o, level l = level::LINFO) : o(o) { o << '[' << lvl(l) << "] "; }
    ~logline() { o << '\n'; }
    operator bool() const noexcept { return true; }
    struct force {
        level const save;
        force(level l) : save(loglevel) { loglevel = l; }
        ~force() { loglevel = save; }
    };
};
inline logline::level parse_lvl() {
    auto s = [] {
        auto c = std::getenv("VALET_LOGLEVEL");
        std::string z = c && c[0] ? c : "";
        for (auto& c: z)
            c = std::toupper(c);
        return z;
    }();
    for (int i = 0; i < 4; ++i)
        if (s.find(logline::name_and_tag[i].first) == 0)
            return logline::level(i);
    return logline::level::LWARN;
}
}
#define LOG_IN_LEVEL(l) valor::logline::need((l)) && valor::logline(std::clog, (l))
#define INFO LOG_IN_LEVEL(valor::logline::level::LINFO)
#define LOG INFO
#define WARN LOG_IN_LEVEL(valor::logline::level::LWARN)
#define CRIT LOG_IN_LEVEL(valor::logline::level::LCRIT)
#define DEBUG LOG_IN_LEVEL(valor::logline::level::LDEBUG)
