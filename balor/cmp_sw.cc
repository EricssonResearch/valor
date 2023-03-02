// Measure task switch overhead with Boost Fibers vs coroutines
#include <boost/fiber/all.hpp>
#include <coroutine>
#include <iostream>

void fibers(int n) try {
    boost::fibers::fiber
        f1([&] {
            while (n >= 0)
                boost::this_fiber::yield();
        }),
        f2([&] {
            while (n--)
                boost::this_fiber::yield();
        });
    f1.join(), f2.join();
} catch (std::exception const& e) {
    std::cerr << "exception: " << e.what() << '\n';
} catch (...) {
    std::cerr << "unhandled exception\n";
}

struct task {
    struct promise_type {
        auto get_return_object() noexcept { return task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        struct yielder {
            bool await_ready() noexcept { return false; }
            auto await_suspend(std::coroutine_handle<promise_type> h) noexcept { return std::exchange(h.promise().then, {}); }
            void await_resume() noexcept {}
        };
        yielder yield_value(char) noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() noexcept { std::terminate(); }
        std::coroutine_handle<> then;
    };
    task(std::coroutine_handle<promise_type> coro) noexcept : coro(coro) {}
    task(task&& t) noexcept : coro(std::exchange(t.coro, {})) {}
    ~task() { if (coro) coro.destroy(); }
    bool await_ready() { return false; }
    auto await_suspend(std::coroutine_handle<> h) noexcept { coro.promise().then = h; return coro; }
    auto await_resume() {}
    std::coroutine_handle<promise_type> coro;
};

task counter(task& t) {
    while (!t.coro.done())
        co_await t;
}

// Causes circular calling of await_suspend() -> .then's aw_resume() -> repeat
// until stack is exhausted.
task timer(int& n) {
    while (n--)
        co_yield 0;
}

void coro(int n) {
    auto f2 = timer(n), f1 = counter(f2);
    f1.coro.resume();
}

// So we cut this by returning a suspended awaiter
// then calling coro.resume() repeatedly from the "scheduler".
task timer2(int& n) {
    while (n--)
        co_await std::suspend_always{};
}

void coro2(int n) {
    auto f2 = timer2(n), f1 = counter(f2);
    while (!f1.coro.done())
        f1.coro.resume();
}

int main(int, char** a) {
    (a[1][0] == 'f' ? fibers
    : a[1][0] == 'c' ? coro
    : a[1][0] == 'C' ? coro2
    : exit)(std::atoi(a[2]));
}
