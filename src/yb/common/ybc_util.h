// Copyright (c) YugaByte, Inc.
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

// C wrappers around some YB utilities. Suitable for inclusion into C codebases such as our modified
// version of PostgreSQL.

#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {

struct varlena;

#endif

/*
 * Guc variable to log the protobuf string for every outgoing (DocDB) read/write request.
 * See the "YB Debug utils" section in pg_yb_utils.h (as well as guc.c) for more information.
 */
extern bool yb_debug_log_docdb_requests;

/*
 * Toggles whether formatting functions exporting system catalog information
 * include DocDB metadata (such as tablet split information).
 */
extern bool yb_format_funcs_include_yb_metadata;

/*
 * Guc variable to enable the use of regular transactions for operating on system catalog tables
 * in case a DDL transaction has not been started.
 */
extern bool yb_non_ddl_txn_for_sys_tables_allowed;

/*
 * Toggles whether to force use of global transaction status table.
 */
extern bool yb_force_global_transaction;

/*
 * Guc that toggles whether strict inequalities are pushed down.
 */
extern bool yb_pushdown_strict_inequality;

/*
 * Guc variable to suppress non-Postgres logs from appearing in Postgres log file.
 */
extern bool suppress_nonpg_logs;

/*
 * Guc variable to control the max session batch size before flushing.
 */
extern int ysql_session_max_batch_size;

/*
 * Guc variable to control the max number of in-flight operations from YSQL to tablet server.
 */
extern int ysql_max_in_flight_ops;

/*
 * Guc variable to enable binary restore from a binary backup of YSQL tables. When doing binary
 * restore, we copy the docdb SST files of those tables from the source database and reuse them
 * for a newly created target database to restore those tables.
 */
extern bool yb_binary_restore;

/*
 * Set to true only for runs with EXPLAIN ANALYZE
 */
extern bool yb_run_with_explain_analyze;

/*
 * GUC variable that enables batching RPCs of generated for IN queries
 * on hash keys issued to the same tablets.
 */
extern bool yb_enable_hash_batch_in;

 /* Set to true when only for ANALYZE (EXPLAIN, DIST) and enables collection of RPC stats that may
 * slow down regular query execution.
 */
extern bool yb_run_with_analyze_explain_dist;

/* Variables related to opentelemetry tracing */
typedef struct
{
  bool 		  is_tracing_enabled;
	int64_t		query_id;
  uint32_t	global_span_counter;
  uint32_t  trace_level;
} yb_trace_vars;

extern yb_trace_vars trace_vars;

typedef struct
{
	int         statement_retries;
	double      planning_catalog_requests;
	double      catalog_read_requests;
	uint64_t    catalog_write_requests;
	uint64_t    storage_read_requests;
	uint64_t    storage_write_requests;
  double      printtup_time;
} yb_trace_counters;

extern yb_trace_counters trace_counters;

extern void ResetYbTraceVars();

extern void ResetYbCounters();

/*
 * xcluster consistency level
 */
#define XCLUSTER_CONSISTENCY_TABLET 0
#define XCLUSTER_CONSISTENCY_DATABASE 1

/*
 * Enables atomic and ordered reads of data in xCluster replicated databases. This may add a delay
 * to the visibility of all data in the database.
 */
extern int yb_xcluster_consistency_level;

typedef struct YBCStatusStruct* YBCStatus;

bool YBCStatusIsNotFound(YBCStatus s);
bool YBCStatusIsDuplicateKey(YBCStatus s);
bool YBCStatusIsSnapshotTooOld(YBCStatus s);
bool YBCStatusIsTryAgain(YBCStatus s);
uint32_t YBCStatusPgsqlError(YBCStatus s);
uint16_t YBCStatusTransactionError(YBCStatus s);
void YBCFreeStatus(YBCStatus s);

const char* YBCStatusFilename(YBCStatus s);
int YBCStatusLineNumber(YBCStatus s);
const char* YBCStatusFuncname(YBCStatus s);
size_t YBCStatusMessageLen(YBCStatus s);
const char* YBCStatusMessageBegin(YBCStatus s);
const char* YBCMessageAsCString(YBCStatus s);
unsigned int YBCStatusRelationOid(YBCStatus s);
const char** YBCStatusArguments(YBCStatus s, size_t* nargs);

bool YBCIsRestartReadError(uint16_t txn_errcode);

bool YBCIsTxnConflictError(uint16_t txn_errcode);
bool YBCIsTxnSkipLockingError(uint16_t txn_errcode);
uint16_t YBCGetTxnConflictErrorCode();

void YBCResolveHostname();

