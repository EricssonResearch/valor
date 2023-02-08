#include "valet.h"
#include <clang/Tooling/CommonOptionsParser.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <clang/Tooling/Tooling.h>
#pragma GCC diagnostic pop

using namespace llvm::cl;
using namespace clang;
using namespace clang::tooling;

OptionCategory opt_cat("Valet options");
extrahelp help0(CommonOptionsParser::HelpMessage);
extrahelp help("\nValet help...\n");

int main(int argc, const char** argv) {
    if (auto p = CommonOptionsParser::create(argc, argv, opt_cat)) {
        auto& pp = p.get();
        return ClangTool(pp.getCompilations(), pp.getSourcePathList()).run(newFrontendActionFactory<valor::valet>().get());
    } else {
        llvm::errs() << p.takeError();
        return 1;
    }
}
