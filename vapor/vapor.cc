#include <gcc-plugin.h>
#include <plugin-version.h>
#include <tree.h>
#include <tree-pass.h>
#include <tree-iterator.h>
#include <cp/cp-tree.h>
#include <stringpool.h>
#include <gimple.h>
#include <gimple-iterator.h>
#include <gimple-pretty-print.h>
#include <print-tree.h>
#include <context.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cassert>

using namespace std::string_literals;

int plugin_is_GPL_compatible;

plugin_info vapor_info{"0.1", "Blah"};

std::vector<std::string> gimple_codes;

pass_data const vapor_pass_data{
    GIMPLE_PASS,
    "vapor",
    OPTGROUP_NONE,
    TV_NONE,
    PROP_gimple_any,
};

struct vapor_pass : gimple_opt_pass {
    vapor_pass(gcc::context* ctx) : gimple_opt_pass(vapor_pass_data, ctx) {}
    unsigned execute(function* fun) override {
        std::cerr << "function " << function_name(fun) << ' ' << (LOCATION_FILE(fun->function_start_locus) ? : "<unknown>") << ':' << LOCATION_LINE(fun->function_start_locus) << '\n';
        print_gimple_seq(stderr, fun->gimple_body, 1, TDF_RAW);
        // FOR_EACH_LOCAL_DECL(fun) ...
        // basic_block bb;
        // FOR_ALL_BB_FN(bb, fun)
        //     std::clog << " bb\n";
        //     for (auto gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
        //         auto stmt = gsi_stmt(gsi);
        //         switch (gimple_code(stmt)) {
        //             case GIMPLE_CALL: {
        //                 auto lhs = gimple_call_lhs(stmt);
        //                 for (int i = 0; i < gimple_call_num_args(stmt); ++i) {
        //                     tree arg = gimple_call_arg(stmt, i);
        //                     // erase_if_used_lhs(potential_unused_lhs, arg);
        //                 }
        //                 break;
        //             }
        //           // case GIMPLE_ASSIGN:
        //           //   {
        //           //     tree lhs = gimple_assign_lhs(stmt);
        //           //     erase_if_used_lhs(potential_unused_lhs, lhs);

        //           //     tree rhs1 = gimple_assign_rhs1(stmt);
        //           //     erase_if_used_lhs(potential_unused_lhs, rhs1);

        //           //     tree rhs2 = gimple_assign_rhs2(stmt);
        //           //     if (rhs2 != NULL)
        //           //       erase_if_used_lhs(potential_unused_lhs, rhs2);

        //           //     tree rhs3 = gimple_assign_rhs3(stmt);
        //           //     if (rhs3 != NULL)
        //           //       erase_if_used_lhs(potential_unused_lhs, rhs3);

        //           //     break;
        //           //   }
        //           //   break;
        //           // case GIMPLE_RETURN:
        //           //   {
        //           //     erase_if_used_lhs(potential_unused_lhs,
        //           //         gimple_return_retval(as_a<greturn*>(stmt)));
        //           //     break;
        //           //   }
        //             default:
        //                 std::cerr << " [gimple code " << int(gimple_code(stmt)) << "]\n";
        //         }
        //     }


        //   gimple_bb_info *bb_info = &bb->il.gimple;

        //   std::cerr << "bb_" << fun << "_" << bb->index << "[label=\"";
        //   if (bb->index == 0)
        //   {
        //     std::cerr << "ENTRY: "
        //               << function_name(fun) << "\n"
        //               << (LOCATION_FILE(fun->function_start_locus) ? : "<unknown>")
        //               << ":" << LOCATION_LINE(fun->function_start_locus);
        //   }
        //   else if (bb->index == 1)
        //   {
        //     std::cerr << "EXIT: "
        //               << function_name(fun) << "\n"
        //               << (LOCATION_FILE(fun->function_end_locus) ? : "<unknown>") <<
        //               ":" << LOCATION_LINE(fun->function_end_locus);
        //   }
        //   else
        //   {
        //     print_gimple_seq(stderr, bb_info->seq, 0, 0);
        //   }
        //   std::cerr << "\"];\n";

        //   edge e;
        //   edge_iterator ei;

        //   FOR_EACH_EDGE(e, ei, bb->succs)
        //   {
        //     basic_block dest = e->dest;
        //     std::
        //     cerr << "bb_" << fun << "_" << bb->index << " -> bb_" << fun <<
        //          "_" << dest->index << ";\n";
        //   }
        // }

        // std::cerr << "}\n";
        return 0;
    }
    vapor_pass* clone() override { return this; }
};

#define DECL_NAME_STR(x) (DECL_NAME(x) ? IDENTIFIER_POINTER(DECL_NAME((x))) : "<anon>")

void walk(tree t, size_t spaces = 0) {
    std::string indent(++spaces, ' ');
    auto pr = [&](auto d) {
        std::clog << indent << get_tree_code_name(TREE_CODE(d)) << ' ' << DECL_NAME_STR(d) << ' ' << DECL_SOURCE_FILE(d) << ':' << DECL_SOURCE_LINE(d) << '\n';
    };// OVL_CURRENT / _NEXT for DECL_...
    for (auto d = cp_namespace_decls(t); d; d = TREE_CHAIN(d)) {
        // if (!DECL_IS_UNDECLARED_BUILTIN(d))
            pr(d);
        // if ()
    }
}

#define INT(d) int_cst_value(d)

