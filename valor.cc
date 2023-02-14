#include <valor.h>
#include <iostream>
#include <chrono>
#include <charconv>

using namespace valor;
using namespace std::chrono_literals;
// using namespace std::string_literals;

struct remote_op {//TODO op co_await
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept { std::cerr << " remote::asusp " << this << " on handle " << h.address() << '\n'; }
    void await_resume() const noexcept { std::cerr << " remote::aresume " << this << '\n'; }
};

void nontriv_work(int& i) { i += size_t(&i); }

// ser_ctx<> f_orig(std::string_view = {}/*!! need to add manually*/) {
//     std::cerr << "f: started\n";
//     std::cerr << "f: 1st coawait\n";
//     co_await remote_op();
//     std::cerr << "f: 2nd coawait\n";
//     co_await remote_op();
//     std::cerr << "f: done\n";
// }

// ser_ctx<> f(std::string_view = {}/*appended by V*/) {
// auto p = co_await ser_ctx<>::get_promise{};
// if (auto st = p->stage()) {
//     std::cerr << "f: to stage " << st << '\n';
//     static ssize_t stages[] = {0, (char*)&&s1 - (char*)&&s0, (char*)&&s2 - (char*)&&s0};
//     goto *(void*)((char*)&&s0 + stages[st]);
// }
//     std::cerr << "f: started\n";
// p->save(0);// useless, just for symmetry
//     // nothing in between
// s0: // not jumped to
//     std::cerr << "f: 1st coawait\n";
// p->save(1);
//     co_await remote_op();
// s1:
//     std::cerr << "f: 2nd coawait\n";
// p->save(2);
//     co_await remote_op();
// s2:
//     std::cerr << "f: done\n";
// }

// ser_ctx<> g_orig(std::string_view = {}) {
//     int i = 13;
//     std::cerr << "g: started\n";
//     nontriv_work(i);
//     std::cerr << "g: 1st coawait with i=" << i << '\n';
//     co_await remote_op();
//     int j = 17;
//     nontriv_work(j);
//     std::cerr << "g: 2nd coawait j=" << j << '\n';
//     co_await remote_op();
//     std::cerr << "g: done i=" << i << " j=" << j << '\n';
// }

// ser_ctx<> g(std::string_view = {}) {
// auto p = co_await ser_ctx<>::get_promise{};
// if (auto st = p->stage()) {
//     std::cerr << "g: to stage " << st << '\n';
//     static ssize_t stages[] = {0, (char*)&&s1 - (char*)&&s0, (char*)&&s2 - (char*)&&s0};
//     // assert(st < stages.size);
//     goto *(void*)((char*)&&s0 + stages[st]);
// }
//     int i = 13;
//     std::cerr << "g: started\n";
// s0:
//     nontriv_work(i);
//     std::cerr << "g: 1st coawait with i=" << i << '\n';
// p->save(1, i);//TODO what if throws? Save in p?
//     co_await remote_op();
// s1:
// p->load(i);//TODO args first
//     int j = 17;
//     nontriv_work(j);
//     std::cerr << "g: 2nd coawait j=" << j << '\n';
// p->save(2, i, j);
//     co_await remote_op();
// s2:
// p->load(i, j);
//     std::cerr << "g: done i=" << i << " j=" << j << '\n';
// }

// ser_ctx<>/*TODO any castable to*/ h_orig(int in, std::string_view = {}) {
//     std::cerr << "h: started with in=" << in << '\n';
//     nontriv_work(in);
//     std::cerr << "h: 1st coawait with in=" << in << '\n';
//     co_await remote_op();
//     int j = 17;
//     nontriv_work(j);
//     std::cerr << "h: 2nd coawait j=" << j << '\n';
//     co_await remote_op();
//     std::cerr << "h: done in=" << in << " j=" << j << '\n';
// }

// ser_ctx<> h(int in, std::string_view/*injected*/) {
// auto p = co_await ser_ctx<>::get_promise{};
// if (auto st = p->stage()) {
//     std::cerr << "h: to stage " << st << '\n';
//     static ssize_t stages[] = {0, (char*)&&s1 - (char*)&&s0, (char*)&&s2 - (char*)&&s0};
//     // assert(st < stages.size);
//     goto *(void*)((char*)&&s0 + stages[st]);
// }
// s0:
//     std::cerr << "h: started with in=" << in << '\n';
//     nontriv_work(in);
//     std::cerr << "h: 1st coawait with in=" << in << '\n';
// p->save(1, in);//TODO what if throws? Save in p?
//     co_await remote_op();
// s1:
// p->load(in);
//     int j = 17;//TODO [[attr] to skip saving
//     nontriv_work(j);
//     std::cerr << "h: 2nd coawait j=" << j << '\n';
// p->save(2, in, j);
//     co_await remote_op();
// s2:
// p->load(in, j);
//     std::cerr << "h: done in=" << in << " j=" << j << '\n';
// }
// /*autogen -- what if already exists?*/auto h(int i) { return h(i, {}); }
// /*autogen*/auto h(std::string_view s) { return h({}, s); }

ser_ctx<int> k_orig(int in) {
    std::cerr << "k: started with in=" << in << '\n';
    nontriv_work(in);
    std::cerr << "k: 1st coawait with in=" << in << '\n';
    co_await remote_op();
    int j = 17;
    nontriv_work(j);
    std::cerr << "k: 2nd coawait j=" << j << '\n';
    co_await remote_op();
    std::cerr << "k: done in=" << in << " j=" << j << '\n';
    co_return 14;
}

[[valor_skip]]
ser_ctx<int> k(int in) {
auto p = co_await ser_ctx<int>::get_promise{};
if (auto st = p->stage()) {
    std::cerr << "k: to stage " << st << '\n';
    static ssize_t stages[] = {0, (char*)&&s1 - (char*)&&s0, (char*)&&s2 - (char*)&&s0};
    // assert(st < stages.size);
    goto *(void*)((char*)&&s0 + stages[st]);
}
s0:
    std::cerr << "k: started with in=" << in << '\n';
    nontriv_work(in);
    std::cerr << "k: 1st coawait with in=" << in << '\n';
p->save(1, in);
    co_await remote_op();
s1:
p->load(in);
    int j = 17;//TODO [attr] to skip saving
    nontriv_work(j);
    std::cerr << "k: 2nd coawait j=" << j << '\n';
p->save(2, in, j);
    co_await remote_op();
s2:
p->load(in, j);
    std::cerr << "k: done in=" << in << " j=" << j << '\n';
    co_return 14;
}

auto start(ser_ctx<auto>&& c) {
    c.owner = true;
    c();
    return std::move(c);
}

template<typename>
struct args;

template<typename R, typename... Args>
struct args<R(*)(Args...)> {
    using a = std::tuple<Args...>;
};

auto continue_from(std::string_view ser, auto fun) {
    auto c = std::apply(fun, typename args<decltype(fun)>::a{});
    c.deserialize(ser);
    return start(std::move(c));
}

void saving() {
    std::cerr << "start fun\n";
    auto coro = start(k(45));
    auto s = coro.serialize();
    std::cerr << "saving coro state: \"" << (s.size() ? uf::print_escaped(s) : "") << "\"\n";
    std::cout << s;
}

void restoring(std::string_view ser) {
    std::cerr << "restore from: \"" << (ser.size() ? uf::print_escaped(ser) : "") << "\"\n";
    auto coro = continue_from(ser, k);
    while (coro) {
        std::cerr << "continue fun\n";
        coro();
    }
    std::cerr << "retval: " << coro.get() << '\n';
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
