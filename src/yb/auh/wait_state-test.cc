// Copyright (c) YugabyteDB, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
// in compliance with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software distributed under the License
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
// or implied.  See the License for the specific language governing permissions and limitations
// under the License.
//

#include <gtest/gtest.h>

#include "yb/auh/wait_state.h"
#include "yb/common/common.pb.h"
#include "yb/util/random_util.h"
#include "yb/util/test_util.h"

namespace yb {

using yb::auh::AUHAuxInfo;
using yb::auh::AUHMetadata;

class WaitStateTest : public YBTest {};

HostPort RandomHostPort() {
  return HostPort(Format("host-$0", RandomUniformInt<uint16_t>(0, 10)),
                              RandomUniformInt<uint16_t>());
}

AUHMetadata GenerateRandomMetadata() {
  return AUHMetadata{
      .top_level_request_id{Uuid::Generate()},
      .top_level_node_id{Uuid::Generate()},
      .query_id = RandomUniformInt<int64_t>(),
      .current_request_id = RandomUniformInt<int64_t>(),
      .client_host_port = RandomHostPort()};
}

TEST(WaitStateTest, TestToAndFromPB) {
  AUHMetadata meta1 = GenerateRandomMetadata();
  AUHMetadataPB pb;
  meta1.ToPB(&pb);
  ASSERT_EQ(pb.top_level_request_id().size(), kUuidSize);
  ASSERT_EQ(pb.top_level_node_id().size(), kUuidSize);
  AUHMetadata meta2 = AUHMetadata::FromPB(pb);
  ASSERT_EQ(meta1.top_level_request_id, meta2.top_level_request_id);
  ASSERT_EQ(meta1.top_level_node_id, meta2.top_level_node_id);
  ASSERT_EQ(meta1.query_id, meta2.query_id);
  ASSERT_EQ(meta1.current_request_id, meta2.current_request_id);
  ASSERT_EQ(meta1.client_host_port, meta2.client_host_port);
}

TEST(WaitStateTest, TestUpdate) {
  AUHMetadata meta1 = GenerateRandomMetadata();
  const AUHMetadata meta1_copy = meta1;
  // Update 3 fields, rest unset.
  AUHMetadataPB pb1;
  auto pb1_top_level_request_id = Uuid::Generate();
  pb1_top_level_request_id.ToBytes(pb1.mutable_top_level_request_id());
  pb1.set_query_id(RandomUniformInt<int64_t>());
  HostPortToPB(RandomHostPort(), pb1.mutable_client_host_port());
  meta1.UpdateFrom(AUHMetadata::FromPB(pb1));
  ASSERT_EQ(meta1.top_level_request_id, pb1_top_level_request_id);
  ASSERT_EQ(meta1.top_level_node_id, meta1_copy.top_level_node_id);
  ASSERT_EQ(meta1.query_id, pb1.query_id());
  ASSERT_EQ(meta1.current_request_id, meta1_copy.current_request_id);
  ASSERT_EQ(meta1.client_host_port, HostPortFromPB(pb1.client_host_port()));

  meta1 = meta1_copy;
  // Update 2 other fields, rest unset.
  AUHMetadataPB pb2;
  auto pb2_top_level_node_id = Uuid::Generate();
  pb2_top_level_node_id.ToBytes(pb2.mutable_top_level_node_id());
  pb2.set_current_request_id(RandomUniformInt<int64_t>());
  meta1.UpdateFrom(AUHMetadata::FromPB(pb2));
  ASSERT_EQ(meta1.top_level_request_id, meta1_copy.top_level_request_id);
  ASSERT_EQ(meta1.top_level_node_id, pb2_top_level_node_id);
  ASSERT_EQ(meta1.query_id, meta1_copy.query_id);
  ASSERT_EQ(meta1.current_request_id, pb2.current_request_id());
  ASSERT_EQ(meta1.client_host_port, meta1_copy.client_host_port);
}

}  // namespace yb
