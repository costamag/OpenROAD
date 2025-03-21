/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <string>

#include "boostParser.h"
#include "lefLayerPropParser.h"
#include "odb/db.h"
#include "odb/lefin.h"

namespace odb::lefTechLayerCornerSpacing {

void setWithin(double value,
               odb::dbTechLayerCornerSpacingRule* rule,
               odb::lefinReader* lefinReader)
{
  rule->setCornerOnly(true);
  rule->setWithin(lefinReader->dbdist(value));
}
void setEolWidth(double value,
                 odb::dbTechLayerCornerSpacingRule* rule,
                 odb::lefinReader* lefinReader)
{
  rule->setExceptEol(true);
  rule->setEolWidth(lefinReader->dbdist(value));
}
void setJogLength(double value,
                  odb::dbTechLayerCornerSpacingRule* rule,
                  odb::lefinReader* lefinReader)
{
  rule->setExceptJogLength(true);
  rule->setJogLength(lefinReader->dbdist(value));
}
void setEdgeLength(double value,
                   odb::dbTechLayerCornerSpacingRule* rule,
                   odb::lefinReader* lefinReader)
{
  rule->setEdgeLengthValid(true);
  rule->setEdgeLength(lefinReader->dbdist(value));
}
void setMinLength(double value,
                  odb::dbTechLayerCornerSpacingRule* rule,
                  odb::lefinReader* lefinReader)
{
  rule->setMinLengthValid(true);
  rule->setMinLength(lefinReader->dbdist(value));
}
void setExceptNotchLength(double value,
                          odb::dbTechLayerCornerSpacingRule* rule,
                          odb::lefinReader* lefinReader)
{
  rule->setExceptNotchLengthValid(true);
  rule->setExceptNotchLength(lefinReader->dbdist(value));
}
void addSpacing(
    boost::fusion::vector<double, double, boost::optional<double>>& params,
    odb::dbTechLayerCornerSpacingRule* rule,
    odb::lefinReader* lefinReader)
{
  auto width = lefinReader->dbdist(at_c<0>(params));
  auto spacing1 = lefinReader->dbdist(at_c<1>(params));
  auto spacing2 = at_c<2>(params);
  if (spacing2.is_initialized()) {
    rule->addSpacing(width, spacing1, lefinReader->dbdist(spacing2.value()));
  } else {
    rule->addSpacing(width, spacing1, spacing1);
  }
}
template <typename Iterator>
bool parse(Iterator first,
           Iterator last,
           odb::dbTechLayer* layer,
           odb::lefinReader* lefinReader)
{
  odb::dbTechLayerCornerSpacingRule* rule
      = odb::dbTechLayerCornerSpacingRule::create(layer);
  qi::rule<std::string::iterator, space_type> convexCornerRule
      = (lit("CONVEXCORNER")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setType,
             rule,
             odb::dbTechLayerCornerSpacingRule::CONVEXCORNER)]
         >> -(lit("SAMEMASK")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setSameMask, rule, true)])
         >> -((lit("CORNERONLY")
               >> double_[boost::bind(&setWithin, _1, rule, lefinReader)])
              | lit("CORNERTOCORNER")[boost::bind(
                  &odb::dbTechLayerCornerSpacingRule::setCornerToCorner,
                  rule,
                  true)])
         >> -(lit("EXCEPTEOL")
              >> double_[boost::bind(&setEolWidth, _1, rule, lefinReader)]
              >> -(lit("EXCEPTJOGLENGTH")
                   >> double_[boost::bind(&setJogLength, _1, rule, lefinReader)]
                   >> -(lit("EDGELENGTH") >> double_[boost::bind(
                            &setEdgeLength, _1, rule, lefinReader)])
                   >> -(lit("INCLUDELSHAPE")[boost::bind(
                       &odb::dbTechLayerCornerSpacingRule::setIncludeShape,
                       rule,
                       true)]))));
  qi::rule<std::string::iterator, space_type> concaveCornerRule
      = (lit("CONCAVECORNER")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setType,
             rule,
             odb::dbTechLayerCornerSpacingRule::CONCAVECORNER)]
         >> -(lit("MINLENGTH")
              >> double_[boost::bind(&setMinLength, _1, rule, lefinReader)]
              >> -(lit("EXCEPTNOTCH")[boost::bind(
                       &odb::dbTechLayerCornerSpacingRule::setExceptNotch,
                       rule,
                       true)]
                   >> -double_[boost::bind(
                       &setExceptNotchLength, _1, rule, lefinReader)])));
  qi::rule<std::string::iterator, space_type> exceptSameRule
      = (lit("EXCEPTSAMENET")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setExceptSameNet, rule, true)]
         | lit("EXCEPTSAMEMETAL")[boost::bind(
             &odb::dbTechLayerCornerSpacingRule::setExceptSameMetal,
             rule,
             true)]);

  qi::rule<std::string::iterator, space_type> spacingRule
      = (lit("WIDTH") >> double_ >> lit("SPACING") >> double_
         >> -double_)[boost::bind(&addSpacing, _1, rule, lefinReader)];

  qi::rule<std::string::iterator, space_type> cornerSpacingRule
      = (lit("CORNERSPACING") >> (convexCornerRule | concaveCornerRule)
         >> -(exceptSameRule) >> +(spacingRule) >> lit(";"));

  bool valid = qi::phrase_parse(first, last, cornerSpacingRule, space)
               && first == last;

  if (!valid) {
    odb::dbTechLayerCornerSpacingRule::destroy(rule);
  }

  return valid;
}
}  // namespace odb::lefTechLayerCornerSpacing

namespace odb {

bool lefTechLayerCornerSpacingParser::parse(std::string s,
                                            dbTechLayer* layer,
                                            odb::lefinReader* l)
{
  return lefTechLayerCornerSpacing::parse(s.begin(), s.end(), layer, l);
}

}  // namespace odb
