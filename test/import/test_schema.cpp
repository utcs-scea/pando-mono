// SPDX-License-Identifier: MIT
// Copyright (c) 2023. University of Texas at Austin. All rights reserved.

#include <gtest/gtest.h>
#include <pando-rt/export.h>

#include <pando-lib-galois/graphs/wmd_graph.hpp>
#include <pando-lib-galois/import/schema.hpp>
#include <pando-rt/containers/vector.hpp>

namespace {

const char* someFile = "some_file.csv";
const char* someFile2 = "some_file2.csv";

galois::WMDParser<galois::WMDVertex, galois::WMDEdge> getParser() {
  pando::Vector<const char*> files;
  EXPECT_EQ(files.initialize(0), pando::Status::Success);
  EXPECT_EQ(files.pushBack(someFile), pando::Status::Success);
  EXPECT_EQ(files.pushBack(someFile2), pando::Status::Success);
  return galois::WMDParser<galois::WMDVertex, galois::WMDEdge>(files);
}

void checkParsedNode(galois::ParsedGraphStructure<galois::WMDVertex, galois::WMDEdge> result,
                     uint64_t id, agile::TYPES type) {
  EXPECT_EQ(result.isNode, true);
  EXPECT_EQ(result.isEdge, false);
  EXPECT_EQ(result.edges.size(), 0);
  EXPECT_EQ(result.node.id, id);
  EXPECT_EQ(result.node.edges, 0);
  EXPECT_EQ(result.node.type, type);
}

void checkParsedEdge(galois::ParsedGraphStructure<galois::WMDVertex, galois::WMDEdge> result,
                     galois::WMDEdge expected, agile::TYPES expectedInverse) {
  EXPECT_EQ(result.isNode, false);
  EXPECT_EQ(result.isEdge, true);
  EXPECT_EQ(result.edges.size(), 2);

  galois::WMDEdge edge0 = result.edges[0];
  galois::WMDEdge edge1 = result.edges[1];

  EXPECT_EQ(edge0.src, expected.src);
  EXPECT_EQ(edge0.dst, expected.dst);
  EXPECT_EQ(edge0.type, expected.type);
  EXPECT_EQ(edge0.srcType, expected.srcType);
  EXPECT_EQ(edge0.dstType, expected.dstType);
  EXPECT_EQ(edge1.type, expectedInverse);

  EXPECT_NE(edge0.type, edge1.type);
  EXPECT_EQ(edge0.src, edge1.dst);
  EXPECT_EQ(edge0.dst, edge1.src);
  EXPECT_EQ(edge0.srcType, edge1.dstType);
  EXPECT_EQ(edge0.dstType, edge1.srcType);
}

} // namespace

TEST(ImportSchema, Constructor) {
  galois::WMDParser<galois::WMDVertex, galois::WMDEdge> parser = getParser();
  pando::Vector<const char*> files = parser.getFiles();
  EXPECT_EQ(files.size(), 2);
  EXPECT_EQ(files[0], someFile);
  EXPECT_EQ(files[1], someFile2);
}

TEST(ImportSchema, Parse) {
  galois::WMDParser<galois::WMDVertex, galois::WMDEdge> parser = getParser();

  const char* invalid = "invalid,,,1615340315424362057,1116314936447312244,,,2/11/2018,,";

  const char* person = "Person,477384404927196020,,,,,,,,";
  const char* person2 = "Person,1011840732795343182,,,,,,,,";
  const char* forumEvent = "ForumEvent,,,1615340315424362057,1116314936447312244,,,2/11/2018,,";
  const char* forum = "Forum,,,227560344059645632,,,,,,;";
  const char* publication = "Publication,,,,,102583151124020340,,4/1/2013,,";
  const char* topicMinimal = "Topic,,,,,,271997,,,";
  const char* topicPositive = "Topic,,,,,,929,,6.7,20.9";
  const char* topicNegative = "Topic,,,,,,34128,,-17.6666666667,-149.583333333";

  const char* sale = "Sale,46514102944103431,354168676132531843,,,,,8/6/2018,,";
  const char* author = "Author,1338150154370467418,,,,1613206864711314799,,,,";
  const char* include = "Includes,,,353365307219544531,581872462392533272,,,,,";
  const char* hasTopic = "HasTopic,,,,1044846551426542419,,9420,,,";
  const char* hasOrg = "HasOrg,,,,,1660292526362246147,49210,,,";

  galois::ParsedGraphStructure<galois::WMDVertex, galois::WMDEdge> result;
  result = parser.parseLine(invalid);
  EXPECT_EQ(result.isNode, false);
  EXPECT_EQ(result.isEdge, false);
  EXPECT_EQ(result.edges.size(), 0);

  result = parser.parseLine(person);
  checkParsedNode(result, 477384404927196020UL, agile::TYPES::PERSON);
  result = parser.parseLine(person2);
  checkParsedNode(result, 1011840732795343182UL, agile::TYPES::PERSON);
  result = parser.parseLine(forumEvent);
  checkParsedNode(result, 1116314936447312244UL, agile::TYPES::FORUMEVENT);
  result = parser.parseLine(forum);
  checkParsedNode(result, 227560344059645632UL, agile::TYPES::FORUM);
  result = parser.parseLine(publication);
  checkParsedNode(result, 102583151124020340UL, agile::TYPES::PUBLICATION);
  result = parser.parseLine(topicMinimal);
  checkParsedNode(result, 271997UL, agile::TYPES::TOPIC);
  result = parser.parseLine(topicPositive);
  checkParsedNode(result, 929UL, agile::TYPES::TOPIC);
  result = parser.parseLine(topicNegative);
  checkParsedNode(result, 34128UL, agile::TYPES::TOPIC);

  result = parser.parseLine(sale);
  checkParsedEdge(result,
                  galois::WMDEdge(46514102944103431UL, 354168676132531843UL, agile::TYPES::SALE,
                                  agile::TYPES::PERSON, agile::TYPES::PERSON),
                  agile::TYPES::PURCHASE);
  result = parser.parseLine(author);
  checkParsedEdge(
      result,
      galois::WMDEdge(1338150154370467418UL, 1613206864711314799UL, agile::TYPES::AUTHOR,
                      agile::TYPES::PERSON, agile::TYPES::PUBLICATION),
      agile::TYPES::WRITTENBY);
  result = parser.parseLine(include);
  checkParsedEdge(
      result,
      galois::WMDEdge(353365307219544531UL, 581872462392533272UL, agile::TYPES::INCLUDES,
                      agile::TYPES::FORUM, agile::TYPES::FORUMEVENT),
      agile::TYPES::INCLUDEDIN);
  result = parser.parseLine(hasTopic);
  checkParsedEdge(result,
                  galois::WMDEdge(1044846551426542419UL, 9420UL, agile::TYPES::HASTOPIC,
                                  agile::TYPES::FORUMEVENT, agile::TYPES::TOPIC),
                  agile::TYPES::TOPICIN);
  result = parser.parseLine(hasOrg);
  checkParsedEdge(result,
                  galois::WMDEdge(1660292526362246147UL, 49210UL, agile::TYPES::HASORG,
                                  agile::TYPES::PUBLICATION, agile::TYPES::TOPIC),
                  agile::TYPES::ORG_IN);
}
