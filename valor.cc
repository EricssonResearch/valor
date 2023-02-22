#include <valor.h>
#include <iostream>

using namespace valor;
using namespace std::chrono_literals;

#define LOG std::cerr << '[' << getpid() << "] "

struct remote_op {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept { LOG << " remote::asusp " << this << " on handle " << h.address() << '\n'; }
    void await_resume() const noexcept { LOG << " remote::aresume " << this << '\n'; }
};

void nontriv_work(int& i) { i += size_t(&i); }

ser_ctx<int> k(int in) {
    LOG << "k: started with in=" << in << '\n';
    nontriv_work(in);
    LOG << "k: 1st coawait with in=" << in << '\n';
    co_await remote_op();
    int j = 17;
    nontriv_work(j);
    LOG << "k: 2nd coawait j=" << j << '\n';
    co_await remote_op();
    LOG << "k: done in=" << in << " j=" << j << '\n';
    co_return 14;
}

// [[valor_skip]]
// ser_ctx<int> k_valor(int in) {
// auto p = co_await ser_ctx<int>::get_promise{};
// if (auto st = p->stage()) {
//     std::cerr << "k: to stage " << st << '\n';
//     static ssize_t stages[] = {0, (char*)&&s1 - (char*)&&s0, (char*)&&s2 - (char*)&&s0};
//     // assert(st < stages.size);
//     goto *(void*)((char*)&&s0 + stages[st]);
// }
// s0:
//     std::cerr << "k: started with in=" << in << '\n';
//     nontriv_work(in);
//     std::cerr << "k: 1st coawait with in=" << in << '\n';
// p->save(1, in);
//     co_await remote_op();
// s1:
// p->load(in);
//     int j = 17;//TODO [attr] to skip saving
//     nontriv_work(j);
//     std::cerr << "k: 2nd coawait j=" << j << '\n';
// p->save(2, in, j);
//     co_await remote_op();
// s2:
// p->load(in, j);
//     std::cerr << "k: done in=" << in << " j=" << j << '\n';
//     co_return 14;
// }

void saving() {
    LOG << "start fun\n";
    auto coro = start(k(45));
    auto s = coro.serialize();
    LOG << "saving coro state " << (s.size() ? uf::any_view(uf::from_raw, s).print() : "") << '\n';
    std::cout << s;
}

void restoring(std::string_view ser) {
    LOG << "restore from " << (ser.size() ? uf::any_view(uf::from_raw, ser).print() : "") << '\n';
    auto coro = continue_from(ser, k);
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
