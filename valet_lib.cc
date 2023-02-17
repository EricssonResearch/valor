#include "valet.h"
using namespace clang;
using namespace valor;
namespace {
ParsedAttrInfoRegistry::Add<skip_attr> _("valor", "");
}
std::unordered_set<Decl const*> skip_attr::skip;
logline::level logline::loglevel = parse_lvl();