#define CHECKED_YBCSTATUS __attribute__ ((warn_unused_result)) YBCStatus

typedef void* (*YBCPAllocFn)(size_t size);

typedef struct varlena* (*YBCCStringToTextWithLenFn)(const char* c, int size);

// Global initialization of the YugaByte subsystem.
CHECKED_YBCSTATUS YBCInit(
    const char* argv0,
    YBCPAllocFn palloc_fn,
    YBCCStringToTextWithLenFn cstring_to_text_with_len_fn);

// From glog's log_severity.h:
// const int GLOG_INFO = 0, GLOG_WARNING = 1, GLOG_ERROR = 2, GLOG_FATAL = 3;

// Logging macros with printf-like formatting capabilities.
#define YBC_LOG_INFO(...) \
    YBCLogImpl(/* severity */ 0, __FILE__, __LINE__, /* stack_trace */ false, __VA_ARGS__)
#define YBC_LOG_WARNING(...) \
    YBCLogImpl(/* severity */ 1, __FILE__, __LINE__, /* stack_trace */ false, __VA_ARGS__)
#define YBC_LOG_ERROR(...) \
    YBCLogImpl(/* severity */ 2, __FILE__, __LINE__, /* stack_trace */ false, __VA_ARGS__)
#define YBC_LOG_FATAL(...) \
    YBCLogImpl(/* severity */ 3, __FILE__, __LINE__, /* stack_trace */ false, __VA_ARGS__)

// Versions of these warnings that do nothing in debug mode. The fatal version logs a warning
// in release mode but does not crash.
#ifndef NDEBUG
// Logging macros with printf-like formatting capabilities.
#define YBC_DEBUG_LOG_INFO(...) YBC_LOG_INFO(__VA_ARGS__)
#define YBC_DEBUG_LOG_WARNING(...) YBC_LOG_WARNING(__VA_ARGS__)
#define YBC_DEBUG_LOG_ERROR(...) YBC_LOG_ERROR(__VA_ARGS__)
#define YBC_DEBUG_LOG_FATAL(...) YBC_LOG_FATAL(__VA_ARGS__)
#else
#define YBC_DEBUG_LOG_INFO(...)
#define YBC_DEBUG_LOG_WARNING(...)
#define YBC_DEBUG_LOG_ERROR(...)
#define YBC_DEBUG_LOG_FATAL(...) YBC_LOG_ERROR(__VA_ARGS__)
#endif

// The following functions log the given message formatted similarly to printf followed by a stack
// trace.

#define YBC_LOG_INFO_STACK_TRACE(...) \
    YBCLogImpl(/* severity */ 0, __FILE__, __LINE__, /* stack_trace */ true, __VA_ARGS__)
#define YBC_LOG_WARNING_STACK_TRACE(...) \
    YBCLogImpl(/* severity */ 1, __FILE__, __LINE__, /* stack_trace */ true, __VA_ARGS__)
#define YBC_LOG_ERROR_STACK_TRACE(...) \
    YBCLogImpl(/* severity */ 2, __FILE__, __LINE__, /* stack_trace */ true, __VA_ARGS__)

// 5 is the index of the format string, 6 is the index of the first printf argument to check.
void YBCLogImpl(int severity,
                const char* file_name,
                int line_number,
                bool stack_trace,
                const char* format,
                ...) __attribute__((format(printf, 5, 6)));

// VA version of YBCLogImpl
void YBCLogVA(int severity,
              const char* file_name,
              int line_number,
              bool stack_trace,
              const char* format,
              va_list args);

// Returns a string representation of the given block of binary data. The memory for the resulting
// string is allocated using palloc.
const char* YBCFormatBytesAsStr(const char* data, size_t size);

const char* YBCGetStackTrace();

// Initializes global state needed for thread management, including CDS library initialization.
void YBCInitThreading();

double YBCEvalHashValueSelectivity(int32_t hash_low, int32_t hash_high);

// Otel tracing macros
typedef enum SpanTag {
  T_Planning = 0,
  T_Execution,
  T_ReadRequest,
  T_WriteRequest,
  T_CatalogRequest,
  T_FlushRead,
  T_FlushWrite,
  T_ClientWrite,
  T_Sorting,
  T_CreateFile,
  T_CloseFile
} SpanTag;

extern const char* SpanName(SpanTag tag);
extern const char* SpanCounter(SpanTag tag);
extern const char* SpanTimer(SpanTag tag);

