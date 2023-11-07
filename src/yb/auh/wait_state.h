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
#pragma once

#include <atomic>
#include <string>

#include "yb/common/wire_protocol.h"
#include "yb/gutil/casts.h"
#include "yb/util/enums.h"
#include "yb/util/locks.h"
#include "yb/util/net/net_util.h"
#include "yb/util/uuid.h"

#define SET_WAIT_STATUS_TO(ptr, state) \
  if ((ptr)) (ptr)->set_state(state)
#define SET_WAIT_STATUS(state) \
  SET_WAIT_STATUS_TO(yb::auh::WaitStateInfo::CurrentWaitState(), (state))

#define ADOPT_WAIT_STATE(ptr) \
  yb::auh::ScopedAdoptWaitState _scoped_state { (ptr) }

#define SCOPED_WAIT_STATUS_FOR(ptr, state) \
  yb::auh::ScopedWaitStatus _scoped_status { (ptr), (state) }
#define SCOPED_WAIT_STATUS(state) \
  SCOPED_WAIT_STATUS_FOR(yb::auh::WaitStateInfo::CurrentWaitState(), (state))

// Wait components refer to which process the specific wait-event is part of.
// Generally, these are PG, TServer, YBClient/Perform layer, and PgGate.
//
// Within each component, we further group wait events into similar groups called
// classes. Rpc related wait-events may be grouped together under "Rpc".
// Consensus related wait-events may be grouped together under a group -- "consensus".
// and so on.
//
// We use a 32-bit uint to represent a wait-event.
// The highest 4 bits of the wait-event-code represents the component.
// The next 4 bits of the wait-event-code represents the wait-event class.
//
// Each wait-event class may have up to 2^24 wait-events.

// YB AUH Wait Components
#define YB_AUH_COMPONENT_PGGATE    0xF0000000U
#define YB_AUH_COMPONENT_TSERVER   0xE0000000U
#define YB_AUH_COMPONENT_YBC       0xC0000000U
#define YB_AUH_COMPONENT_PG        0x00000000U
// YB AUH Wait Classes
#define YB_AUH_CLASS_PG_WAIT_PERFORM           0x0E000000U
#define YB_AUH_CLASS_RPC                       0xEF000000U
#define YB_AUH_CLASS_FLUSH_AND_COMPACTION      0xEE000000U
#define YB_AUH_CLASS_CONSENSUS                 0xED000000U
#define YB_AUH_CLASS_TABLET_WAIT               0xEC000000U
#define YB_AUH_CLASS_ROCKSDB                   0xEB000000U

#define YB_AUH_CLASS_PG_CLIENT_SERVICE         0xCF000000U
#define YB_AUH_CLASS_CQL_WAIT_STATE            0xCE000000U
#define YB_AUH_CLASS_CLIENT                    0xCD000000U

namespace yb::auh {

YB_DEFINE_TYPED_ENUM(WaitStateCode, uint32_t,
    ((Unused, 0)));

struct AUHMetadata {
  Uuid top_level_request_id = Uuid::Nil();
  Uuid top_level_node_id = Uuid::Nil();
  int64_t query_id = 0;
  int64_t current_request_id = 0;
  HostPort client_host_port;

  void set_client_host_port(const HostPort& host_port);

  std::string ToString() const;

  void UpdateFrom(const AUHMetadata& other) {
    if (!other.top_level_request_id.IsNil()) {
      top_level_request_id = other.top_level_request_id;
    }
    if (!other.top_level_node_id.IsNil()) {
      top_level_node_id = other.top_level_node_id;
    }
    if (other.query_id != 0) {
      query_id = other.query_id;
    }
    if (other.current_request_id != 0) {
      current_request_id = other.current_request_id;
    }
    if (other.client_host_port != HostPort()) {
      client_host_port = other.client_host_port;
    }
  }

  template <class PB>
  void ToPB(PB* pb) const {
    if (!top_level_request_id.IsNil()) {
      top_level_request_id.ToBytes(pb->mutable_top_level_request_id());
    }
    if (!top_level_node_id.IsNil()) {
      top_level_node_id.ToBytes(pb->mutable_top_level_node_id());
    }
    if (query_id != 0) {
      pb->set_query_id(query_id);
    }
    if (current_request_id != 0) {
      pb->set_current_request_id(current_request_id);
    }
    if (client_host_port != HostPort()) {
      HostPortToPB(client_host_port, pb->mutable_client_host_port());
    }
  }

