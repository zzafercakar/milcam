// tests/core/check.h
#pragma once
#include <cstdio>
#include <string>
#include <vector>
#include <functional>

namespace t {
struct Case { std::string name; std::function<void()> fn; };
inline std::vector<Case>& cases() { static std::vector<Case> c; return c; }
inline int& fails() { static int f = 0; return f; }
struct Reg { Reg(const std::string& n, std::function<void()> f){ cases().push_back({n,f}); } };

inline void eq(const std::string& got, const std::string& want, const char* where) {
    if (got != want) {
        ++fails();
        std::printf("FAIL %s\n--- got ----\n%s\n--- want ---\n%s\n------------\n",
                    where, got.c_str(), want.c_str());
    }
}
}  // namespace t

#define TEST(name) \
    static void name(); \
    static t::Reg reg_##name(#name, name); \
    static void name()
#define MILCAM_STR2(x) #x
#define MILCAM_STR(x)  MILCAM_STR2(x)
#define CHECK_EQ(got, want) t::eq((got), (want), __FILE__ ":" MILCAM_STR(__LINE__))
