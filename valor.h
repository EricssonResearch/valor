#pragma once
#include <any>
#include <coroutine>
#include <functional>
#include <ufser/ufser.h>
#include <variant>
// #include <iostream>

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
    /// Get last saved coro state
    std::string_view serialize() const { assert(händel); assert(!händel.done()); return händel.promise().state; }
    /// Restore coro state before continuation, no-op for destroyed
    void deserialize(std::string_view s) const { if (händel) händel.promise().state = s; }
    /// Continue the coro if not yet finished; no-op otherwise
    void operator()() const { if (händel && !händel.done()) händel(); }
    /// Check if the coro is unfinished
    operator bool() const { return händel && !händel.done(); }
    /// Return value of the finished coro
    R get() const requires (!std::is_void_v<R>) { assert(händel.done()); return händel ? std::move(händel.promise().retval) : R(); }
    std::any awaiter(size_t i) { return {}; /*how to get type here? virtual wtf? händel.promise().aw.get_as<...>(i);*/ }
    /// Set the next awaiter retval before continuing a suspended coro, no-op otherwise
    void set_aw_ret(auto&& x) const { if (händel) händel.promise().aw.put(std::move(x)); }
    /// Helper awaitable for getting our promise
    struct get_promise {
      promise_type* p;
      bool await_ready() const noexcept { return false; }
      bool await_suspend(handle_type h) noexcept { p = &h.promise(); return false; }
      auto await_resume() const noexcept { return p; }
    };
    struct promise_type : promise_base<R> {
        template<typename T> auto await_transform();
        std::string state;
        using any_vec = std::vector<std::any>;
        /// Helper for a group of co_await expressions
        /// Each element can be either:
        ///  - the user-provided awaiter for the co_await, wrapped in await_transform::ret
        ///  - retval thereof supplied earlier through ser_ctx::set_aw_ret
        /// Indexing of elements should match the number of co_a:s in the full-expr
        struct : private any_vec {
            /// Check if any awaiter/retval is stored
            operator bool() const { return !empty(); }
            /// Check if awaiter #i is stored
            bool has(size_t i) const { /*std::cerr<<"[?aw #"<<i<<' '<<bool(i<size() && (*this)[i].has_value())<<']';*/ return i < size() && (*this)[i].has_value(); }
            /// Move out awaiter result for stage
            template<typename T> requires (!std::same_as<void, decltype(std::declval<promise_type>().await_transform(std::declval<T>()).await_resume())>)
            auto get_as(size_t i) { assert(has(i)); /*std::cerr<<"[get #"<<i<<']';*/ return std::any_cast<decltype(std::declval<promise_type>().await_transform(std::declval<T>()).await_resume())>(std::move((*this)[i])); }
            template<typename T> requires (std::same_as<void, decltype(std::declval<promise_type>().await_transform(std::declval<T>()).await_resume())>)
            auto get_as(size_t i) { assert(has(i)); /*std::cerr<<"[get void #"<<i<<']';*/ }
            /// Append an element
            auto& put(std::any&& a) { /*std::cerr<<"[put #"<<size()<<']';*/ return emplace_back(std::move(a)); }
            /// Clear and reserve space for n awaiters
            void resize(size_t n) { clear(); reserve(n); }
        } aw;
        auto get_return_object() noexcept { return ser_ctx{handle_type::from_promise(*this), true}; }
        std::suspend_always initial_suspend() const noexcept { /*std::cerr << " init susp\n";*/ return {}; }
        std::suspend_always final_suspend() const noexcept { /*std::cerr << " final susp\n";*/ return {}; } // if not susp then autodestroys the handle obj -- but then problem at last coro() in restore()
        void unhandled_exception() const noexcept { std::terminate(); }// TODO store
        template<typename T>//TODO concept Awaiter + movable
        auto await_transform(T&& x) {
            // std::cerr<<"[awtr? "<<bool(aw)<<']';
            struct ret {
                std::variant<std::any, std::reference_wrapper<std::any>> a;
                bool await_ready() { return std::any_cast<T>(get()).await_ready(); }
                auto await_suspend(std::coroutine_handle<> h) { return std::any_cast<T>(get()).await_suspend(h); }//TODO bool-returning variant
                auto await_resume() { return std::any_cast<T>(get()).await_resume(); }
                std::any& get() {
                    if (auto x = std::get_if<0>(&a))
                        return *x;
                    return std::get<1>(a);
                }
            };
            return aw ? ret{std::move(x)} : ret{std::ref(aw.put(std::move(x)))};
        }
        auto await_transform(get_promise&& p) { /*std::cerr<<"[awtr p]";*/ return std::move(p); }// don't touch our own helper
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
            if (!aw)//TODO only works for a single coa
                return;
            auto const& v = uf::deserialize_view_as<uf::any_view>(state).get_content();
            // assert size
            auto i = v.begin();
            ((++i)->get(t = T{}), ...);
        }
    };
};

/// Start the supplied coro's body (ser_ctx creates them lazy)
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

/// Create coro and start it
/// @param fun the function pointer
/// @param ser the serialized state of fun
/// @param aw_ret co_await retvals of the suspended coro
auto continue_from(std::string_view ser, auto fun, auto&&... aw_ret) {
    auto c = std::apply(fun, typename args<decltype(fun)>::a{});
    c.deserialize(ser);
    c.händel.promise().aw.resize(sizeof...(aw_ret));
    (c.set_aw_ret(std::move(aw_ret)), ...);
    return start(std::move(c));
}

}
