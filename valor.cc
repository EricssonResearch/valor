#include <valor.h>
#include <iostream>

using namespace valor;

#define LOG std::cerr

void nontriv_work(auto& i) { i += size_t(&i); }

template<typename T = int>
struct remote_op {
    T i;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept { LOG << " remote::asusp " << this << " on handle " << h.address() << '\n'; }
    T await_resume() noexcept { LOG << " remote::aresume " << this << '\n'; nontriv_work(i); return i; }
};

template<>
struct remote_op<void> {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept { LOG << " remote<void>::asusp " << this << " on handle " << h.address() << '\n'; }
    void await_resume() noexcept { LOG << " remote<void>::aresume " << this << '\n'; }
};

// TODO prohibit op coa to not meddle with our awaiters

// ser_ctx<int> k_(int in) {
//     LOG << "k: started with in=" << in << '\n';
//     nontriv_work(in);
//     LOG << "k: 1st coawait with in=" << in << '\n';
//     auto j = co_await remote_op{in} + co_await remote_op{13};
//     LOG << "k: after coawait j=" << j << '\n';
//     nontriv_work(j);
//     LOG << "k: 2nd coawait j=" << j << '\n';
//     co_await remote_op<void>();
//     LOG << "k: done in=" << in << " j=" << j << '\n';
//     co_return 14;
// }

[[valor_skip]]
ser_ctx<int> k(int in) {
auto p = co_await ser_ctx<int>::get_promise{};
if (auto st = p->stage()) {
    LOG << "k: to stage " << st << '\n';
    static ssize_t stages[] = {0, (char*)&&s1 - (char*)&&s0, (char*)&&s2 - (char*)&&s0};
    // assert(st < stages.size);
    goto *(void*)((char*)&&s0 + stages[st]);
}
s0:
    LOG << "k: started with in=" << in << '\n';
    nontriv_work(in);
    LOG << "k: 1st coawait with in=" << in << '\n';
p->save(1, in);
p->aw.resize(1);
s1:
p->load(in);
    auto j = // NB: does not work for j referencing non-local var
(p->aw.has(0) ? p->aw.get_as<decltype(remote_op{in})>(0) :
    co_await remote_op{in}
)
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97452 plus descending into the ternary
//     +
// (p->aw.has(1) ? p->aw.get_as<decltype(remote_op{13})>(1) :
//     co_await remote_op{13}
// )
    ;
    LOG << "k: after coawait j=" << j << '\n';
    nontriv_work(j);
    LOG << "k: 2nd coawait j=" << j << '\n';
p->save(2, in, j);
p->aw.resize(1);
s2:
p->load(in, j);
(p->aw.has(0) ? p->aw.get_as<decltype(remote_op<void>())>(0) :
    co_await remote_op<void>()
)
    ;
    LOG << "k: done in=" << in << " j=" << j << '\n';
    co_return 14;
}

void saving() {
    LOG << "\n1st process start fun\n";
    auto coro = start(k(45));
    auto aw = coro.awaiter(0); //TODO Who supplies the type for any_cast?
    auto s = coro.serialize();
    LOG << "saving coro state " << (s.size() ? uf::any_view(uf::from_raw, s).print() : "") << '\n';
    std::cout << s;
}

void restoring(std::string_view ser) {
    LOG << "\n2nd process restore from " << (ser.size() ? uf::any_view(uf::from_raw, ser).print() : "") << '\n';
    // NB: its a *must* to supply aw_ret otherwise the load / co_a logic gets garlbed
    auto coro = continue_from(ser, k, 29/*use whatever, eg. std::nullopt_t for void*/);//TODO from where? verify its the same type as remote_op aw_res? (otherwise bad_cast will happen in the coro -- maybe we can live with that)
    while (coro) {
        LOG << "continue fun\n";
        coro();
    }
    LOG << "retval: " << coro.get() << '\n';
}

int main(int, char** a) {
    if (a[1][0] == 's')
        saving();
    if (a[1][0] == 'r') {
        std::ostringstream s;
        s << std::cin.rdbuf();
        restoring(s.str());
    }
}
