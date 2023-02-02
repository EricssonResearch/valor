#include <iostream>
#include <coroutine>
#include <chrono>
#include <charconv>
#include <ufser.h>

using namespace std::chrono_literals;
// using namespace std::string_literals;

// TODO coro_traits for a ufserializable

template<typename T>
struct promise_base { // the alternative is no_unique_addr bleh in e.g., https://brevzin.github.io/c++/2021/11/21/conditional-members/#conditional-member-variables
    T retval;//TODO ?dfl-constructible is enough / too much?
    void return_value(auto&& t) { retval = std::move(t); }
};

template<typename T> requires (std::is_void_v<T>)
struct promise_base<T> {
    void return_void() {}
};

template<typename R = void>
struct ser_ctx {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    handle_type händel;
    bool owner;//TODO sh refcount?
    ser_ctx() noexcept = default;
    ser_ctx(handle_type&& h, bool delete_on_exit) noexcept : händel(std::move(h)), owner(delete_on_exit) {}
    ser_ctx(ser_ctx const&) = delete;
    ser_ctx(ser_ctx&& o) { (*this) = std::move(o); }
    ser_ctx& operator=(ser_ctx const&) = delete;
    ser_ctx& operator=(ser_ctx&& o) noexcept { händel = std::move(o.händel); owner = false; std::swap(owner, o.owner); return *this; }
    ~ser_ctx() { if (owner && händel) händel.destroy(); }
    std::string_view serialize() const { return händel.promise().state; }//TODO chk händel
    void deserialize(std::string_view s) const { händel.promise().state = s; }
    void operator()() const { std::cerr << " h resume\n"; händel(); }
    operator bool() const { return händel && !händel.done(); }
    R get() const requires (!std::is_void_v<R>) { return händel ? std::move(händel.promise().retval) : R(); }//TODO chk done?
    struct promise_type : promise_base<R> {
        std::string state;
        auto get_return_object() noexcept { return ser_ctx{handle_type::from_promise(*this), true}; }
        std::suspend_always initial_suspend() const noexcept { std::cerr << " init susp\n"; return {}; }
        std::suspend_always final_suspend() const noexcept { std::cerr << " final susp\n"; return {}; } // if not susp then autodestroys the handle obj -- but then problem at last coro() in restore()
        void unhandled_exception() const noexcept {}// TODO store
        uint16_t stage() const {
             if (state.size()) {
                auto a = uf::deserialize_view_as<uf::any_view>(state);
                if (a.is_structured_type())
                    a = a.get_content(1)[0];
                return a.get_view_as<unsigned>();
             }
             return 0;
        }
        void save(uint16_t i, auto const&... t) { state = uf::serialize(uf::any(std::forward_as_tuple(unsigned(i), t...))); }// TODO catch ufser error in a sepaarate member
        template<typename... T>
        void load(T&... t) const {
            auto const& v = uf::deserialize_view_as<uf::any_view>(state).get_content();
            // assert size
            auto i = v.begin();
            ((++i)->get(t = T{}), ...);
        }
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

int main(int n, char** a) {
    if (a[1][0] == 's')
        saving();
    if (a[1][0] == 'r') {
        std::ostringstream s;
        s << std::cin.rdbuf();
        restoring(s.str());
    }
}
