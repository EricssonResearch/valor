#include "valet.h"
#include <clang/Tooling/CommonOptionsParser.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <clang/Tooling/Tooling.h>
#pragma GCC diagnostic pop

using namespace llvm::cl;
using namespace clang;
using namespace tooling;
using namespace valor;

OptionCategory opt_cat("Valet options");
extrahelp help0(CommonOptionsParser::HelpMessage);
extrahelp help("\nValet help...\n");

int main(int argc, const char** argv) {
    if (auto p = CommonOptionsParser::create(argc, argv, opt_cat)) {
        valet v;
        return ClangTool(p->getCompilations(), p->getSourcePathList()).run(newFrontendActionFactory(&v, &v).get());
    } else {
        llvm::errs() << p.takeError();
        return 1;
    }
}

ParsedAttrInfoRegistry::Add<skip_attr> _("valor", "");
std::unordered_set<Decl const*> skip_attr::skip;
logline::level logline::loglevel = parse_lvl();
