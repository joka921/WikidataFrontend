//
// Created by johannes on 11.10.18.
//

#include <gtest/gtest.h>
#include <string>
#include "../src/WikidataEntityParse.h"

using std::string;

TEST(WikidataEntityTest, parsing)
{
  string line = "<Q43>\t120\tsomeAlias\tsomeOtherAlias";
  auto e = WikidataEntityParse(line);
  ASSERT_TRUE(e._isValid);
  ASSERT_EQ(string("<Q43>"), e._wdName);
  ASSERT_EQ(120, e._numSiteLinks);
  ASSERT_EQ(2, e._aliases.size());
  ASSERT_EQ(string("someAlias"), e._aliases[0]);
  ASSERT_EQ(string("someOtherAlias"), e._aliases[1]);



}

