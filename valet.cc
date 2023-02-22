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
opt<bool> opt_v("v", desc("Add comments to Valet-inserted source lines"), cat(opt_cat));
list<std::string> opt_i("I", desc("Include path (most notably to find `valor.h'; shortcut for `--extra-arg=-I...')"), cat(opt_cat), Prefix);

auto overview = R"(Valet rewrites coroutines in input files with Valor™ state serialization/
deserialization function calls. If no sources are given or *any* of `sourceK'
is '-' then *only* stdin is processed to stdout.
See https://gitlab.internal.ericsson.com/sdna/valor for details.
)";

int main(int argc, char const** argv_in) {
    // Skip the need for '--' at end of cmdline (I couldn't find a way with clang::*Parser parameters)
    std::vector<char const*> argv(argv_in, argv_in + argc);
    argv.emplace_back() = "--";
    argc = argv.size();
    INFO << "This is Valet, your friendly coro-rewriter";
    auto append_args = [](auto&& opts) {
        for (auto& i: opt_i)
            opts.emplace_back("-I" + i);
        return opts;
    };
    if (auto p = CommonOptionsParser::create(argc, argv.data(), opt_cat, ZeroOrMore, overview)) {
        valet v;
        v.verbose = opt_v;
        auto fac = newFrontendActionFactory(&v, &v);
        auto files = p->getSourcePathList();
        if (files.empty() || files.end() != std::find(files.begin(), files.end(), "-")) {
            v.out = &std::cout;
            std::ostringstream ss;
            ss << std::cin.rdbuf();
            return !runToolOnCodeWithArgs(fac->create(), ss.str(), append_args(v.cc_opts(p->getArgumentsAdjuster() ? p->getArgumentsAdjuster()({}, "stdin.cc") : CommandLineArguments())), "stdin.cc");//TODO '<stdin>'
        }
        ClangTool ct(p->getCompilations(), p->getSourcePathList());
        ct.appendArgumentsAdjuster([&](auto args, auto) { return append_args(v.cc_opts(args)); });// ?only if fname.ends_with(.cc)?
        return ct.run(&*fac);
    } else {
        llvm::errs() << p.takeError();
        return 1;
    }
}
