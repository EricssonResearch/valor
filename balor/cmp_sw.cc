// Measure task switch overhead with Boost Fibers vs coroutines
#include <atomic>
#include <boost/fiber/all.hpp>
#include <coroutine>
#include <iostream>
#include <optional>

void fibers(int n) try {
    std::atomic_size_t c1 = 0, c2 = 0;
    boost::fibers::fiber
        f1([&] {
            while (++c1, n >= 0)
                boost::this_fiber::yield();
        }),
        f2([&] {
            while (++c2, n--)
                boost::this_fiber::yield();
        });
    f1.join(), f2.join();
    std::cout << "f1 #" << c1 << " f2 #" << c2 << '\n';
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
        void return_value(size_t n) { val = n; }
        std::coroutine_handle<> then;
        std::optional<size_t> val;
    };
    task(std::coroutine_handle<promise_type> coro) noexcept : coro(coro) {}
    task(task&& t) noexcept : coro(std::exchange(t.coro, {})) {}
    ~task() { if (coro) coro.destroy(); }
    bool await_ready() { return false; }
    auto await_suspend(std::coroutine_handle<> h) noexcept { coro.promise().then = h; return coro; }
    auto await_resume() {}
    auto get() const { return *coro.promise().val; }
    std::coroutine_handle<promise_type> coro;
};

task counter(task& t) {
    std::atomic_size_t c = 0;
    while (++c, !t.coro.done())
        co_await t;
    co_return c;
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
    std::atomic_size_t c = 0;
    while (++c, n--)
        co_await std::suspend_always{};
    co_return c;
}

void coro2(int n) {
    auto f2 = timer2(n), f1 = counter(f2);
    while (!f1.coro.done())
        f1.coro.resume();
    std::cout << "f1 #" << f1.get() << " f2 #" << f2.get() << '\n';
}

int main(int, char** a) {
    (a[1][0] == 'f' ? fibers
    : a[1][0] == 'c' ? coro
    : a[1][0] == 'C' ? coro2
    : exit)(std::atoi(a[2]));
}
