/** This file is part of the Valor project which is released under the MIT license.
 * See file COPYING for full license details.
 * Copyright 2024 Ericsson AB
 */
#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter" // TODO -Wall
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#undef PACKAGE
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Sema/ParsedAttr.h>
#include <clang/Sema/Sema.h>
#include <clang/Sema/SemaDiagnostic.h>
#include <clang/Tooling/Tooling.h>
#pragma GCC diagnostic pop
#include <llvm/Support/raw_os_ostream.h>
#include <filesystem>
#include <numeric>
#include <unordered_set>

inline std::ostream& operator<<(std::ostream& o, llvm::StringRef s) { return o << std::string_view(s); }
#include "log.h"

namespace valor {

using namespace clang;
using namespace ast_matchers;
using namespace tooling;

namespace fs = std::filesystem;
using namespace std::string_literals;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

struct skip_attr : ParsedAttrInfo {
    skip_attr() {
        OptArgs = 1;
        static constexpr Spelling s[] = {{ParsedAttr::AS_GNU, "valor_skip"}, {ParsedAttr::AS_CXX11, "valor_skip"}};
        Spellings = s;
    }
    bool diagAppertainsToDecl(Sema& s, ParsedAttr const& a, Decl const* d) const override {
        if (isa<FunctionDecl>(d))
            return true;
        //TODO or local var
        s.Diag(a.getLoc(), diag::warn_attribute_wrong_decl_type_str) << a << "functions or local variables";
        return false;
    }
    bool diagAppertainsToStmt(Sema&, ParsedAttr const&, Stmt const*) const override { return false; }
    AttrHandling handleDeclAttribute(Sema&, Decl* d, ParsedAttr const&) const override { skip.emplace(d); return AttributeApplied; }
    static std::unordered_set<Decl const*> skip; // dance around custom attributes not being clang::Attr so we don't get them in hasAttr*()
};

AST_MATCHER(CoroutineBodyStmt, coro_body) { return true; }
AST_MATCHER(Decl, valor_skip) { return skip_attr::skip.count(&Node); }
AST_MATCHER(QualType, valor_ser_ctx) { return Node.getBaseTypeIdentifier()->getName() == "ser_ctx"; }

#pragma GCC diagnostic pop

struct valet : SyntaxOnlyAction, MatchFinder, SourceFileCallbacks, private MatchFinder::MatchCallback, private Rewriter {
    std::ostream* out = nullptr;///< Write output here, presumably from a single file (default: write each file one with '_valet' appended before the extension)
    bool verbose = false;///< Add 'Valet was here' comments in rewrites (default: off)
    valet() {
        addMatcher(//TODO valor::ser_ctx defined TODO *templated* coro funcs
            traverse(TK_IgnoreUnlessSpelledInSource, // Optimization; skip it if misses implicit stmts (not expected for coro)
                functionDecl(
                    has(coro_body()), // Use hasDescendant() if not direct child --> is this possible?
                    // TODO what if derived from? returns(valor_ser_ctx()),// TODO can we get the *name* of a retval? 'returns(asString(".."))' didn't work // TODO CXXBaseSpec?
                    unless(valor_skip()),
                    hasDescendant(findAll(coawaitExpr().bind("await"))),// TODO ?DependentCoawaitExpr ?co_return
                    optionally(findAll(varDecl().bind("var")))
                    // If I was clever maybe I could fold in here matchers for the scopes of 'aw' and 'var'
                    // but those are left for the callback below.
                ).bind("f")
            ), this);
    }
    bool handleBeginSource(CompilerInstance& cc) override {
        if (!verbose && cc.hasDiagnostics())
            cc.getDiagnostics().setSuppressAllDiagnostics(true);
        auto& sm = cc.getSourceManager();
        DEBUG << "Valet on " << sm.getFileEntryForID(sm.getMainFileID())->getName();
        setSourceMgr(sm, cc.getLangOpts());
        skip_attr::skip.clear();// would be better as a member but how to register that?
        return true;
    }
    void handleEndSource() override {
        rewrite();
        for (auto i = buffer_begin(); i != buffer_end(); ++i)// alternative is overwriteChangedFiles() -- we should only have at most one file here BTW
            if (out) {
                llvm::raw_os_ostream o(*out);
                i->second.write(o);
            } else {
                fs::path p(getSourceMgr().getFileEntryForID(i->first)->tryGetRealPathName().str());
                auto ex = p.extension();
                p = (p.replace_extension() += "_valet") += ex;
                DEBUG << " write to " << p;
                std::error_code ec;
                llvm::raw_fd_ostream o(p.native(), ec);
                if (ec)
                    bail("TODO problem opening " + p.string() + ": " + ec.category().name() + " [" + std::to_string(ec.value()) + ']');
                i->second.write(o);
            }
    }
    void run(MatchFinder::MatchResult const& res) override {
        auto aw = res.Nodes.getNodeAs<CoawaitExpr>("await");
        auto& stage = ensure_stage_for(res.Nodes.getNodeAs<FunctionDecl>("f"), aw, *res.Context);
        if (auto var = res.Nodes.getNodeAs<VarDecl>("var");
                var && var->getBeginLoc() < aw->getBeginLoc()
                && (isa<ParmVarDecl>(var) || stage.scope.count(block_of(var, *res.Context)))) // TODO ParmVarDecl references?
            stage.vars.emplace_back(var);
    }
    ~valet() { if (out) out->flush(); }
    /// Prepend necessary compiler options to the supplied list, so that 'args' takes precedence.
    /// Creates a copy of 'args'.
    auto cc_opts(CommandLineArguments const& args = {}) const {
        auto ret = args;
        auto it = ret.emplace(ret.begin() + !ret.empty()/*argv[0] is the tool, if any*/, "-std=c++20");
        if (verbose)
            ret.emplace(it + 1, "-v");
        return ret;
    }
private:
    /// Per-coawait rewrite info
    struct stage {
        CoawaitExpr const* aw;
        std::unordered_set<CompoundStmt const*> scope;
        std::vector<VarDecl const*> vars;
    };
    std::unordered_map<FunctionDecl const*, std::vector<stage>> stages;// order of stages only important for matching labels to jump table entries
    void rewrite() {
        auto& sm = getSourceMgr();
        for (auto&& [f,ss]: stages) {
            // f->dump();
            INFO << "Valet patching " << f->getNameInfo().getName().getAsString() << " at " << sm.getFilename(f->getLocation()) << ':' << sm.getSpellingLineNumber(f->getBeginLoc());
            assert(!f->hasTrivialBody());
            auto loc = f->getBody()->getBeginLoc();
            auto lineno = [&] { return sm.getSpellingLineNumber(loc); };
            // TODO keep indent after insertions
            if (InsertTextAfterToken(loc, preamble(f->getDeclaredReturnType().getAsString(), ss.size(), lineno())))//TODO pre: get full qualif name then skip 'valor::' below
                bail("preamble");
            for (size_t n = 0; auto& st: ss) {
                ++n;
                auto vars = std::accumulate(st.vars.begin(), st.vars.end(), std::string(), [](auto s, auto v) { return s + ", " + v->getNameAsString(); });
                std::string_view vars_strip(vars.size() ? vars.data() + 2 : "");
                INFO << " stage #" << n << (vars.size() ? " with variables: " + std::string(vars_strip) : "");
                loc = st.aw->getBeginLoc();
                if (InsertTextAfter(loc, save(n, vars, lineno())))
                    bail("await save " + std::to_string(n));
                loc = semi_after(st.aw->getEndLoc());
                if (InsertTextAfterToken(loc, load(n, vars_strip, lineno())))
                    bail("await load " + std::to_string(n));
            }
            // TODO remove attr + don't match for valor_skip in valet()
        }
        stages.clear();
    }
    std::string preamble(std::string ctx_type, size_t labels, size_t lineno) {
        assert(labels);// otherwise 'f' shouldn't have been matched
        auto ret = (verbose ? "// Valet preamble\n"s : "") +
R"(
auto _valet_p = co_await valor::)" + ctx_type + R"..(::get_promise{};
if (auto _valet_st = _valet_p->stage()) {
    assert(_valet_st <= ).." + std::to_string(labels) + R"..();
    // std::cerr << "k: to stage " << _valet_st << '\n';
    static ssize_t _valet_stages[] = {0)..";
        for (size_t i = 1; i <= labels; ++i)
            ret += ", (char*)&&_valet_s" + std::to_string(i) + " - (char*)&&_valet_s0";
        return ret +
R"..(};
    goto *(void*)((char*)&&_valet_s0 + _valet_stages[_valet_st]);
}
_valet_s0:
#line ).." + std::to_string(lineno)
            + (verbose ? " // End of Valet preamble" : "") + '\n';
    }
    std::string save(size_t stage, std::string_view varlist, size_t lineno) {
        // return std::format("\n_valet_p->save({}{});// Valet stage save\n#line {}\n", stage, varlist, lineno); // TODO soon, soon, 13
        return (verbose ? "\n// Valet stage save"s : "") + "\n_valet_p->save("
            + std::to_string(stage) + std::string(varlist) + ");\n#line " + std::to_string(lineno)
            + (verbose ? " // End of Valet save" : "") + '\n';
    }
    std::string load(size_t stage, std::string_view varlist, size_t lineno) {
        return (verbose ? "\n// Valet stage load"s : "") + "\n_valet_s"
            + std::to_string(stage) + ":\n_valet_p->load(" + std::string(varlist) + ");\n#line " + std::to_string(lineno)
            + (verbose ? " // End of Valet load" : "") + '\n';
    }
    /// Get or create the stage helper at 'aw' in function 'f'
    stage& ensure_stage_for(FunctionDecl const* f, CoawaitExpr const* aw, ASTContext& ctx) {
        auto& v = stages[f];
        for (auto&& i: v)
            if (i.aw == aw)
                return i;
        auto& sc = v.emplace_back(aw).scope;// Add an entry for 'f' with the first stage
        // then traverse f for compounds until one that has aw. (Coro body inside f would be enough but meh.)
        for (auto&& i: ast_matchers::match(decl(hasDescendant(findAll(compoundStmt(hasDescendant(equalsNode(aw))).bind("c")))), *f, ctx))
            sc.emplace(i.getNodeAs<CompoundStmt>("c"));// collect such compounds as aw's scope
        return v.back();
    }
    /// Get declaring compound of a var
    CompoundStmt const* block_of(VarDecl const* v, ASTContext& ctx) {
        return ast_matchers::match(
            decl(hasAncestor(compoundStmt().bind("c"))),//TODO hasAncestor() returns the 1st? otherwise check parent-of-parent is a declstmt (is it always? for/ifstmt?)
            *v,
            ctx
        ).front().getNodeAs<CompoundStmt>("c");
    }
    [[noreturn]] void bail(std::string_view e) {
        CRIT << "Bailing out: " << e;// -fno-exceptions
        exit(2);
    }
    SourceLocation semi_after(SourceLocation loc) {
        // adapted from findSemiAfterLocation()
        auto& sm = getSourceMgr();
        assert(!loc.isMacroID());
        loc = Lexer::getLocForEndOfToken(loc, 0, sm, getLangOpts());
        auto [fid, off] = sm.getDecomposedLoc(loc);
        bool bad = false;
        auto file = sm.getBufferData(fid, &bad);
        if (bad)
            bail("load file");//TODO ret invalid?
        auto begin = file.data() + off;
        Lexer lexer(sm.getLocForStartOfFile(fid), getLangOpts(), file.begin(), begin, file.end());
        Token tok;
        lexer.LexFromRawLexer(tok);
        if (tok.isNot(tok::semi))
            return semi_after(tok.getLocation());// correct? can it be followed by other tokens?
        return tok.getLocation();
    }
};

}