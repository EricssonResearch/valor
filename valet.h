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
    AttrHandling handleDeclAttribute(Sema& s, Decl* d, ParsedAttr const&) const override { skip.emplace(d); return AttributeApplied; }
    static std::unordered_set<Decl const*> skip; // dance around custom attributes not being clang::Attr so we don't get them in hasAttr*()
};

AST_MATCHER(CoroutineBodyStmt, coro_body) { return true; }
AST_MATCHER(Decl, valor_skip) { return skip_attr::skip.count(&Node); }

#pragma GCC diagnostic pop

struct valet : SyntaxOnlyAction, MatchFinder, SourceFileCallbacks, private MatchFinder::MatchCallback, private Rewriter {
    // struct visitor : ASTConsumer, RecursiveASTVisitor<visitor> {
    //     explicit visitor(ASTContext& ctx) : ctx(ctx) {}
    //     virtual void HandleTranslationUnit(ASTContext&) override { /*assert(&c == &ctx);*/ TraverseAST(ctx); /*TraverseDecl(ctx.getTranslationUnitDecl());*/ }
    //     bool VisitAttr(auto a) { std::clog << "attr: " << (a->getAttrName() ? a->getAttrName()->getName() : "<unnamed>") << '\n'; return true; }
    //     bool VisitAttr(AssumptionAttr* a) { std::clog << "aatr: " << a->getAssumption() << '\n'; return true; }
    //     // bool VisitStmt(auto s) { std::clog << "stmt: " << s->getStmtClassName() << '\n'; return true; }
    //     bool VisitType(auto t) { std::clog << "type: " << t->getTypeClassName() << '\n'; return true; }
    //     bool VisitTypeLoc(auto) { std::clog << "tloc: " << "l->...()" << '\n'; return true; }
    //     bool VisitQualifiedTypeLoc(auto) { std::clog << "qloc: " << "l->...()" << '\n'; return true; }
    //     bool VisitUnqualTypeLoc(auto) { std::clog << "uloc: " << "l->...()" << '\n'; return true; }
    //     bool VisitDecl(auto d) {
    //         std::clog << "decl: " << d->getDeclKindName() << '\n';
    //         // d->dump();
    //         return true;
    //     }
    //     bool VisitDecl(FunctionDecl* f) {
    //         std::clog << "fdcl: " << f->getName() << '\n';
    //         // if (f->hasAttr<attr::Assumption>())
    //         if (f->getAttr<AssumptionAttr>())
    //             std::clog << " !ass\n";
    //         return true;
    //     }
    //     bool VisitCXXRecordDecl(auto decl) {
    //         std::clog << " n: " << decl->getName() << " qn: " << decl->getQualifiedNameAsString() << '\n';
    //         // decl->dump();
    //         if (decl->getQualifiedNameAsString() == "X")
    //             if (auto l = ctx.getFullLoc(decl->getBeginLoc()); l.isValid())
    //                 std::clog << "Found declaration at " << l.getSpellingLineNumber() << ':' << l.getSpellingColumnNumber() << '\n';
    //         return true;
    //     }
    //     ASTContext& ctx;
    // };
    // std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& cc, llvm::StringRef) override { return std::make_unique<visitor>(cc.getASTContext()); }
    valet() {
        addMatcher(
            traverse(TK_IgnoreUnlessSpelledInSource, // Optimization; skip it if misses implicit stmts (not expected for coro)
                functionDecl(
                    has(coro_body()), // Use hasDescendant() if not direct child --> is this possible?
                    unless(valor_skip()),
                    hasDescendant(findAll(coawaitExpr().bind("await"))),// TODO ?DependentCoawaitExpr ?co_return
                    optionally(findAll(varDecl().bind("var")))
                    // If I was clever maybe I could fold in here matchers for the scopes of 'aw' and 'var'
                    // but those are left for the callback below.
                ).bind("f")
            ), this);
    }
    bool handleBeginSource(CompilerInstance& cc) override { setSourceMgr(cc.getSourceManager(), cc.getLangOpts()); return true; }
    void handleEndSource() override {
        rewrite();
        auto& sm = getSourceMgr();
        for (auto i = buffer_begin(); i != buffer_end(); ++i) {// alternative is overwriteChangedFiles() -- we should only have one file here BTW
            fs::path p(sm.getFileEntryForID(i->first)->tryGetRealPathName().str());
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
            f->dump();
            INFO << "Valet patching " << f->getNameInfo().getName().getAsString() << " at " << sm.getFilename(f->getLocation()) << ':' << sm.getSpellingLineNumber(f->getBeginLoc());
            assert(!f->hasTrivialBody());
            auto loc = f->getBody()->getBeginLoc();
            auto lineno = [&] { return sm.getSpellingLineNumber(loc); };
            // TODO keep indent after insertions
            if (InsertTextAfterToken(loc, preamble(ss.size(), lineno())))
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
    static std::string preamble(size_t labels, size_t lineno) {
        assert(labels);// otherwise 'f' shouldn't have been matched
        std::string ret = // TODO ser_ctx<T>
R"..(
// Valet preamble
auto _valet_p = co_await ser_ctx<int>::get_promise{};
if (auto _valet_st = _valet_p->stage()) {
    assert(_valet_st <= ).." + std::to_string(labels) + R"..();
    // std::cerr << "k: to stage " << _valet_st << '\n';
    static ssize_t _valet_stages[] = {0)..";
        for (size_t i = 1; i <= labels; ++i)
            ret += R"..(, (char*)&&_valet_s).." + std::to_string(i) + R"..( - (char*)&&_valet_s0)..";
        ret +=
R"..(};
    goto *(void*)((char*)&&_valet_s0 + _valet_stages[_valet_st]);
}
_valet_s0:
#line ).." + std::to_string(lineno) + R"..( // End of Valet preamble
)..";
        return ret;
    }
    static std::string save(size_t stage, std::string_view varlist, size_t lineno) {
        // return std::format("\n_valet_p->save({}{});// Valet stage save\n#line {}\n", stage, varlist, lineno); // TODO soon, soon, 13
        return "\n// Valet stage save\n_valet_p->save(" + std::to_string(stage) + std::string(varlist) + ");\n#line " + std::to_string(lineno) + " // End of Valet save\n";
    }
    static std::string load(size_t stage, std::string_view varlist, size_t lineno) {
        return "\n// Valet stage load\n_valet_s" + std::to_string(stage) + ":\n_valet_p->load(" + std::string(varlist) + ");\n#line " + std::to_string(lineno) + " // End of Valet load\n";
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