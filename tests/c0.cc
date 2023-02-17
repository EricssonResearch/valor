#include "valet.h"
#include <gtest/gtest.h>
#include <clang/Tooling/Tooling.h>
#include <unistd.h>

using namespace std::string_literals;

std::pair<bool, std::string> run(std::string_view code) {
    using namespace clang::tooling;
    CommandLineArguments args;
    if (auto c = std::getenv("srcdir"); c && c[0])
        args.emplace_back("-I"s + c + "/..");
    std::ostringstream out;
    valor::valet v; // Need to create a new one for each test -- there is some state in one of the AST classes that will *@#! up things if re-run
    v.out = &out;
    return {runToolOnCodeWithArgs(newFrontendActionFactory(&v, &v)->create(), code, v.cc_opts(args)), out.str()/*need a copy as per notes*/};
}

auto expect(std::string_view code, std::string_view expected) {
    auto [ok, out] = run(code);
    EXPECT_TRUE(ok);
    EXPECT_EQ(out, expected);
}

TEST(runner, empty_input) {
    EXPECT_TRUE(run("").first);
}

TEST(runner, non_coro) {
    EXPECT_TRUE(run("int f() { return 3; }").first);
}

auto pre =
R"(#include <valor.h>
using namespace valor;
struct remote_op {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    void await_resume() const noexcept {}
};
void nontriv_work(int& i) { i += size_t(&i); }
)"s;

TEST(valet, no_locals) {
    expect(
        pre +
R"(ser_ctx<> f() { co_await remote_op(); }
)",
        pre +
R"..(ser_ctx<> f() {
auto _valet_p = co_await valor::ser_ctx<>::get_promise{};
if (auto _valet_st = _valet_p->stage()) {
    assert(_valet_st <= 1);
    // std::cerr << "k: to stage " << _valet_st << '\n';
    static ssize_t _valet_stages[] = {0, (char*)&&_valet_s1 - (char*)&&_valet_s0};
    goto *(void*)((char*)&&_valet_s0 + _valet_stages[_valet_st]);
}
_valet_s0:
#line 9
 
_valet_p->save(1);
#line 9
co_await remote_op();
_valet_s1:
_valet_p->load();
#line 9
 }
)..");
}

TEST(valet, local_vars) {
    expect(
        pre +
R"(ser_ctx<int> f(int in) {
    nontriv_work(in);
    {
        int x = 21;
        nontriv_work(x);
    }
    co_await remote_op();int j = 17;
    nontriv_work(j);
    co_await remote_op();
    co_return 14;
}
)",
        pre +
R"(ser_ctx<int> f(int in) {
auto _valet_p = co_await valor::ser_ctx<int>::get_promise{};
if (auto _valet_st = _valet_p->stage()) {
    assert(_valet_st <= 2);
    // std::cerr << "k: to stage " << _valet_st << '\n';
    static ssize_t _valet_stages[] = {0, (char*)&&_valet_s1 - (char*)&&_valet_s0, (char*)&&_valet_s2 - (char*)&&_valet_s0};
    goto *(void*)((char*)&&_valet_s0 + _valet_stages[_valet_st]);
}
_valet_s0:
#line 9

    nontriv_work(in);
    {
        int x = 21;
        nontriv_work(x);
    }
    
_valet_p->save(1, in);
#line 15
co_await remote_op();
_valet_s1:
_valet_p->load(in);
#line 15
int j = 17;
    nontriv_work(j);
    
_valet_p->save(2, in, j);
#line 17
co_await remote_op();
_valet_s2:
_valet_p->load(in, j);
#line 17

    co_return 14;
}
)");
}