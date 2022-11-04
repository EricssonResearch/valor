#include "uctx.h"
#include <iostream>
#include <coroutine>
#include <chrono>
#include <charconv>

using namespace std::chrono_literals;
// using namespace std::string_literals;

// TODO coro_traits for a ufserializable

struct ser_ctx {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type const händel;
    ser_ctx(handle_type h) : händel(h) { std::cerr << " handle " << händel.address() << " with promise " << &händel.promise() << '\n'; }
    ~ser_ctx() { if (händel) händel.destroy(); }
    void resume() { std::cerr << " h resume\n"; händel.resume(); }
    auto serialize() const { return std::to_string(händel.promise().diff); }
    struct promise_type {
        intptr_t diff;
        promise_type(ssize_t d) : diff(d) { std::cerr << " promise " << this << " with diff " << diff << '\n'; }
        auto get_return_object() noexcept { return ser_ctx{handle_type::from_promise(*this)}; }
        std::suspend_never initial_suspend() const noexcept { std::cerr << " promise init susp has diff " << diff << '\n'; return {}; }
        std::suspend_always final_suspend() const noexcept { std::cerr << " final susp\n"; return {}; }
        void return_void() {}
        void unhandled_exception() const noexcept {}
    };
};

ser_ctx f(ssize_t);

struct remote_op {//TODO op co_await
    bool await_ready() const noexcept { return false; }
    void await_suspend(ser_ctx::handle_type h) const noexcept {
        std::cerr << " asusp " << h.address() << " at " << ctx::rip(2) << " in " << ctx::addr((void*)f) << '\n';
        std::cerr << "  diff " << (h.promise().diff = ctx::rip_diff_to((void*)f, 2)) << '\n';
    }
    void await_resume() const noexcept { std::cerr << " aresume " /*<< h.address()*/ << '\n'; }
};

ser_ctx f(ssize_t diff = 0) {
    if (diff) {
        std::cerr << "f: jump " << (void*)f << " + " << diff << std::endl;
        asm volatile /*goto*/ (
            "lea (%%rip), %%rsi;"
            "lea 7(%0, %%rsi), %%rsi;"
            "jmp *%%rsi"
            :
            : "r" (uint64_t(diff))
            : "rsi"
        );
        __builtin_unreachable();
    }
    std::cerr << "f: started\n";
    std::cerr << "f: 1st coawait\n";
    co_await remote_op();
    // should be restarted from here
    std::cerr << "f: 2nd coawait\n";
    co_await remote_op();
}

void saving() {
    std::cerr << "start f\n";
    auto coro = f();// start and advance to the first stage
    auto s = coro.serialize();
    std::cerr << "saving coro state: " << s << '\n';
    std::cout << s;
}

void restoring(std::string_view ser) {
    std::cerr << "restore from: " << ser << '\n';
    int diff = 0;
    if (std::from_chars(ser.begin(), ser.end(), diff).ec != std::errc{})
        throw std::invalid_argument("not a diff: \"" + std::string(ser) + '"');
    auto coro = f(diff);
    std::cerr << "resume f\n";
    coro.resume();
}

int main(int n, char** a) {
    std::cerr << "addr of f is " << (void*)f << '\n';
    if (a[1][0] == 's')
        saving();
    if (a[1][0] == 'r') {
        std::ostringstream s;
        s << std::cin.rdbuf();
        restoring(s.str());
    }
}