void walk_rec(tree r, std::string_view indent, bool recurse = true) {
    gcc_assert(TREE_CODE(r) == RECORD_TYPE);
    std::clog << indent << "= struct " << DECL_NAME_STR(TYPE_NAME(r)) << '\n';
    static std::vector<tree> visited;
    for (auto i: visited)
        if (same_type_p(r, i)) {//TODO TYPE_MAIN_VARIANT
            std::clog << indent << " (already seen)\n";
            return;
        }
    visited.emplace_back(r);
    for (auto f = TYPE_FIELDS(r); f; f = TREE_CHAIN(f))
        switch (TREE_CODE(f)) {
        case FIELD_DECL: {
            // std::clog << indent << ' ' << get_tree_code_name(TREE_CODE(TREE_TYPE(f))) << ' ' << DECL_NAME_STR(f) << '\n';
            // debug_tree(f);
            auto t = TREE_TYPE(f);
            switch (TREE_CODE(t)) {
            case POINTER_TYPE:
                if (TREE_CODE(TREE_TYPE(t)) == FUNCTION_TYPE) {
                    std::clog << indent << " member fun_ptr " << DECL_NAME_STR(f) << '\n';
                    // debug_tree(TREE_TYPE(t));
                } else
                    std::clog << indent << " TODO member ptr " << get_tree_code_name(TREE_CODE(TREE_TYPE(t))) << '\n';
                break;
            case RECORD_TYPE:
                std::clog << indent << " member " << DECL_NAME_STR(f) << '\n';
                if (recurse)
                    walk_rec(t, std::string(indent) + "  ", true);
                break;
            case INTEGER_TYPE:
                std::clog << indent << " member " << DECL_NAME_STR(f) << (TYPE_UNSIGNED(t) ? " unsigned" : " signed") << " size " << INT(TYPE_SIZE(t)) << " (prec " << TYPE_PRECISION(t) << ")\n";
                break;
            default:
                std::clog << indent << " TODO member " << get_tree_code_name(TREE_CODE(t)) << ' ' << DECL_NAME_STR(f) << '\n';
            }
            if (DECL_C_BIT_FIELD(t))
                std::clog << indent << " !!! TODO bitfield\n";
            assert(ssize_t(DECL_OFFSET_ALIGN(f)) >= INT(DECL_FIELD_BIT_OFFSET(f)) + INT(DECL_SIZE(f))); // fits the word
            std::clog << indent << "  size " << INT(DECL_SIZE(f)) << " at offset " << INT(DECL_FIELD_OFFSET(f)) << "B+" << INT(DECL_FIELD_BIT_OFFSET(f)) << '\n';
            break;
        }
        case TYPE_DECL:
            if (!DECL_SELF_REFERENCE_P(f)) {
                std::clog << indent << " inner struct " << DECL_NAME_STR(f) << '\n';
                if (recurse)
                    walk_rec(TREE_TYPE(f), std::string(indent) + "  ", true); // TODO would recurse inf --> walk_tree_once()
                // debug_tree(r);
            }
            break;
        default:
            std::clog << indent << " TODO " << get_tree_code_name(TREE_CODE(f)) << ' ' << DECL_NAME_STR(f) << '\n';
        }
    // debug_tree(r);
}

void walk_vars(tree body, std::string_view indent = " ") {
    if (TREE_CODE(body) == BIND_EXPR) {
        for (auto v = BIND_EXPR_VARS(body); v; v = TREE_CHAIN(v)) {
            if (TREE_CODE(v) == VAR_DECL) {
                std::clog << indent << "var " << DECL_NAME_STR(v) << '\n';
                if ("_Coro_frameptr"s == DECL_NAME_STR(v))
                    // gcc_assert(TYPE_PTR_P(TREE_TYPE(v)));
                    // gcc_assert(TREE_CODE(TREE_TYPE(TREE_TYPE(v))) == RECORD_TYPE);
                    walk_rec(TREE_TYPE(TREE_TYPE(v)), std::string(indent) + ' ');
            } else
                std::clog << indent << "skip nonvar " << get_tree_code_name(TREE_CODE(v)) << '\n';
        }
        body = BIND_EXPR_BODY(body);
    }
    if (TREE_CODE(body) != STATEMENT_LIST) {
        std::clog << indent << "skip body " << get_tree_code_name(TREE_CODE(body)) << '\n';
        return;
    }
    for (auto i = tsi_start(body); !tsi_end_p(i); tsi_next(&i)) {
        auto st = tsi_stmt(i);
        if (TREE_CODE(st) == BIND_EXPR)
            walk_vars(st, std::string(indent) + ' ');
        else
            std::clog << indent << " skip stmt " << get_tree_code_name(TREE_CODE(st)) << '\n';
    }
}

tree find_coro_frame(tree t) {
    return cp_walk_tree_without_duplicates(&t, [](tree* t, auto, auto) {
        return TREE_CODE(*t) == VAR_DECL /*TODO: why static? cf. FIELD_DECL?*/ && DECL_NAME_STR(*t) == "_Coro_frameptr"s
            ? *t : NULL_TREE;
    }, nullptr);
}

tree find_coro_handle(tree t) {
    gcc_assert(TREE_CODE(t) == RECORD_TYPE);
    for (auto f = TYPE_FIELDS(t); f; f = TREE_CHAIN(f))
        if (auto i = cp_walk_tree_without_duplicates(&f, [](tree* t, auto, auto) {
                        return TREE_CODE(*t) == FIELD_DECL && TREE_CODE(TREE_TYPE(*t)) == RECORD_TYPE && DECL_NAME_STR(*t) == "_Coro_self_handle"s
                            ? *t : NULL_TREE;
                    }, nullptr))
            return i;
    return NULL_TREE;
}

