/******************************************************************************
 * Top contributors (to current version):
 *   Andrew Reynolds, Aina Niemetz, Andres Noetzli
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2025 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Implementation of virtual term substitution term cache.
 */

#include "theory/quantifiers/cegqi/vts_term_cache.h"

#include "expr/node_algorithm.h"
#include "expr/skolem_manager.h"
#include "expr/sort_to_term.h"
#include "theory/arith/arith_msum.h"
#include "theory/quantifiers/quantifiers_inference_manager.h"
#include "theory/rewriter.h"
#include "util/rational.h"

using namespace cvc5::internal::kind;

namespace cvc5::internal {
namespace theory {
namespace quantifiers {

VtsTermCache::VtsTermCache(Env& env) : EnvObj(env), d_hasAllocated(false) {}

bool VtsTermCache::hasAllocated() const { return d_hasAllocated; }

void VtsTermCache::getVtsTerms(std::vector<Node>& t,
                               bool isFree,
                               bool create,
                               bool inc_delta)
{
  if (inc_delta)
  {
    Node delta = getVtsDelta(isFree, create);
    if (!delta.isNull())
    {
      t.push_back(delta);
    }
  }
  NodeManager* nm = nodeManager();
  for (unsigned r = 0; r < 2; r++)
  {
    TypeNode tn = r == 0 ? nm->realType() : nm->integerType();
    Node inf = getVtsInfinity(tn, isFree, create);
    if (!inf.isNull())
    {
      t.push_back(inf);
    }
  }
}

Node VtsTermCache::getVtsDelta(bool isFree, bool create)
{
  if (create)
  {
    NodeManager* nm = nodeManager();
    SkolemManager* sm = nm->getSkolemManager();
    if (d_vts_delta_free.isNull())
    {
      d_hasAllocated = true;
      d_vts_delta_free = sm->mkSkolemFunction(SkolemId::ARITH_VTS_DELTA_FREE);
    }
    if (d_vts_delta.isNull())
    {
      d_hasAllocated = true;
      d_vts_delta = sm->mkSkolemFunction(SkolemId::ARITH_VTS_DELTA);
      // mark as a virtual term
      VirtualTermSkolemAttribute vtsa;
      d_vts_delta.setAttribute(vtsa, true);
    }
  }
  return isFree ? d_vts_delta_free : d_vts_delta;
}

Node VtsTermCache::getVtsInfinity(TypeNode tn, bool isFree, bool create)
{
  Assert(tn.isRealOrInt());
  if (create)
  {
    NodeManager* nm = nodeManager();
    SkolemManager* sm = nm->getSkolemManager();
    Node stt = nm->mkConst(SortToTerm(tn));
    if (d_vts_inf_free[tn].isNull())
    {
      d_hasAllocated = true;
      d_vts_inf_free[tn] =
          sm->mkSkolemFunction(SkolemId::ARITH_VTS_INFINITY_FREE, stt);
    }
    if (d_vts_inf[tn].isNull())
    {
      d_hasAllocated = true;
      d_vts_inf[tn] = sm->mkSkolemFunction(SkolemId::ARITH_VTS_INFINITY, stt);
      // mark as a virtual term
      VirtualTermSkolemAttribute vtsa;
      d_vts_inf[tn].setAttribute(vtsa, true);
    }
  }
  return isFree ? d_vts_inf_free[tn] : d_vts_inf[tn];
}

Node VtsTermCache::substituteVtsFreeTerms(Node n)
{
  std::vector<Node> vars;
  getVtsTerms(vars, false, false);
  std::vector<Node> vars_free;
  getVtsTerms(vars_free, true, false);
  Assert(vars.size() == vars_free.size());
  if (vars.empty())
  {
    return n;
  }
  return n.substitute(
      vars.begin(), vars.end(), vars_free.begin(), vars_free.end());
}

Node VtsTermCache::rewriteVtsSymbols(Node n)
{
  NodeManager* nm = nodeManager();
  if (((n.getKind() == Kind::EQUAL && n[0].getType().isRealOrInt())
       || n.getKind() == Kind::GEQ))
  {
    Trace("quant-vts-debug") << "VTS : process " << n << std::endl;
    Node rew_vts_inf;
    bool rew_delta = false;
    // rewriting infinity always takes precedence over rewriting delta
    for (unsigned r = 0; r < 2; r++)
    {
      TypeNode tn = r == 0 ? nm->realType() : nm->integerType();
      Node inf = getVtsInfinity(tn, false, false);
      if (!inf.isNull() && expr::hasSubterm(n, inf))
      {
        if (rew_vts_inf.isNull())
        {
          rew_vts_inf = inf;
        }
        else
        {
          // for mixed int/real with multiple infinities
          Trace("quant-vts-debug") << "Multiple infinities...equate " << inf
                                   << " = " << rew_vts_inf << std::endl;
          std::vector<Node> subs_lhs;
          subs_lhs.push_back(inf);
          std::vector<Node> subs_rhs;
          subs_rhs.push_back(rew_vts_inf);
          n = n.substitute(subs_lhs.begin(),
                           subs_lhs.end(),
                           subs_rhs.begin(),
                           subs_rhs.end());
          n = rewrite(n);
          // may have cancelled
          if (!expr::hasSubterm(n, rew_vts_inf))
          {
            rew_vts_inf = Node::null();
          }
        }
      }
    }
    if (rew_vts_inf.isNull())
    {
      if (!d_vts_delta.isNull() && expr::hasSubterm(n, d_vts_delta))
      {
        rew_delta = true;
      }
    }
    if (!rew_vts_inf.isNull() || rew_delta)
    {
      std::map<Node, Node> msum;
      if (ArithMSum::getMonomialSumLit(n, msum))
      {
        if (TraceIsOn("quant-vts-debug"))
        {
          Trace("quant-vts-debug") << "VTS got monomial sum : " << std::endl;
          ArithMSum::debugPrintMonomialSum(msum, "quant-vts-debug");
        }
        Node vts_sym = !rew_vts_inf.isNull() ? rew_vts_inf : d_vts_delta;
        Assert(!vts_sym.isNull());
        Node iso_n;
        Node nlit;
        int res = ArithMSum::isolate(vts_sym, msum, iso_n, n.getKind(), true);
        if (res != 0)
        {
          Trace("quant-vts-debug") << "VTS isolated :  -> " << iso_n
                                   << ", res = " << res << std::endl;
          Node slv = iso_n[res == 1 ? 1 : 0];
          // ensure the vts terms have been eliminated
          if (containsVtsTerm(slv))
          {
            Trace("quant-vts-warn")
                << "Bad vts literal : " << n << ", contains " << vts_sym
                << " but bad solved form " << slv << "." << std::endl;
            // safe case: just convert to free symbols
            nlit = substituteVtsFreeTerms(n);
            Trace("quant-vts-debug") << "...return " << nlit << std::endl;
            return nlit;
          }
          else
          {
            if (!rew_vts_inf.isNull())
            {
              nlit = nm->mkConst(n.getKind() == Kind::GEQ && res == 1);
            }
            else
            {
              Assert(iso_n[res == 1 ? 0 : 1] == d_vts_delta);
              if (n.getKind() == Kind::EQUAL)
              {
                nlit = nm->mkConst(false);
              }
              else
              {
                Node zero = nm->mkConstRealOrInt(slv.getType(), Rational(0));
                if (res == 1)
                {
                  nlit = nm->mkNode(Kind::GEQ, zero, slv);
                }
                else
                {
                  nlit = nm->mkNode(Kind::GT, slv, zero);
                }
              }
            }
          }
          Trace("quant-vts-debug") << "Return " << nlit << std::endl;
          return nlit;
        }
        else
        {
          Trace("quant-vts-warn")
              << "Bad vts literal : " << n << ", contains " << vts_sym
              << " but could not isolate." << std::endl;
          // safe case: just convert to free symbols
          nlit = substituteVtsFreeTerms(n);
          Trace("quant-vts-debug") << "...return " << nlit << std::endl;
          return nlit;
        }
      }
    }
    return n;
  }
  else if (n.getKind() == Kind::FORALL)
  {
    // cannot traverse beneath quantifiers
    return substituteVtsFreeTerms(n);
  }
  bool childChanged = false;
  std::vector<Node> children;
  for (const Node& nc : n)
  {
    Node nn = rewriteVtsSymbols(nc);
    children.push_back(nn);
    childChanged = childChanged || nn != nc;
  }
  if (childChanged)
  {
    if (n.getMetaKind() == kind::metakind::PARAMETERIZED)
    {
      children.insert(children.begin(), n.getOperator());
    }
    Node ret = nm->mkNode(n.getKind(), children);
    Trace("quant-vts-debug") << "...make node " << ret << std::endl;
    return ret;
  }
  return n;
}

bool VtsTermCache::containsVtsTerm(Node n, bool isFree)
{
  std::vector<Node> t;
  getVtsTerms(t, isFree, false);
  return expr::hasSubterm(n, t);
}

bool VtsTermCache::containsVtsTerm(std::vector<Node>& n, bool isFree)
{
  std::vector<Node> t;
  getVtsTerms(t, isFree, false);
  if (!t.empty())
  {
    for (const Node& nc : n)
    {
      if (expr::hasSubterm(nc, t))
      {
        return true;
      }
    }
  }
  return false;
}

bool VtsTermCache::containsVtsInfinity(Node n, bool isFree)
{
  std::vector<Node> t;
  getVtsTerms(t, isFree, false, false);
  return expr::hasSubterm(n, t);
}

}  // namespace quantifiers
}  // namespace theory
}  // namespace cvc5::internal