#define VStartEventSpan(level, tag) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
      YBCStartQueryEvent(SpanName(tag), __FILE__, __LINE__, __func__); \
      YBCPushSpanKey(trace_vars.global_span_counter - 1); \
      YBCUInt32SpanAttribute("verbosity", level, trace_vars.global_span_counter - 1); \
    } else if (trace_vars.is_tracing_enabled && (level) == trace_vars.trace_level + 1) { \
      YBCIncrementCounterAndStartTimer(SpanCounter(tag)); \
    } \
  } while (0)

#define VEndEventSpan(level, tag) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
      uint32_t span_key = YBCTopSpanKey(); \
      YBCPopSpanKey(); \
      YBCEndQueryEvent(span_key); \
    } else if (trace_vars.is_tracing_enabled && (level) == trace_vars.trace_level + 1) { \
      YBCEndTimer(SpanTimer(tag)); \
    } \
  } while (0)

#define VUInt32EventAttribute(level, key, value) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
	    YBCUInt32SpanAttribute((key), (value), YBCTopSpanKey()); \
    } \
  } while (0)

#define VStringEventAttribute(level, key, value) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
	    YBCStringSpanAttribute((key), (value), YBCTopSpanKey()); \
    } \
  } while (0)

#define VDoubleEventAttribute(level, key, value) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
	    YBCUInt32SpanAttribute((key), (value), YBCTopSpanKey()); \
    } \
  } while (0)

#define VAddSpanLogs(level, logs) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
	    YBCAddLogsToSpan((logs), YBCTopSpanKey()); \
    } \
  } while (0)

#define StartEventSpan(tag) \
  VStartEventSpan(0, (tag))

#define EndEventSpan(tag) \
  VEndEventSpan(0, tag)

#define UInt32EventAttribute(key, value) \
  VUInt32EventAttribute(0, (key), (value))

#define StringEventAttribute(key, value) \
  VStringEventAttribute(0, (key), (value))

#define DoubleEventAttribute(key, value) \
  VDoubleEventAttribute(0, (key), (value))

#define AddSpanLogs(logs) \
  VAddSpanLogs(0, (logs))

#define VPggateStartEventSpan(level, tag) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
      RETURN_NOT_OK(pg_session_->StartQueryEvent(SpanName(tag), __FILE__, __LINE__, __func__)); \
      RETURN_NOT_OK(pg_session_->PushSpanKey(trace_vars.global_span_counter - 1)); \
    } else if (trace_vars.is_tracing_enabled && (level) == trace_vars.trace_level + 1) { \
      RETURN_NOT_OK(pg_session->IncrementCounterAndStartTimer(SpanCounter(tag))); \
    } \
  } while (0)

#define VPggateEndEventSpan(level) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
      uint32_t span_key = pg_session_->TopSpanKey(); \
      RETURN_NOT_OK(pg_session_->PopSpanKey()); \
      RETURN_NOT_OK(pg_session_->EndQueryEvent(span_key)); \
    } else if (trace_vars.is_tracing_enabled && (level) == trace_vars.trace_level + 1) { \
      RETURN_NOT_OK(pg_session->EndTimer(SpanTimer(tag))); \
    } \
  } while (0)

#define VPggateUInt32EventAttribute(level, key, value) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
  	  RETURN_NOT_OK(pg_session_->UInt32SpanAttribute((key), (value), pg_session_->TopSpanKey())); \
    } \
  } while (0)

#define VPggateStringEventAttribute(level, key, value) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
	    RETURN_NOT_OK(pg_session_->StringSpanAttribute((key), (value), pg_session_->TopSpanKey())); \
    } \
  } while (0)

#define VPggateDoubleEventAttribute(level, key, value, span_key) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
	    RETURN_NOT_OK(pg_session_->Int32SpanAttribute((key), (value), pg_session_->TopSpanKey())); \
    } \
  } while (0)

#define VPggateAddSpanLogs(level, logs) \
  do { \
    if (trace_vars.is_tracing_enabled && (level) <= trace_vars.trace_level) { \
	    RETURN_NOT_OK(pg_session_->AddLogsToSpan((logs), pg_session_->TopSpanKey())); \
    } \
  } while (0)

#define PggateStartEventSpan(tag) \
  VPggateStartEventSpan(0, (tag))

#define PggateEndEventSpan() \
  VPggateEndEventSpan(0)

#define PggateUInt32EventAttribute(key, value) \
  VPggateUInt32EventAttribute(0, (key), (value))

#define PggateStringEventAttribute(key, value) \
  VPggateStringEventAttribute(0, (key), (value))

#define PggateDoubleEventAttribute(key, value, span_key) \
  VPggateDoubleEventAttribute(0, (key), (value), (span_key))

#define PggateAddSpanLogs(logs) \
  VPggateAddSpanLogs(0, (logs))

#ifdef __cplusplus
} // extern "C"
#endif