void do_fun_0_patch_handle(tree f) {
    std::clog << "coro fun " << DECL_NAME_STR(f) << " at " << DECL_SOURCE_FILE(f) << ':' << DECL_SOURCE_LINE(f) << '\n';
    auto body = DECL_SAVED_TREE(f);
    // walk_vars(body);
    // debug_tree(body);
    if (auto fr = find_coro_frame(body)) {
        std::clog << " frame";
        // walk_rec(TREE_TYPE(TREE_TYPE(fr)), " ");
        // debug_tree(TREE_TYPE(TREE_TYPE(fr)));
        if (auto h = find_coro_handle(TREE_TYPE(TREE_TYPE(fr)))) {
            auto ht = TREE_TYPE(h);
            std::clog << " :: handle";
            // walk_rec(ht, " ");
            // debug_tree(h);
            std::clog << " modify\n";
            auto first = TYPE_FIELDS(ht);
            // debug_tree(first);
            // debug_tree(TREE_VALUE(TREE_CHAIN(TREE_CHAIN(TYPE_ARG_TYPES(TREE_TYPE(first))))));
            auto tl = build_tree_list(NULL_TREE, void_type_node);
            // debug_tree(tl);
            auto mm = build_method_type_directly(ht, void_type_node, tl);
            // debug_tree(mm);
            auto my = build_decl(DECL_SOURCE_LOCATION(h), FUNCTION_DECL, get_identifier("seri"), mm);
            // debug_tree(my);
            //TODO
            //  public abstract external in_system_header autoinline decl_3 decl_8 QI /usr/include/c++/12/coroutine:195:12 align:16 warn_if_not_align:0 context <record_type 0x7f95b9c2dbd0 coroutine_handle>
            //  full-name "constexpr std::__n4861::coroutine_handle<ret::promise_type>::~coroutine_handle() noexcept (<uninstantiated>)"
            //  not-really-extern ...
            TREE_CHAIN(my) = first;
            TYPE_FIELDS(ht) = my;
            // debug_tree(ht);
            walk_rec(ht, " ");
        }
    }
}

struct coro_descr {
    std::string fun, file;
    int line;
    uint16_t idx;
};

// std::vector<coro_descr> coros;
// std::vector<tree> coro_funcs;
std::unordered_map<tree, tree> coros; // fun-to-frame

uint16_t coro_member_offset(tree r, std::string_view n) {
    r = TREE_TYPE(TREE_TYPE(r));
    gcc_assert(TREE_CODE(r) == RECORD_TYPE);
    for (auto f = TYPE_FIELDS(r); f; f = TREE_CHAIN(f))
        if (TREE_CODE(f) == FIELD_DECL && DECL_NAME_STR(f) == n) {
            assert(INT(DECL_SIZE(f)) == 16);
            assert(INT(DECL_FIELD_BIT_OFFSET(f)) % 8 == 0);
            return INT(DECL_FIELD_OFFSET(f)) + INT(DECL_FIELD_BIT_OFFSET(f)) / 8;
        }
    return -1;
}

void collect_coro(tree f) {
    if (!DECL_COROUTINE_P(f))
        return;
    std::clog << "coro fun " << DECL_NAME_STR(f) << " at " << DECL_SOURCE_FILE(f) << ':' << DECL_SOURCE_LINE(f) << '\n';
    auto body = DECL_SAVED_TREE(f);
    if (auto fr = find_coro_frame(body)) {
        std::clog << " frame";
        walk_rec(TREE_TYPE(TREE_TYPE(fr)), " ");
        // debug_tree(TREE_TYPE(TREE_TYPE(fr)));
        // coros.emplace_back(coro_descr{DECL_NAME_STR(f), DECL_SOURCE_FILE(f), DECL_SOURCE_LINE(f), find_coro_res_idx(fr)});
        coros[f] = fr; // TODO map to promise type, not the fun
    } else
        std::clog << " !! no frame?!\n";
}

/*
coro fun g at ../toy.cc:36
 frame = struct _Z1gif.Frame
  member fun_ptr _Coro_resume_fn
   size 64 at offset 0B+0
  member fun_ptr _Coro_destroy_fn
   size 64 at offset 0B+64
  member _Coro_promise
   = struct promise_type
    TODO function_decl __dt 
    TODO function_decl __dt_base 
    TODO function_decl __dt_comp 
    TODO function_decl __ct 
    TODO function_decl __ct_base 
    TODO function_decl __ct_comp 
    TODO function_decl __ct 
    TODO function_decl __ct_base 
    TODO function_decl __ct_comp 
    TODO function_decl __ct 
    TODO function_decl __ct_base 
    TODO function_decl __ct_comp 
    TODO function_decl get_return_object
    TODO function_decl initial_suspend
    TODO function_decl final_suspend
    TODO function_decl unhandled_exception
   size 8 at offset 16B+0
  member _Coro_self_handle
   = struct coroutine_handle
    TODO function_decl __dt 
    TODO function_decl __dt_base 
    TODO function_decl __dt_comp 
    TODO function_decl __ct 
    TODO function_decl __ct_base 
    TODO function_decl __ct_comp 
    TODO function_decl __ct 
    TODO function_decl __ct_base 
    TODO function_decl __ct_comp 
    TODO function_decl from_promise
    TODO function_decl operator=
    TODO function_decl address
    TODO function_decl from_address
    TODO function_decl __conv_op 
    TODO function_decl __conv_op 
    TODO function_decl done
    TODO function_decl operator()
    TODO function_decl resume
    TODO function_decl destroy
    TODO function_decl promise
    TODO member ptr void_type
     size 64 at offset 0B+0
   size 64 at offset 16B+64
  member in signed size 32 (prec 32)
   size 32 at offset 32B+0
  TODO member real_type dummy
   size 32 at offset 32B+32
  member _Coro_resume_index unsigned size 16 (prec 16)
   size 16 at offset 32B+64
  TODO member boolean_type _Coro_frame_needs_free
   size 8 at offset 32B+80
  TODO member boolean_type _Coro_initial_await_resume_called
   size 8 at offset 32B+88
  member Is_1_1
   = struct suspend_never
    TODO function_decl __dt 
    TODO function_decl __dt_base 
    TODO function_decl __dt_comp 
    TODO function_decl await_ready
    TODO function_decl await_suspend
    TODO function_decl await_resume
   size 8 at offset 32B+96
  member i_1_2 signed size 32 (prec 32)
   size 32 at offset 48B+0
  member Is_1_1_2
   = struct S
    TODO function_decl __dt 
    TODO function_decl __dt_base 
    TODO function_decl __dt_comp 
    TODO function_decl __ct 
    TODO function_decl __ct_base 
    TODO function_decl __ct_comp 
    TODO function_decl __ct 
    TODO function_decl __ct_base 
    TODO function_decl __ct_comp 
    TODO function_decl __ct 
    TODO function_decl __ct_base 
    TODO function_decl __ct_comp 
    member k signed size 32 (prec 32)
     size 32 at offset 0B+0
   size 32 at offset 48B+32
  member j_1_2 signed size 32 (prec 32)
   size 32 at offset 48B+64
  member x_2_3 unsigned size 64 (prec 64)
   size 64 at offset 64B+0
  member y_2_3 unsigned size 64 (prec 64)
   size 64 at offset 64B+64
  member Aw0_2_4
   = struct h
    TODO function_decl __dt 
    TODO function_decl __dt_base 
    TODO function_decl __dt_comp 
    member i_of_h signed size 32 (prec 32)
     size 32 at offset 0B+0
    TODO function_decl await_ready
    TODO function_decl await_suspend
    TODO function_decl await_resume
   size 32 at offset 80B+0
  member zs_2_5 unsigned size 64 (prec 64)
   size 64 at offset 80B+64
  member Aw1_2_6
   = struct h
    (already seen)
   size 32 at offset 96B+0
  member Fs_1_7
   = struct suspend_never
    (already seen)
   size 8 at offset 96B+32
*/
std::string scope(tree t) {
    std::string s;
    for (t = CP_DECL_CONTEXT(t); t != global_namespace; t = CP_DECL_CONTEXT(t)) {
        if (TREE_CODE(t) == RECORD_TYPE)
            t = TYPE_NAME(t);
        s.insert(0, "::");
        s.insert(0, DECL_NAME_STR(t));
    }
    return s;
}

