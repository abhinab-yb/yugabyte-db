---
title: What's new in YugabyteDB Voyager
linkTitle: What's new
description: YugabyteDB Voyager release notes.
headcontent: New features, key enhancements, and bug fixes
menu:
  preview:
    identifier: release-notes
    parent: voyager
    weight: 106
type: docs
---

Included here are the release notes for the YugabyteDB Voyager v1 release series. Content will be added as new notable features and changes are available in the patch releases of the YugabyteDB v1 series.

## v1.3 - May 30, 2023

### Key enhancements

* Export data for MySQL and Oracle is now 2-4x faster. To leverage this performance improvement, set the environment variable `BETA_FAST_DATA_EXPORT=1`. Most features, such as migrating partitioned tables, sequences, and so on, are supported in this mode. Refer to [Export data](../migrate-steps/#export-data) for more details.

* Added support for characters such as backspace(\b) in quote and escape character with [--file-opts](../../migrate/reference/yb-voyager-cli/#file-opts) in import data file.

* Added ability to specify null value string in import data file.

* During export data, yb-voyager can now explicitly inform you of any unsupported datatypes, and requests for permission to ignore them.

### Bug fixes

* yb-voyager can now parse `CREATE TABLE` statements that have complex check constraints.

* Import data file with AWS S3 now works when yb-voyager is installed via Docker.

## v1.2 - April 3, 2023

### Key enhancements

* When using the `import data file` command with the `--data-dir` option, you can provide an AWS S3 bucket as a path to the data directory.

* Added support for rotation of log files in a new logs directory found in `export-dir/logs`.

### Known issues

* [16658](https://github.com/yugabyte/yugabyte-db/issues/16658) The `import data file` command may not recognise the data directory being provided, causing the  step to fail for dockerized yb-voyager.

## v1.1 - March 7, 2023

### Key enhancements

* When using the `import data file` command with CSV files, YB Voyager now supports any character as an escape character and a quote character in the `--file-opts` flag, such as single quote (`'`) as a `quote_char` and backslash (`\`) as an `escape_char`, and so on. Previously, YB Voyager only supported double quotes (`"`) as a quote character and an escape character.

* Creating the Orafce extension on the target database for Oracle migrations is now available by default.

* User creation for Oracle no longer requires `EXECUTE` permissions on `PROCEDURE`, `FUNCTION`, `PACKAGE`, and `PACKAGE BODY` objects.

* The precision and scale of numeric data types from Oracle are migrated to the target database.

* For PostgreSQL migrations, YB Voyager no longer uses a password in the `pg_dump` command running in the background. Instead, the password is internally set as an environment variable to be used by `pg_dump`.

* For any syntax error in the data file or CSV file, complete error details such as line number, column, and data are displayed in the output of the `import data` or `import data file` commands.

* For the `export data` command, the list of table names passed in the `--table-list` and `--exclude-table-list` are, by default, case insensitive. Enclose each name in double quotes to make it case-sensitive.

<!-- For the import data command,the behavior remains unchanged from the previous release. These names are, by default, case-sensitive. No need to enclose them in double-quotes. -->

* In the 1.0 release, the schema details in the report generated via `analyze-schema` are sent with diagnostics when the `--send-diagnostics` flag is on. These schema details are now removed before sending diagnostics.

* The object types which YB Voyager can't categorize are placed in a separate file as `uncategorized.sql`, and the information regarding this file is available as a note under the **Notes** section in the report generated via `analyze-schema`.

### Bug fixes

* [[765](https://github.com/yugabyte/yb-voyager/issues/765)] Fixed function parsing issue when the complete body of the function is in a single line.
* [[757](https://github.com/yugabyte/yb-voyager/issues/757)] Fixed the issue of migrating tables with names as reserved keywords in the target.
