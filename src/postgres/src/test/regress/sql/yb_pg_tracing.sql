SELECT is_yb_pg_tracing_enabled((SELECT pg_backend_pid()), NULL);

SELECT yb_pg_enable_tracing(pid => -1, query_id => NULL);

SELECT is_yb_pg_tracing_enabled(-1, NULL);

SELECT yb_pg_enable_tracing(query_id => NULL, pid => (SELECT pg_backend_pid()));

SELECT yb_pg_enable_tracing(-100, query_id => NULL);

SELECT yb_pg_enable_tracing(0, NULL);

SELECT yb_pg_enable_tracing(query_id => NULL, -1);

SELECT yb_pg_disable_tracing(-1, NULL);