tree get_global_fn(std::string_view n) {
    for (auto t = cp_namespace_decls(global_namespace); t; t = TREE_CHAIN(t))
        if (t != error_mark_node && TREE_CODE(t) == FUNCTION_DECL && DECL_NAME_STR(t) == n)
            return t;
    return NULL_TREE;
}

void patch_seri() {
    std::clog << "coros:\n";
    for (auto [fun,frame]: coros) {
        std::clog << DECL_NAME_STR(fun) << ' ' << DECL_SOURCE_FILE(fun) << ':' << DECL_SOURCE_LINE(fun)
            << "  residx at +" << coro_member_offset(frame, "_Coro_resume_index")
            << '\n';
        auto seri = get_global_fn("seri");// TODO matching seri()
        std::clog << " seri\n";
        debug_tree(seri);
        // tree arg = TREE_VALUE(TYPE_ARG_TYPES(TREE_TYPE(seri))); // == record-type coro handle
        // std::clog << " ARG\n";
        // debug_tree(arg);
        // walk_rec(arg, " ");
        // std::clog << " BODY\n";
        auto body = DECL_SAVED_TREE(seri);
        // debug_tree(body);
        auto ie = TREE_OPERAND(body, 0); // #0 = #1
        gcc_assert(TREE_CODE(ie) == INIT_EXPR);
        // debug_tree(ie);
        // auto ptrpls = TREE_OPERAND(TREE_OPERAND(ie, 1)/*=indirect_ref*/, 0); // = #0(ptr) + #1(cst)
        auto ptrpls = TREE_OPERAND(TREE_OPERAND(TREE_OPERAND(ie, 1)/*=indirect_ref*/, 0)/*=nop*/, 0); // = #0(ptr) + #1(cst)
        gcc_assert(TREE_CODE(ptrpls) == POINTER_PLUS_EXPR);
        // debug_tree(TREE_OPERAND(ptrpls, 1));
        TREE_OPERAND(ptrpls, 1) = build_int_cst(TREE_TYPE(TREE_OPERAND(ptrpls, 1)), coro_member_offset(frame, "_Coro_resume_index")); // Had to preserve orig type as size_type_node creates a long int, not 'sizetype'
        // debug_tree(TREE_OPERAND(ptrpls, 1));
        // debug_tree(body);

        auto resi = get_global_fn("resi");// TODO matching arg type
        std::clog << " resi\n";
        // debug_tree(resi);
        // tree arg = TREE_VALUE(TYPE_ARG_TYPES(TREE_TYPE(resi))); // == record-type coro handle
        // std::clog << " ARG\n";
        // debug_tree(arg);
        // walk_rec(arg, " ");
        body = DECL_SAVED_TREE(resi);
        ptrpls = TREE_OPERAND(TREE_OPERAND(TREE_OPERAND(TREE_OPERAND(TREE_OPERAND(TREE_OPERAND(body/*=cleanup*/, 0)/*=expr*/, 0)/*=conv*/, 0)/*=modif*/, 0)/*=indir*/, 0)/*=nop*/, 0);
        TREE_OPERAND(ptrpls, 1) = build_int_cst(TREE_TYPE(TREE_OPERAND(ptrpls, 1)), coro_member_offset(frame, "_Coro_resume_index"));
        debug_tree(body);
    }
}

/*  seri
<return_expr 0x7fa0e028e700
    type <void_type 0x7fa0e2a23f18 void type_6 VOID
        align:8 warn_if_not_align:0 symtab:0 alias-set -1 canonical-type 0x7fa0e2a23f18
        pointer_to_this <pointer_type 0x7fa0e2a2b000>>
    side-effects
    arg:0 <init_expr 0x7fa0e0296230
        type <integer_type 0x7fa0e1874690 uint16_t unsigned type_6 HI
            size <integer_cst 0x7fa0e2a25108 constant 16>
            unit-size <integer_cst 0x7fa0e2a25120 constant 2>
            align:16 warn_if_not_align:0 symtab:0 alias-set -1 canonical-type 0x7fa0e2a23540 precision:16 min <integer_cst 0x7fa0e2a25138 0> max <integer_cst 0x7fa0e2a250f0 65535>
            pointer_to_this <pointer_type 0x7fa0e02957e0>>
        side-effects
        arg:0 <result_decl 0x7fa0e0281e10 D.56168 type <integer_type 0x7fa0e1874690 uint16_t>
            unsigned ignored HI ../toy.cc:17:40 size <integer_cst 0x7fa0e2a25108 16> unit-size <integer_cst 0x7fa0e2a25120 2>
            align:16 warn_if_not_align:0 context <function_decl 0x7fa0e0290500 seri>>
        arg:1 <indirect_ref 0x7fa0e028e660 type <integer_type 0x7fa0e1874690 uint16_t>
           
            arg:0 <nop_expr 0x7fa0e028e680 type <pointer_type 0x7fa0e02957e0>
                tree_0
                arg:0 <pointer_plus_expr 0x7fa0e0296208 type <pointer_type 0x7fa0e2a2ddc8>
                   
                    arg:0 <indirect_ref 0x7fa0e028e6a0 type <pointer_type 0x7fa0e2a2ddc8>
                       
                        arg:0 <nop_expr 0x7fa0e028e6c0 type <pointer_type 0x7fa0e2270738>
                            tree_0 arg:0 <addr_expr 0x7fa0e028e6e0>
                            ../toy.cc:17:65 start: ../toy.cc:17:65 finish: ../toy.cc:17:74>
                        ../toy.cc:17:64 start: ../toy.cc:17:64 finish: ../toy.cc:17:74>
                    arg:1 <integer_cst 0x7fa0e22571f8 constant 3>
*/

