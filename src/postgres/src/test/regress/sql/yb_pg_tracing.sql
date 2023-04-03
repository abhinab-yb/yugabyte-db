-- Test for checking trace current backend
-- Equivalent to SELECT is_yb_pg_tracing_enabled((SELECT pg_backend_pid()), NULL);
SELECT is_yb_pg_tracing_enabled((SELECT pg_backend_pid()));

-- Test for enabling tracing on current backend with pid and query_id specified
SELECT yb_pg_enable_tracing(pid => NULL, query_id => NULL);

-- Test to check if tracing is enabled by the above query
-- Equivalent to SELECT is_yb_pg_tracing_enabled(pid => NULL, query_id => NULL);
SELECT is_yb_pg_tracing_enabled(query_id => NULL);

-- Test for enabling tracing by changing the parameter positions
SELECT yb_pg_enable_tracing(query_id => NULL, pid => (SELECT pg_backend_pid()));

-- Test to fail for pids which doesn't exist
SELECT yb_pg_enable_tracing(-100, query_id => NULL);

-- Test to enable tracing on all backends
SELECT yb_pg_enable_tracing(0, NULL);

-- Test to disable tracing on current backend for all queries, with defaults
-- Equivalent to SELECT yb_pg_disable_tracing(NULL, NULL);
SELECT yb_pg_disable_tracing();

-- Test if tracing is disabled by the above query
SELECT is_yb_pg_tracing_enabled((SELECT pg_backend_pid()), NULL);

-- Test if trying to enable tracing for backends with invalid pids results in error
SELECT yb_pg_enable_tracing(-1);

-- Test to enable tracing for a random query id
SELECT yb_pg_enable_tracing(query_id => 12345);

-- Test to enable tracing for an already enabled query
SELECT yb_pg_enable_tracing(query_id => 12345);

-- Test to check if tracing for the above query id is enabled
SELECT is_yb_pg_tracing_enabled(query_id => 12345);

-- Test to disable tracing for the above query
SELECT yb_pg_disable_tracing(query_id => 12345);

-- Test to check if tracing for the above query id is disabled
SELECT is_yb_pg_tracing_enabled(query_id => 12345);

-- Test to check if tracing for a query id is enabled
SELECT is_yb_pg_tracing_enabled(query_id => 12346789);

-- Test to check if disabling a query which is not enabled results in a warning
SELECT yb_pg_disable_tracing(query_id => 12346789);