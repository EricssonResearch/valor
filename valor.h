#pragma once
#include <coroutine>
#include <iostream>
#include <ufser/ufser.h>

namespace valor {

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

}
