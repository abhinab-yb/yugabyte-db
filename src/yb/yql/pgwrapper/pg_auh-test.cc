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

#include "yb/rpc/rpc_controller.h"

#include "yb/tserver/mini_tablet_server.h"
#include "yb/tserver/pg_client.proxy.h"
#include "yb/tserver/tablet_server.h"

#include "yb/yql/pgwrapper/pg_mini_test_base.h"

DECLARE_bool(enable_yb_auh);

using namespace std::literals;
using std::string;

namespace yb {
namespace pgwrapper {

using tserver::PgClientServiceProxy;

class PgAuhTest : public pgwrapper::PgMiniTestBase {
 protected:
  static constexpr int kTimeoutMs = 2000;

  void SetUp() override {
    ANNOTATE_UNPROTECTED_WRITE(FLAGS_enable_yb_auh) = true;
    PgMiniTestBase::SetUp();
    InitPgClientProxy();
  }

  void InitPgClientProxy() {
    pg_client_service_proxy = std::make_unique<PgClientServiceProxy>(
        &client_->proxy_cache(),
        HostPort::FromBoundEndpoint(cluster_->mini_tablet_server(0)->bound_rpc_addr()));
  }

  PgClientServiceProxy* GetPgClientServiceProxy() {
    return pg_client_service_proxy.get();
  }

  Result<std::string> GetLocalTserverUuid() {
    tserver::PgGetLocalTserverUuidRequestPB req;
    rpc::RpcController controller;
    controller.set_timeout(kTimeoutMs * 1ms * kTimeMultiplier);
    tserver::PgGetLocalTserverUuidResponsePB resp;
    auto proxy = GetPgClientServiceProxy();

    RETURN_NOT_OK(proxy->GetLocalTserverUuid(req, &resp, &controller));
    RETURN_NOT_OK(ResponseStatus(resp));

    return resp.local_tserver_uuid();
  }

 private:
  std::unique_ptr<PgClientServiceProxy> pg_client_service_proxy;
};

TEST_F(PgAuhTest, LocalTserverUuid) {
  auto local_tserver_uuid = ASSERT_RESULT(GetLocalTserverUuid());
  auto expected_tserver_uuid = cluster_->mini_tablet_server(0)->server()->permanent_uuid();

  ASSERT_EQ(local_tserver_uuid, expected_tserver_uuid);
}

} // namespace pgwrapper
} // namespace yb
