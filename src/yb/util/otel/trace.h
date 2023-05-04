// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// The following only applies to changes made to this file as part of YugaByte development.
//
// Portions Copyright (c) YugaByte, Inc.
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

#include <stddef.h>
#include <string>
#include <map>
#include <stack>

#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/span.h"

#include "yb/util/monotime.h"

namespace trace_api      = opentelemetry::trace;
namespace nostd          = opentelemetry::nostd;

namespace yb {

static const size_t kTraceIdSize             = 32;
static const size_t kSpanIdSize              = 16;

#define INVALID_SPAN new trace_api::DefaultSpan(trace_api::SpanContext::GetInvalid())

void InitPgTracer(int pid);
void InitTserverTracer(const std::string& host_name, const std::string& uuid);

void CleanupTracer();

nostd::shared_ptr<trace_api::Tracer> get_tracer(std::string tracer_name);
nostd::shared_ptr<trace_api::Span> StartSpanFromParentId(
    const std::string& trace_id, const std::string& span_id, const std::string& span);
nostd::shared_ptr<trace_api::Span> StartSpan(
    const std::string& span_name,
    const trace_api::SpanContext& parent_context);

class TraceAggregates {
 public:
    TraceAggregates() = default;
    ~TraceAggregates() = default;

    void IncrementCounterAndStartTimer(const char* counter) {
        this->trace_aggregates_[std::string(counter)] ++;
        this->span_timer_.push(MonoTime::Now());
    }

    void EndTimer(const char* timer) {
        assert(!this->span_timer_.empty());
        auto time = MonoTime::Now() - this->span_timer_.top();
        this->trace_aggregates_[std::string(timer)] += time.ToMilliseconds();
        this->span_timer_.pop();
    }

    void SetAggregates(nostd::shared_ptr<opentelemetry::trace::Span> span) {
        for (auto [aggregate_key, aggregate_value] : this->trace_aggregates_) {
            span->SetAttribute(aggregate_key, std::to_string(aggregate_value) + "ms");
        }
        this->trace_aggregates_.clear();
    }

 private:
    std::unordered_map<std::string, double> trace_aggregates_;
    std::stack<MonoTime> span_timer_;
};

} //namespace