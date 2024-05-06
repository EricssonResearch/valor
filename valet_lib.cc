/** This file is part of the Valor project which is released under the MIT license.
 * See file COPYING for full license details.
 * Copyright 2024 Ericsson AB
 */
#include "valet.h"
using namespace clang;
using namespace valor;
namespace {
ParsedAttrInfoRegistry::Add<skip_attr> _("valor", "");
}
std::unordered_set<Decl const*> skip_attr::skip;
logline::level logline::loglevel = parse_lvl();
