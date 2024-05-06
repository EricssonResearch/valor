/** This file is part of the Valor project which is released under the MIT license.
 * See file COPYING for full license details.
 * Copyright 2024 Ericsson AB
 */
// #include <ucontext.h>
// #include <cstdlib>
#include <bit>
#include <iostream>
#include <sstream>
#include <dlfcn.h>

namespace ctx {

namespace {

// void chk(char const* s, int e) {
//     if (e == -1)
//        perror(s), exit(EXIT_FAILURE);
// }
void chk(int e, auto f) {
    if (!e)
       throw std::runtime_error(f());
}

}

std::string addr(void* p) {
    Dl_info i;
    chk(dladdr(p, &i), dlerror);
    // using namespace std::string_literals;
    std::ostringstream s;
    s << "fname:" << i.dli_fname << ",base:" << i.dli_fbase << ",symb:" << i.dli_sname << ",saddr:" << i.dli_saddr;
    return s.str();
}

namespace {

struct ctx {
    void * const rip;
    // ctx() { asm("movq %%rip, (%%rdi)" : "=D" (rip)); }
    // ctx() { asm("movq %%rip, %0" : "=r" (rip)); }
    __attribute__((noinline)) void* retaddr(unsigned fr) {
        switch(fr) {
        case 0: return __builtin_return_address(1);
        case 1: return __builtin_return_address(2);
        case 2: return __builtin_return_address(3);
        case 3: return __builtin_return_address(4);
        case 4: return __builtin_return_address(5);
        default: throw 1;
        }
    }
    __attribute__((noinline)) ctx(unsigned fr) : rip{__builtin_extract_return_addr(retaddr(fr))} {} // 0 is whoever calls the ctor (rip()); 1 is asusp(); 2 is func Frame; 3 is the user func c
    std::string str() const noexcept { std::ostringstream s; s << "rip:" << rip << '[' << addr(rip) << ']'; return s.str(); }
};

}

auto rip(unsigned frame = 1) {
    // ucontext_t u;
    // chk("getuctx", getcontext(&u));
    // std::cerr << " [u: " << &u << "] ";
    return ctx{frame}.str();
}

auto rip_diff_to(void* p, unsigned frame = 1) {
    return (char*)(ctx{frame}.rip) - (char*)p;
}

}