  template <class PB>
  static AUHMetadata FromPB(const PB& pb) {
    Uuid top_level_request_id = Uuid::Nil();
    if (pb.has_top_level_request_id()) {
      Result<Uuid> result = Uuid::FromSlice(pb.top_level_request_id());
      WARN_NOT_OK(result, "Could not decode uuid from protobuf.");
      if (result.ok()) {
        top_level_request_id = *result;
      }
    }
    Uuid top_level_node_id = Uuid::Nil();
    if (pb.has_top_level_node_id()) {
      Result<Uuid> result = Uuid::FromSlice(pb.top_level_node_id());
      WARN_NOT_OK(result, "Could not decode uuid from protobuf.");
      if (result.ok()) {
        top_level_node_id = *result;
      }
    }
    return AUHMetadata{
        top_level_request_id,                                            // top_level_request_id
        top_level_node_id,                                               // top_level_node_id
        pb.query_id(),                                                   // query_id
        pb.current_request_id(),                                         // current_request_id
        HostPortFromPB(pb.client_host_port())                            // client_host_port
    };
  }
};

struct AUHAuxInfo {
  std::string tablet_id;
  std::string table_id;
  std::string method;

  std::string ToString() const;

  void UpdateFrom(const AUHAuxInfo& other);

  template <class PB>
  void ToPB(PB* pb) const {
    pb->set_tablet_id(tablet_id);
    pb->set_table_id(table_id);
    pb->set_method(method);
  }

  template <class PB>
  static AUHAuxInfo FromPB(const PB& pb) {
    return AUHAuxInfo{
      .tablet_id = pb.tablet_id(),
      .table_id = pb.table_id(),
      .method = pb.method()
    };
  }
};

class WaitStateInfo;
using WaitStateInfoPtr = std::shared_ptr<WaitStateInfo>;

class WaitStateInfo {
 public:
  WaitStateInfo() = default;
  explicit WaitStateInfo(AUHMetadata&& meta);

  void set_code(WaitStateCode c);
  WaitStateCode get_code() const;

  void set_top_level_request_id(const Uuid& id) EXCLUDES(mutex_);
  void set_top_level_node_id(const Uuid& top_level_node_id) EXCLUDES(mutex_);
  int64_t query_id() EXCLUDES(mutex_);
  void set_query_id(int64_t query_id) EXCLUDES(mutex_);
  void set_current_request_id(int64_t id) EXCLUDES(mutex_);
  void set_client_host_port(const HostPort& host_port) EXCLUDES(mutex_);

  static WaitStateInfoPtr CurrentWaitState();
  static void SetCurrentWaitState(WaitStateInfoPtr);

  void UpdateMetadata(const AUHMetadata& meta) EXCLUDES(mutex_);
  void UpdateAuxInfo(const AUHAuxInfo& aux) EXCLUDES(mutex_);

  template <class PB>
  static void UpdateMetadataFromPB(const PB& pb) {
    auto wait_state = CurrentWaitState();
    if (wait_state) {
      wait_state->UpdateMetadata(AUHMetadata::FromPB(pb));
    }
  }

  template <class PB>
  void ToPB(PB* pb) EXCLUDES(mutex_) {
    std::lock_guard lock(mutex_);
    metadata_.ToPB(pb->mutable_metadata());
    WaitStateCode code = get_code();
    pb->set_wait_status_code(yb::to_underlying(code));
#ifndef NDEBUG
    pb->set_wait_status_code_as_string(yb::ToString(code));
#endif
    aux_info_.ToPB(pb->mutable_aux_info());
  }

  std::string ToString() const EXCLUDES(mutex_);

 private:
  std::atomic<WaitStateCode> code_{WaitStateCode::Unused};

  mutable simple_spinlock mutex_;
  AUHMetadata metadata_ GUARDED_BY(mutex_);
  AUHAuxInfo aux_info_ GUARDED_BY(mutex_);

  // The current wait_state_ for this thread.
  static thread_local WaitStateInfoPtr threadlocal_wait_state_;
};

// A helper to adopt a WaitState and revert to the previous WaitState based on RAII.
// This should only be used on the stack (and thus created and destroyed
// on the same thread).
class ScopedAdoptWaitState {
 public:
  explicit ScopedAdoptWaitState(WaitStateInfoPtr wait_state);
  ~ScopedAdoptWaitState();

 private:
  WaitStateInfoPtr prev_state_;

  DISALLOW_COPY_AND_ASSIGN(ScopedAdoptWaitState);
};

// A helper to set the specified WaitStateCode in the specified wait_state
// and revert to the previous WaitStateCode based on RAII when it goes out of scope.
// This should only be used on the stack (and thus created and destroyed
// on the same thread).
class ScopedWaitStatus {
 public:
  ScopedWaitStatus(WaitStateInfoPtr wait_state, WaitStateCode code);
  ~ScopedWaitStatus();

 private:
  WaitStateInfoPtr wait_state_;
  const WaitStateCode code_;
  WaitStateCode prev_code_;

  DISALLOW_COPY_AND_ASSIGN(ScopedWaitStatus);
};

}  // namespace yb::auh
