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
    ~ser_ctx() { if (händel) händel.destroy(); }//TODO ? make into a fun? otherwise the rest of 5
    auto serialize() const { return std::to_string(händel.promise().stage); }
    void operator()() const { std::cerr << " h resume\n"; händel(); }
    operator bool() const { return händel && !händel.done(); }
    struct promise_type {
        uint8_t stage;
        promise_type(uint8_t d) : stage(d) { std::cerr << " promise " << this << " with stage " << int(stage) << '\n'; }
        // ~promise_type() { std::cerr << " ~promise " << this << '\n'; }
        auto get_return_object() noexcept { return ser_ctx{handle_type::from_promise(*this)}; }
        std::suspend_never initial_suspend() const noexcept { std::cerr << " init susp\n"; return {}; }
        std::suspend_always final_suspend() const noexcept { std::cerr << " final susp\n"; return {}; } // if not susp then autodestroys the handle obj -- but then problem at last coro() in restore()?
        void return_void() {}
        void unhandled_exception() const noexcept {}
    };
    struct get_promise {
      promise_type* p;
      bool await_ready() const noexcept { return false; }
      bool await_suspend(handle_type h) noexcept { p = &h.promise(); return false; }
      auto await_resume() const noexcept { return p; }
    };
};

struct remote_op {//TODO op co_await
    bool await_ready() const noexcept { return false; }
    void await_suspend(ser_ctx::handle_type h) const noexcept { std::cerr << " asusp " << h.address() << '\n'; }
    void await_resume() const noexcept { std::cerr << " aresume\n"; }
};

ser_ctx f(uint8_t = 0) {
    auto p = co_await ser_ctx::get_promise{};
    if (p->stage) {
        std::cerr << "f: to stage " << int(p->stage) << '\n';
        static ssize_t stages[] = {0, (char*)&&s1 - (char*)&&s0, (char*)&&s2 - (char*)&&s0};
        // assert(stage < stages.size);
        goto *(void*)((char*)&&s0 + stages[p->stage]);
    }
    std::cerr << "f: started\n";
    // p->stage = 0; // useless, just for symmetry / macro
s0: // not jumped to
    std::cerr << "f: 1st coawait\n";
    p->stage = 1;
    co_await remote_op();
s1:
    std::cerr << "f: 2nd coawait\n";
    p->stage = 2;
    co_await remote_op();
s2:;
    std::cerr << "f: done\n";
}

void saving() {
    std::cerr << "start f\n";
    auto coro = f();// start and advance to the first stage
    auto s = coro.serialize();
    std::cerr << "saving coro state: \"" << s << "\"\n";
    std::cout << s;
}

void restoring(std::string_view ser) {
    std::cerr << "restore from: \"" << ser << "\"\n";
    int stage = 0;
    if (std::from_chars(ser.begin(), ser.end(), stage).ec != std::errc{})
        throw std::invalid_argument("not a stage: \"" + std::string(ser) + '"');
    std::cerr << "resume f from stage " << stage << '\n';
    auto coro = f(stage);
    while (coro) {
        std::cerr << "and continuing f\n";
        coro();
    }
}

int main(int n, char** a) {
    // std::cerr << "addr of f is " << (void*)f << '\n';
    if (a[1][0] == 's')
        saving();
    if (a[1][0] == 'r') {
        std::ostringstream s;
        s << std::cin.rdbuf();
        restoring(s.str());
    }
}
