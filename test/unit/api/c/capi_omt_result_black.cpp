/******************************************************************************
 * Top contributors (to current version):
 *   Aina Niemetz
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2025 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * Black box testing of OMT result functions of the C API.
 */

extern "C" {
#include <cvc5/c/cvc5.h>
}

#include "gtest/gtest.h"

namespace cvc5::internal::test {

class TestCApiBlackOmtResult : public ::testing::Test
{};

TEST_F(TestCApiBlackOmtResult, is_null)
{
  ASSERT_DEATH(cvc5_omt_result_is_null(nullptr), "invalid OMT result");
}

}  // namespace cvc5::internal::test
