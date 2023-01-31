#include <cstring>
#include <iostream>
#include <coroutine>
#include <malloc.h>
size_t modifier() { return (size_t)modifier; }
// std::string seri(std::coroutine_handle<> h) { return std::to_string((size_t)&h); }
uint16_t seri(std::coroutine_handle<> h) { return *(uint16_t*)(*(char**)&h + 3); }//TODO templated arg to get the promise type // TODO ser(ret const&)
struct ret {
    struct promise_type;
    std::coroutine_handle<promise_type> h;
    struct promise_type {
        ret get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_always/*otherwise op() on a finished fun segfaults*/ final_suspend() noexcept { return {}; }
        void unhandled_exception() {}
        void return_void() {} // called after falling-off; UB if not declared in that case
    };
    void operator()() const { h(); } // TODO done() is UB if *not* suspended -- which it is not after returning from the fun
    operator bool() const { return h && !h.done(); }
};
void resi(ret const& r, uint16_t i) { *(uint16_t*)(*(char**)&r.h + 3) = i; }
struct h {
    int const i_of_h;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    int await_resume() const noexcept { return i_of_h + ((((size_t)&i_of_h) >> 32) & 0xff); }
};
ret g(int in, float dummy) {
    int i = in;
    struct S {
        int k;
    };
    S Is_1{(size_t) &i};
    i += Is_1.k;
    {
        auto x = (size_t)g;
        auto y = (size_t)&x;
        i += (y >> 32) & 0xff;
    }
    // std::cout << "i0: " << i << '\n';
    std::cout << "g susp #1\n";
    i = co_await h{i};
    // std::cout << "i after 1: " << i << '\n';
    int j = 17;
    {
        auto zs = (size_t) &j;
        i += (zs >> 32) & 0xff;
    }
    i += (((size_t)&j) >> 32) & 0xff;
    std::cout << "g susp #2\n";
    i = co_await h{i};
    // std::cout << "i after 2: " << i << '\n';
    // co_return {i};
    std::cout << "g susp #3\n";
    co_await std::suspend_always{};
    std::cout << "g out\n";
}
int main() {
    auto c = g(3, {});
    std::clog << "orig handle " << c.h.address() << '\n';
    std::clog << "continue once\n";
    c();
    auto stage = seri(c.h);
    std::clog << "stage: " << stage << '\n';
    c.h.destroy();

    c = g(4, {});
    std::clog << "new handle " << c.h.address() << " stage " << seri(c.h) << '\n';
    resi(c, stage);
    std::clog << "continue from " << seri(c.h) << " till the end\n";
    while (c)
        c();
}

// ret g(int) {
//     co_await h{};
// }
/*
;; Function ret g(int) (_Z1gi)
;; enabled by -tree-original


{
  struct _Z1gi.Frame * _Coro_frameptr = 0B;
  bool _Coro_promise_live = 0;
  bool _Coro_gro_live = 0;

  <<cleanup_point   struct _Z1gi.Frame * _Coro_frameptr = 0B;>>;
  <<cleanup_point   bool _Coro_promise_live = 0;>>;
  <<cleanup_point   bool _Coro_gro_live = 0;>>;
  <<cleanup_point <<< Unknown tree: expr_stmt
    (void) (_Coro_frameptr = (struct _Z1gi.Frame *) operator new (.CO_FRAME (48, _Coro_frameptr))) >>>>>;
  <<< Unknown tree: try_block
    <<cleanup_point <<< Unknown tree: expr_stmt
      (void) (_Coro_frameptr->_Coro_frame_needs_free = 1) >>>>>;
    <<cleanup_point <<< Unknown tree: expr_stmt
      (void) (_Coro_frameptr->_Coro_resume_fn = g) >>>>>;
    <<cleanup_point <<< Unknown tree: expr_stmt
      (void) (_Coro_frameptr->_Coro_destroy_fn = g) >>>>>;
    <<cleanup_point <<< Unknown tree: expr_stmt
      (void) (_Coro_frameptr->_Coro_unnamed_parm_0 = *(int &) &D.56168) >>>>>;
    {
      <<cleanup_point <<< Unknown tree: expr_stmt
        (void) (<retval> = TARGET_EXPR <D.56310, ret::promise_type::get_return_object (&_Coro_frameptr->_Coro_promise)>) >>>>>;
      <<cleanup_point <<< Unknown tree: expr_stmt
        (void) (_Coro_frameptr->_Coro_resume_index = 0) >>>>>;
      <<cleanup_point g (_Coro_frameptr)>>;
      return <retval>;
    }
    <<< Unknown tree: handler
      
      try
        {
          <<cleanup_point <<< Unknown tree: expr_stmt
            (void) __cxa_begin_catch (__builtin_eh_pointer (0)) >>>>>;
          <<cleanup_point <<< Unknown tree: expr_stmt
            operator delete ((void *) _Coro_frameptr) >>>>>;
          <<cleanup_point <<< Unknown tree: expr_stmt
            <<< Unknown tree: throw_expr
              __cxa_rethrow () >>> >>>>>;
        }
      finally
        {
          __cxa_end_catch ();
        } >>> >>>;
}
*/