/*  resi
<cleanup_point_expr 0x7fed5325cb00
    type <void_type 0x7fed55a23f18 void type_6 VOID
        align:8 warn_if_not_align:0 symtab:0 alias-set -1 canonical-type 0x7fed55a23f18
        pointer_to_this <pointer_type 0x7fed55a2b000>>
    side-effects
    arg:0 <expr_stmt 0x7fed5325cae0 type <void_type 0x7fed55a23f18 void>
        side-effects
        arg:0 <convert_expr 0x7fed5325cac0 type <void_type 0x7fed55a23f18 void>
            side-effects
            arg:0 <modify_expr 0x7fed532673e8 type <integer_type 0x7fed5484e690 uint16_t>
                side-effects
                arg:0 <indirect_ref 0x7fed5325ca80 type <integer_type 0x7fed5484e690 uint16_t>
                   
                    arg:0 <nop_expr 0x7fed5325ca60 type <pointer_type 0x7fed532577e0>
                        tree_0
                        arg:0 <pointer_plus_expr 0x7fed53267398 type <pointer_type 0x7fed55a2ddc8>
                            arg:0 <indirect_ref 0x7fed5325ca20> arg:1 <integer_cst 0x7fed552571f8 3>
*/

/*
## orig

0000000000001240 <seri(std::__n4861::coroutine_handle<void>)>:
// std::string seri(std::coroutine_handle<> h) { return std::to_string((size_t)&h); }
uint16_t seri(std::coroutine_handle<> h) { return *(uint16_t*)((char*)&h + 3); }
    1240:       55                      push   %rbp
    1241:       48 89 e5                mov    %rsp,%rbp
    1244:       48 89 7d f8             mov    %rdi,-0x8(%rbp)
    1248:       0f b7 45 fb             movzwl -0x5(%rbp),%eax
    124c:       5d                      pop    %rbp
    124d:       c3                      ret

## rewritten

0000000000001240 <seri(std::__n4861::coroutine_handle<void>)>:
// std::string seri(std::coroutine_handle<> h) { return std::to_string((size_t)&h); }
uint16_t seri(std::coroutine_handle<> h) { return *(uint16_t*)((char*)&h + 3); }
    1240:       55                      push   %rbp
    1241:       48 89 e5                mov    %rsp,%rbp
    1244:       48 89 7d f8             mov    %rdi,-0x8(%rbp)
    1248:       0f b7 45 20             movzwl 0x20(%rbp),%eax
    124c:       5d                      pop    %rbp
    124d:       c3                      ret
*/

int plugin_init(plugin_name_args* info, plugin_gcc_version* gccver) {
    if (!plugin_default_version_check(gccver, &gcc_version)) {
        std::cerr << "Vapor is for GCC " << GCCPLUGIN_VERSION_MAJOR << '.' << GCCPLUGIN_VERSION_MINOR << '\n';
        return 1;
    }
// #define DEFGSCODE(n, s, b) std::cerr << #n << ' ' << s << ' ' << int((n)) << '\n';
#define DEFGSCODE(n, s, b) gimple_codes.resize((n) + 1); gimple_codes[n] = s;
#include <gimple.def>
    // std::clog << "Gimple stmt codes:\n";
    // for (int i = 0; i < gimple_codes.size(); ++i)
    //     std::clog << ' ' << i << ' ' << gimple_codes[i] << '\n';
    // std::clog << "This is Vapor v" << vapor_info.version << ", a dummy plugin for GCC\n";
    // for (int i = 0; i < info->argc; ++i)
    //     std::clog << "  " << info->argv[i].key << " = " << info->argv[i].value << '\n';
    // if (info->help)
    //     std::clog << "Usage:\n" << info->help << '\n';
    // std::clog << "GCC config info: " << gccver->configuration_arguments << '\n';
    register_callback(info->base_name, PLUGIN_INFO, nullptr, &vapor_info);
    static register_pass_info pi{
        .pass = new vapor_pass(g),
        .reference_pass_name = "lower", //"cfg",
        .ref_pass_instance_number = 1,
        .pos_op = PASS_POS_INSERT_BEFORE
    };
    // register_callback(info->base_name, PLUGIN_PASS_MANAGER_SETUP, nullptr, &pi);
    // register_callback(info->base_name, PLUGIN_PASS_EXECUTION, [](auto, auto) { std::cerr << "[pass " << current_pass->name << "]\n"; }, nullptr);
    if (false)
    register_callback(info->base_name, PLUGIN_FINISH_TYPE, [](auto d, auto) {
        auto t = (tree) d;
        if (t != error_mark_node) {
            t = TYPE_NAME(t);
            // std::cerr << "finished type " << scope(t) << DECL_NAME_STR(t) << "\n";
            if (scope(t) + DECL_NAME_STR(t) == "std::__n4861::coroutine_handle") {
                std::cerr << "std coro:\n";
                // walk_rec(TREE_TYPE(t), " ");
                std::clog << "modify\n";
                auto ht = TREE_TYPE(t);
                auto first = TYPE_FIELDS(ht);
                // debug_tree(first);
                // debug_tree(TREE_VALUE(TREE_CHAIN(TREE_CHAIN(TYPE_ARG_TYPES(TREE_TYPE(first))))));
                auto tl = build_tree_list(NULL_TREE, void_type_node);
                // debug_tree(tl);
                auto mm = build_method_type_directly(ht, void_type_node, tl);
                // debug_tree(mm);
                auto my = build_decl(DECL_SOURCE_LOCATION(t), FUNCTION_DECL, get_identifier("seri"), mm);
                // debug_tree(my);
                //TODO
                //  public abstract external in_system_header autoinline decl_3 decl_8 QI /usr/include/c++/12/coroutine:195:12 align:16 warn_if_not_align:0 context <record_type 0x7f95b9c2dbd0 coroutine_handle>
                //  full-name "constexpr std::__n4861::coroutine_handle<ret::promise_type>::~coroutine_handle() noexcept (<uninstantiated>)"
                //  not-really-extern ...
                TREE_CHAIN(my) = first;
                TYPE_FIELDS(ht) = my;
                // debug_tree(ht);
                walk_rec(ht, " ");
            }
        }
    }, nullptr);
    register_callback(info->base_name, PLUGIN_PRE_GENERICIZE, [](auto t, auto) { collect_coro(tree(t)); }, nullptr);
    register_callback (info->base_name, PLUGIN_OVERRIDE_GATE, [](auto, auto) {
        static bool once = false;
        if (once || errorcount || sorrycount)
            return;
        once = true;
        // std::clog << "decl:s in " << main_input_filename << '\n';
        // walk(global_namespace);
        patch_seri();
    }, nullptr);
    return 0;
}

