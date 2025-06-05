/******************************************************************************
 * Top contributors (to current version):
 *   Andrew Reynolds, Aina Niemetz, Gereon Kremer
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2025 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Black box testing of the OmtResult class
 */

#include "test_api.h"

#include <util/omt_result.h>

namespace cvc5::internal {

namespace test {

class TestApiBlackOmtResult : public TestApi
{
};

TEST_F(TestApiBlackOmtResult, isNull)
{
  cvc5::OmtResult res_null;
  ASSERT_TRUE(res_null.isNull());
  ASSERT_FALSE(res_null.isOptimal());
  ASSERT_FALSE(res_null.isLimitOptimal());
  ASSERT_FALSE(res_null.isNonOptimal());
  ASSERT_FALSE(res_null.isUnbounded());
  ASSERT_FALSE(res_null.isUnsat()); 
  ASSERT_FALSE(res_null.isUnknown()); 
}

}  // namespace test
}  // namespace cvc5::internal
