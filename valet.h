#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter" // TODO -Wall
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#include <clang/AST/RecursiveASTVisitor.h>
#undef PACKAGE
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#pragma GCC diagnostic pop
#include <iostream>

std::ostream& operator<<(std::ostream& o, llvm::StringRef s) { return o << std::string_view(s); }

namespace valor {

using namespace clang;

struct valet : SyntaxOnlyAction {
    struct visitor : ASTConsumer, RecursiveASTVisitor<visitor> {
        explicit visitor(ASTContext& ctx) : ctx(ctx) {}
        virtual void HandleTranslationUnit(ASTContext&) override { /*assert(&c == &ctx);*/ TraverseAST(ctx); /*TraverseDecl(ctx.getTranslationUnitDecl());*/ }
        bool VisitAttr(auto a) { std::clog << "attr: " << (a->getAttrName() ? a->getAttrName()->getName() : "<unnamed>") << '\n'; return true; }
        bool VisitStmt(auto s) { std::clog << "stmt: " << s->getStmtClassName() << '\n'; return true; }
        bool VisitType(auto t) { std::clog << "type: " << t->getTypeClassName() << '\n'; return true; }
        bool VisitTypeLoc(auto l) { std::clog << "tloc: " << "l->...()" << '\n'; return true; }
        bool VisitQualifiedTypeLoc(auto l) { std::clog << "qloc: " << "l->...()" << '\n'; return true; }
        bool VisitUnqualTypeLoc(auto l) { std::clog << "uloc: " << "l->...()" << '\n'; return true; }
        bool VisitDecl(auto d) {
            std::clog << "decl: " << d->getDeclKindName() << '\n';
            d->dump();
            return true;
        }
        bool VisitCXXRecordDecl(auto decl) {
            std::clog << " n: " << decl->getName() << " qn: " << decl->getQualifiedNameAsString() << '\n';
            decl->dump();
            if (decl->getQualifiedNameAsString() == "X")
                if (auto l = ctx.getFullLoc(decl->getBeginLoc()); l.isValid())
                    std::clog << "Found declaration at " << l.getSpellingLineNumber() << ':' << l.getSpellingColumnNumber() << '\n';
            return true;
        }
        ASTContext& ctx;
    };
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& cc, llvm::StringRef) override { return std::make_unique<visitor>(cc.getASTContext()); }
};

}