/*
PLUGIN_OVERRIDE_GATE

[pass *warn_unused_result]
[pass *diagnose_omp_blocks]
[pass *diagnose_tm_blocks]
[pass omp_oacc_kernels_decompose]
[pass omplower]
[pass lower]
[pass tmlower]
[pass ehopt]
[pass eh]
[pass coro-lower-builtins]
[pass cfg]
[pass vapor]
function f ../toy.cc:3
 [gimple code 4]
 [gimple code 10]
[pass *warn_function_return]
[pass coro-early-expand-ifns]
[pass ompexp]
[pass *build_cgraph_edges]
.. same for other funcs

PLUGIN_PASS_EXECUTION

[pass *warn_unused_result]
[pass omplower]
[pass lower]
[pass ehopt]
[pass eh]
[pass cfg]
[pass vapor]
function f ../toy.cc:3
 [gimple code 4]
 [gimple code 10]
[pass *warn_function_return]
[pass ompexp]
[pass *build_cgraph_edges]

-fdump-passes

   *warn_unused_result                                 :  ON
   *diagnose_omp_blocks                                :  OFF
   *diagnose_tm_blocks                                 :  OFF
   tree-omp_oacc_kernels_decompose                     :  OFF
   tree-omplower                                       :  ON
   tree-lower                                          :  ON
   tree-tmlower                                        :  OFF
   tree-ehopt                                          :  ON
   tree-eh                                             :  ON
   tree-coro-lower-builtins                            :  ON
   tree-cfg                                            :  ON
   *warn_function_return                               :  ON
   tree-coro-early-expand-ifns                         :  OFF
   tree-ompexp                                         :  ON
   *build_cgraph_edges                                 :  ON
   *free_lang_data                                     :  ON
   ipa-visibility                                      :  ON
   ipa-build_ssa_passes                                :  ON
      tree-fixup_cfg1                                  :  ON
      tree-ssa                                         :  ON
      tree-walloca1                                    :  ON
      tree-warn-printf                                 :  ON
      *nonnullcmp                                      :  ON
      tree-early_uninit                                :  ON
      tree-waccess1                                    :  ON
      tree-ubsan                                       :  OFF
      tree-nothrow                                     :  OFF
      *rebuild_cgraph_edges                            :  ON
   ipa-opt_local_passes                                :  ON
      tree-fixup_cfg2                                  :  ON
      *rebuild_cgraph_edges                            :  ON
      tree-local-fnsummary1                            :  ON
      tree-einline                                     :  ON
      *infinite-recursion                              :  ON
      tree-early_optimizations                         :  OFF
         *remove_cgraph_callee_edges                   :  ON
         tree-early_objsz                              :  ON
         tree-ccp1                                     :  OFF
         tree-forwprop1                                :  ON
         tree-ethread                                  :  OFF
         tree-esra                                     :  OFF
         tree-ealias                                   :  OFF
         tree-fre1                                     :  OFF
         tree-evrp                                     :  OFF
         tree-mergephi1                                :  ON
         tree-dse1                                     :  OFF
         tree-cddce1                                   :  OFF
         tree-phiopt1                                  :  OFF
         tree-tailr1                                   :  OFF
         tree-iftoswitch                               :  ON
         tree-switchconv                               :  OFF
         tree-ehcleanup1                               :  OFF
         tree-profile_estimate                         :  OFF
         tree-local-pure-const1                        :  OFF
         tree-modref1                                  :  OFF
         tree-fnsplit                                  :  OFF
         *strip_predict_hints                          :  ON
      tree-release_ssa                                 :  ON
      *rebuild_cgraph_edges                            :  ON
      tree-local-fnsummary2                            :  ON
   ipa-remove_symbols                                  :  ON
   ipa-ipa_oacc                                        :  OFF
      ipa-pta1                                         :  OFF
      ipa-ipa_oacc_kernels                             :  ON
         tree-oacc_kernels                             :  OFF
            tree-ch1                                   :  OFF
            tree-fre2                                  :  OFF
            tree-lim1                                  :  ON
            tree-dom1                                  :  OFF
            tree-dce1                                  :  OFF
            tree-parloops1                             :  OFF
            tree-ompexpssa1                            :  ON
            *rebuild_cgraph_edges                      :  ON
   ipa-targetclone                                     :  ON
   ipa-afdo                                            :  OFF
   ipa-profile                                         :  OFF
      tree-feedback_fnsplit                            :  OFF
   ipa-free-fnsummary1                                 :  ON
   ipa-increase_alignment                              :  OFF
   ipa-tmipa                                           :  OFF
   ipa-emutls                                          :  OFF
   ipa-analyzer                                        :  OFF
   ipa-odr                                             :  OFF
   ipa-whole-program                                   :  ON
   ipa-profile_estimate                                :  OFF
   ipa-icf                                             :  OFF
   ipa-devirt                                          :  OFF
   ipa-cp                                              :  OFF
   ipa-sra                                             :  OFF
   ipa-cdtor                                           :  OFF
   ipa-fnsummary                                       :  ON
   ipa-inline                                          :  ON
   ipa-pure-const                                      :  OFF
   ipa-modref                                          :  ON
   ipa-free-fnsummary2                                 :  ON
   ipa-static-var                                      :  OFF
   ipa-single-use                                      :  ON
   ipa-comdats                                         :  ON
   ipa-pta2                                            :  OFF
   ipa-simdclone                                       :  ON
   tree-fixup_cfg3                                     :  ON
   tree-ehdisp                                         :  OFF
   tree-oaccloops                                      :  OFF
   tree-omp_oacc_neuter_broadcast                      :  OFF
   tree-oaccdevlow                                     :  OFF
   tree-ompdevlow                                      :  ON
   tree-omptargetlink                                  :  OFF
   tree-adjust_alignment                               :  ON
   *all_optimizations                                  :  OFF
      *remove_cgraph_callee_edges                      :  ON
      *strip_predict_hints                             :  ON
      tree-ccp2                                        :  OFF
      tree-objsz1                                      :  ON
      tree-post_ipa_warn1                              :  ON
      tree-waccess2                                    :  ON
      tree-cunrolli                                    :  OFF
      tree-backprop                                    :  ON
      tree-phiprop                                     :  ON
      tree-forwprop2                                   :  ON
      tree-alias                                       :  OFF
      tree-retslot                                     :  ON
      tree-fre3                                        :  OFF
      tree-mergephi2                                   :  ON
      tree-threadfull1                                 :  OFF
      tree-vrp1                                        :  OFF
      tree-dse2                                        :  OFF
      tree-dce2                                        :  OFF
      tree-stdarg                                      :  ON
      tree-cdce                                        :  OFF
      tree-cselim                                      :  ON
      tree-copyprop1                                   :  OFF
      tree-ifcombine                                   :  ON
      tree-mergephi3                                   :  ON
      tree-phiopt2                                     :  OFF
      tree-tailr2                                      :  OFF
      tree-ch2                                         :  OFF
      tree-cplxlower1                                  :  ON
      tree-sra                                         :  OFF
      tree-thread1                                     :  OFF
      tree-dom2                                        :  OFF
      tree-copyprop2                                   :  OFF
      tree-isolate-paths                               :  OFF
      tree-reassoc1                                    :  ON
      tree-dce3                                        :  OFF
      tree-forwprop3                                   :  ON
      tree-phiopt3                                     :  OFF
      tree-ccp3                                        :  OFF
      tree-sincos                                      :  OFF
      tree-bswap                                       :  OFF
      tree-laddress                                    :  OFF
      tree-lim2                                        :  ON
      tree-walloca2                                    :  ON
      tree-pre                                         :  OFF
      tree-sink1                                       :  OFF
      tree-sancov1                                     :  OFF
      tree-asan1                                       :  OFF
      tree-tsan1                                       :  OFF
      tree-dse3                                        :  OFF
      tree-dce4                                        :  OFF
      tree-fix_loops                                   :  ON
      tree-loop                                        :  ON
         tree-loopinit                                 :  ON
         tree-unswitch                                 :  OFF
         tree-sccp                                     :  ON
         tree-lsplit                                   :  OFF
         tree-lversion                                 :  OFF
         tree-unrolljam                                :  OFF
         tree-cddce2                                   :  OFF
         tree-ivcanon                                  :  ON
         tree-ldist                                    :  OFF
         tree-linterchange                             :  OFF
         tree-copyprop3                                :  OFF
         tree-graphite0                                :  OFF
            tree-graphite                              :  OFF
            tree-lim3                                  :  ON
            tree-copyprop4                             :  OFF
            tree-dce5                                  :  OFF
         tree-parloops2                                :  OFF
         tree-ompexpssa2                               :  ON
         tree-ch_vect                                  :  OFF
         tree-ifcvt                                    :  OFF
         tree-vect                                     :  OFF
            tree-dce6                                  :  OFF
         tree-pcom                                     :  OFF
         tree-cunroll                                  :  ON
         *pre_slp_scalar_cleanup                       :  OFF
            tree-fre4                                  :  OFF
            tree-dse4                                  :  OFF
         tree-slp1                                     :  OFF
         tree-aprefetch                                :  OFF
         tree-ivopts                                   :  ON
         tree-lim4                                     :  ON
         tree-loopdone                                 :  ON
      tree-no_loop                                     :  OFF
         tree-slp2                                     :  OFF
      tree-simduid1                                    :  OFF
      tree-veclower21                                  :  ON
      tree-switchlower1                                :  ON
      tree-recip                                       :  OFF
      tree-reassoc2                                    :  ON
      tree-slsr                                        :  OFF
      tree-split-paths                                 :  OFF
      tree-tracer                                      :  OFF
      tree-fre5                                        :  OFF
      tree-thread2                                     :  OFF
      tree-dom3                                        :  OFF
      tree-strlen1                                     :  OFF
      tree-threadfull2                                 :  OFF
      tree-vrp2                                        :  OFF
      tree-ccp4                                        :  OFF
      tree-wrestrict                                   :  ON
      tree-dse5                                        :  OFF
      tree-cddce3                                      :  OFF
      tree-forwprop4                                   :  ON
      tree-sink2                                       :  OFF
      tree-phiopt4                                     :  OFF
      tree-fab1                                        :  ON
      tree-widening_mul                                :  OFF
      tree-store-merging                               :  OFF
      tree-tailc                                       :  OFF
      tree-dce7                                        :  OFF
      tree-crited1                                     :  ON
      tree-uninit1                                     :  ON
      tree-local-pure-const2                           :  OFF
      tree-modref2                                     :  OFF
      tree-uncprop1                                    :  OFF
   *all_optimizations_g                                :  OFF
      *remove_cgraph_callee_edges                      :  ON
      *strip_predict_hints                             :  ON
      tree-cplxlower2                                  :  ON
      tree-veclower22                                  :  ON
      tree-switchlower2                                :  ON
      tree-ccp5                                        :  OFF
      tree-post_ipa_warn2                              :  ON
      tree-objsz2                                      :  ON
      tree-fab2                                        :  ON
      tree-strlen2                                     :  OFF
      tree-copyprop5                                   :  OFF
      tree-dce8                                        :  OFF
      tree-sancov2                                     :  OFF
      tree-asan2                                       :  OFF
      tree-tsan2                                       :  OFF
      tree-crited2                                     :  ON
      tree-uninit2                                     :  ON
      tree-uncprop2                                    :  OFF
   *tminit                                             :  OFF
      tree-tmmark                                      :  ON
      tree-tmmemopt                                    :  OFF
      tree-tmedge                                      :  ON
   tree-simduid2                                       :  OFF
   tree-vtable-verify                                  :  OFF
   tree-lower_vaarg                                    :  ON
   tree-veclower                                       :  ON
   tree-cplxlower0                                     :  ON
   tree-sancov_O0                                      :  OFF
   tree-switchlower_O0                                 :  ON
   tree-asan0                                          :  OFF
   tree-tsan0                                          :  OFF
   tree-sanopt                                         :  OFF
   tree-ehcleanup2                                     :  OFF
   tree-resx                                           :  ON
   tree-nrv                                            :  OFF
   tree-isel                                           :  ON
   tree-hardcbr                                        :  OFF
   tree-hardcmp                                        :  OFF
   tree-waccess3                                       :  ON
   tree-optimized                                      :  ON
   *warn_function_noreturn                             :  OFF
   rtl-expand                                          :  ON
   *rest_of_compilation                                :  ON
      rtl-vregs                                        :  ON
      rtl-into_cfglayout                               :  ON
      rtl-jump                                         :  ON
      rtl-subreg1                                      :  OFF
      rtl-dfinit                                       :  OFF
      rtl-cse1                                         :  OFF
      rtl-fwprop1                                      :  OFF
      rtl-cprop1                                       :  OFF
      rtl-rtl pre                                      :  OFF
      rtl-hoist                                        :  OFF
      rtl-cprop2                                       :  OFF
      rtl-store_motion                                 :  OFF
      rtl-cse_local                                    :  OFF
      rtl-ce1                                          :  OFF
      rtl-reginfo                                      :  ON
      rtl-loop2                                        :  OFF
         rtl-loop2_init                                :  ON
         rtl-loop2_invariant                           :  OFF
         rtl-loop2_unroll                              :  OFF
         rtl-loop2_doloop                              :  OFF
         rtl-loop2_done                                :  ON
      rtl-subreg2                                      :  OFF
      rtl-web                                          :  OFF
      rtl-cprop3                                       :  OFF
      rtl-stv1                                         :  OFF
      rtl-cse2                                         :  OFF
      rtl-dse1                                         :  OFF
      rtl-fwprop2                                      :  OFF
      rtl-auto_inc_dec                                 :  OFF
      rtl-init-regs                                    :  OFF
      rtl-ud_dce                                       :  OFF
      rtl-combine                                      :  OFF
      rtl-rpad                                         :  OFF
      rtl-stv2                                         :  OFF
      rtl-ce2                                          :  OFF
      rtl-jump_after_combine                           :  OFF
      rtl-bbpart                                       :  OFF
      rtl-outof_cfglayout                              :  ON
      rtl-split1                                       :  ON
      rtl-subreg3                                      :  OFF
      rtl-no-opt dfinit                                :  ON
      *stack_ptr_mod                                   :  ON
      rtl-mode_sw                                      :  ON
      rtl-asmcons                                      :  ON
      rtl-sms                                          :  OFF
      rtl-lr_shrinkage                                 :  OFF
      rtl-sched1                                       :  OFF
      rtl-early_remat                                  :  OFF
      rtl-ira                                          :  ON
      rtl-reload                                       :  ON
      rtl-vzeroupper                                   :  OFF
      *all-postreload                                  :  OFF
         rtl-postreload                                :  OFF
         rtl-gcse2                                     :  OFF
         rtl-split2                                    :  OFF
         rtl-ree                                       :  OFF
         rtl-cmpelim                                   :  OFF
         rtl-pro_and_epilogue                          :  ON
         rtl-dse2                                      :  OFF
         rtl-csa                                       :  OFF
         rtl-jump2                                     :  ON
         rtl-compgotos                                 :  OFF
         rtl-sched_fusion                              :  OFF
         rtl-peephole2                                 :  OFF
         rtl-ce3                                       :  OFF
         rtl-rnreg                                     :  OFF
         rtl-cprop_hardreg                             :  OFF
         rtl-rtl_dce                                   :  OFF
         rtl-bbro                                      :  OFF
         *leaf_regs                                    :  ON
         rtl-split3                                    :  OFF
         rtl-sched2                                    :  OFF
         *stack_regs                                   :  ON
            rtl-split4                                 :  ON
            rtl-stack                                  :  ON
      *all-late_compilation                            :  OFF
         rtl-zero_call_used_regs                       :  ON
         rtl-alignments                                :  ON
         rtl-vartrack                                  :  OFF
         *free_cfg                                     :  ON
         rtl-mach                                      :  ON
         rtl-barriers                                  :  ON
         rtl-dbr                                       :  OFF
         rtl-split5                                    :  OFF
         rtl-eh_ranges                                 :  OFF
         rtl-endbr_and_patchable_area                  :  OFF
         rtl-shorten                                   :  ON
         rtl-nothrow                                   :  ON
         rtl-dwarf2                                    :  ON
         rtl-final                                     :  ON
      rtl-dfinish                                      :  ON
   *clean_state                                        :  ON

*/