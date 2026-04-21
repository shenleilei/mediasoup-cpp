#!/usr/bin/env python3

import argparse
import datetime as dt
import html
import json
import os
import re
import shlex
import shutil
import smtplib
import subprocess
import sys
from email.message import EmailMessage
from pathlib import Path
from typing import Iterable, List, Optional


DEFAULT_CONFIG_PATH = ".nightly-full-regression.env"
DEFAULT_ARTIFACT_ROOT = "artifacts/nightly-full-regression"
DEFAULT_REPORT_PATH = "docs/full-regression-test-results.md"
DEFAULT_LATEST_LOG_PATH = "/var/log/run_all_tests.log"
DEFAULT_MAX_BACKUP_RUNS = 100
DEFAULT_ATTACHMENTS = [
    "docs/full-regression-test-results.md",
    "docs/uplink-qos-case-results.md",
    "docs/plain-client-qos-case-results.md",
    "docs/downlink-qos-test-results-summary.md",
    "docs/downlink-qos-case-results.md",
]
DEFAULT_SUBJECT_PREFIX = "[mediasoup-cpp nightly]"
MAX_FAILED_CASES_IN_EMAIL = 20


class FailedTask(object):
    def __init__(self, label, group, duration):
        self.label = label
        self.group = group
        self.duration = duration


class TaskResult(object):
    def __init__(self, label, group, status, duration):
        self.label = label
        self.group = group
        self.status = status
        self.duration = duration


class GroupSummary(object):
    def __init__(self, name, total, passed, failed):
        self.name = name
        self.total = total
        self.passed = passed
        self.failed = failed


class CaseSummary(object):
    def __init__(self, label, group, total_cases, failed_cases):
        self.label = label
        self.group = group
        self.total_cases = total_cases
        self.failed_cases = failed_cases


class ParsedTaskBlock(object):
    def __init__(self, label, group):
        self.label = label
        self.group = group
        self.explicit_total = None
        self.explicit_failed = None
        self.pass_case_lines = 0
        self.fail_case_lines = 0
        self.child_count = 0
        self.status = None
        self.duration = None


class ReportSummary(object):
    def __init__(
        self,
        generated_at,
        overall_status,
        attempted_tasks,
        passed_tasks,
        failed_tasks,
        failed_groups,
        selected_groups,
        failed_task_rows,
        task_rows,
        group_rows,
        task_case_rows,
        group_case_rows,
        delegated_qos_task_rows,
    ):
        self.generated_at = generated_at
        self.overall_status = overall_status
        self.attempted_tasks = attempted_tasks
        self.passed_tasks = passed_tasks
        self.failed_tasks = failed_tasks
        self.failed_groups = failed_groups
        self.selected_groups = selected_groups
        self.failed_task_rows = failed_task_rows
        self.task_rows = task_rows
        self.group_rows = group_rows
        self.task_case_rows = task_case_rows
        self.group_case_rows = group_case_rows
        self.delegated_qos_task_rows = delegated_qos_task_rows


class MailResult(object):
    def __init__(self, transport, delivered, detail):
        self.transport = transport
        self.delivered = delivered
        self.detail = detail


class RunContext(object):
    def __init__(
        self,
        repo_root,
        run_id,
        run_dir,
        attachments_dir,
        log_path,
        email_body_path,
        summary_path,
    ):
        self.repo_root = repo_root
        self.run_id = run_id
        self.run_dir = run_dir
        self.attachments_dir = attachments_dir
        self.log_path = log_path
        self.email_body_path = email_body_path
        self.summary_path = summary_path


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run the nightly full regression, save artifacts, and email a summary."
    )
    subparsers = parser.add_subparsers(dest="command")

    run_parser = subparsers.add_parser("run", help="execute the nightly full regression flow")
    run_parser.add_argument(
        "--config",
        default=DEFAULT_CONFIG_PATH,
        help=f"env-style runtime config file (default: {DEFAULT_CONFIG_PATH})",
    )
    run_parser.add_argument(
        "--skip-tests",
        action="store_true",
        help="skip launching run_all_tests.sh and reuse an existing log/report source",
    )
    run_parser.add_argument(
        "--source-log",
        help="existing log file to reuse when --skip-tests is set",
    )
    run_parser.add_argument(
        "--source-report",
        help="existing Markdown summary to parse instead of the default report path",
    )
    run_parser.add_argument(
        "--no-mail",
        action="store_true",
        help="build artifacts and summary but skip email delivery",
    )
    run_parser.add_argument(
        "--print-summary",
        action="store_true",
        help="print the rendered email body to stdout",
    )
    args = parser.parse_args()
    if not args.command:
        parser.error("a subcommand is required")
    return args


def parse_bool(value, default=False):
    if value is None:
        return default
    return str(value).strip().lower() in {"1", "true", "yes", "on"}


def parse_list(value):
    if not value:
        return []
    items = re.split(r"[,\n]+", value)
    return [item.strip() for item in items if item.strip()]


def strip_quotes(value):
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in {'"', "'"}:
        return value[1:-1]
    return value


def load_env_file(path):
    if not path.exists():
        return {}
    result = {}
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        result[key.strip()] = strip_quotes(value.strip())
    return result


def git_user_email(repo_root):
    try:
        completed = subprocess.run(
            ["git", "config", "--get", "user.email"],
            cwd=repo_root,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
        )
    except OSError:
        return None

    email = completed.stdout.strip()
    return email or None


def config_value(config_map, key, default=None):
    if key in os.environ:
        return os.environ[key]
    return config_map.get(key, default)


def abs_path(repo_root, raw_path, default_path):
    chosen = raw_path or default_path
    path = Path(chosen)
    if not path.is_absolute():
        path = repo_root / path
    return path.resolve()


def choose_mail_transport(config_map):
    transport = (config_value(config_map, "MAIL_TRANSPORT", "auto") or "auto").strip().lower()
    if transport != "auto":
        return transport
    if config_value(config_map, "SMTP_HOST"):
        return "smtp"
    if shutil.which("mailx"):
        return "mailx"
    return "smtp"


def build_runtime_config(repo_root, config_path):
    file_config = load_env_file(config_path)
    default_email = git_user_email(repo_root)
    attachment_raw = config_value(file_config, "ATTACH_MD_PATHS")
    attachment_paths = parse_list(attachment_raw) if attachment_raw else list(DEFAULT_ATTACHMENTS)

    mail_to = parse_list(config_value(file_config, "MAIL_TO", default_email) or "")
    mail_from = config_value(file_config, "MAIL_FROM", mail_to[0] if mail_to else default_email)

    return {
        "config_path": config_path,
        "artifact_root": abs_path(
            repo_root,
            config_value(file_config, "NIGHTLY_ARTIFACT_ROOT"),
            DEFAULT_ARTIFACT_ROOT,
        ),
        "latest_log_path": abs_path(
            repo_root,
            config_value(file_config, "NIGHTLY_LATEST_LOG_PATH"),
            DEFAULT_LATEST_LOG_PATH,
        ),
        "run_all_tests_path": abs_path(
            repo_root,
            config_value(file_config, "RUN_ALL_TESTS_PATH"),
            "scripts/run_all_tests.sh",
        ),
        "run_all_tests_args": shlex.split(config_value(file_config, "RUN_ALL_TESTS_ARGS", "") or ""),
        "report_path": abs_path(
            repo_root,
            config_value(file_config, "FULL_REGRESSION_REPORT_PATH"),
            DEFAULT_REPORT_PATH,
        ),
        "attachment_paths": [abs_path(repo_root, item, item) for item in attachment_paths],
        "mail_to": mail_to,
        "mail_from": mail_from,
        "mail_transport": choose_mail_transport(file_config),
        "mail_subject_prefix": config_value(
            file_config,
            "MAIL_SUBJECT_PREFIX",
            DEFAULT_SUBJECT_PREFIX,
        ),
        "nightly_max_backup_runs": int(
            config_value(file_config, "NIGHTLY_MAX_BACKUP_RUNS", str(DEFAULT_MAX_BACKUP_RUNS))
            or str(DEFAULT_MAX_BACKUP_RUNS)
        ),
        "nightly_git_commit_docs": parse_bool(
            config_value(file_config, "NIGHTLY_GIT_COMMIT_DOCS"),
            True,
        ),
        "smtp_host": config_value(file_config, "SMTP_HOST"),
        "smtp_port": int(config_value(file_config, "SMTP_PORT", "587") or "587"),
        "smtp_username": config_value(file_config, "SMTP_USERNAME"),
        "smtp_password": config_value(file_config, "SMTP_PASSWORD"),
        "smtp_use_tls": parse_bool(config_value(file_config, "SMTP_USE_TLS"), True),
        "smtp_use_ssl": parse_bool(config_value(file_config, "SMTP_USE_SSL"), False),
        "smtp_timeout_seconds": int(
            config_value(file_config, "SMTP_TIMEOUT_SECONDS", "30") or "30"
        ),
    }


def create_run_context(repo_root, config):
    now = dt.datetime.now().astimezone()
    run_id = now.strftime("%Y-%m-%dT%H-%M-%S%z")
    artifact_root = Path(config["artifact_root"])
    run_dir = artifact_root / run_id
    attachments_dir = run_dir / "attachments"
    run_dir.mkdir(parents=True, exist_ok=False)
    attachments_dir.mkdir(parents=True, exist_ok=True)
    latest_link = artifact_root / "latest"
    try:
        if latest_link.is_symlink() or latest_link.exists():
            latest_link.unlink()
        latest_link.symlink_to(run_dir.name)
    except OSError:
        pass
    return RunContext(
        repo_root=repo_root,
        run_id=run_id,
        run_dir=run_dir,
        attachments_dir=attachments_dir,
        log_path=run_dir / "run_all_tests.log",
        email_body_path=run_dir / "email-body.txt",
        summary_path=run_dir / "summary.json",
    )


def list_timestamped_run_dirs(root_path):
    if not root_path.exists():
        return []

    results = []
    for child in root_path.iterdir():
        if child.is_symlink() or not child.is_dir():
            continue
        if not re.match(r"^\d{4}-\d{2}-\d{2}T\d{2}-\d{2}-\d{2}", child.name):
            continue
        results.append(child)
    return sorted(results, key=lambda item: item.name)


def prune_timestamped_run_dirs(root_path, max_runs):
    if max_runs < 1:
        return []

    timestamped_dirs = list_timestamped_run_dirs(root_path)
    to_delete = timestamped_dirs[: max(0, len(timestamped_dirs) - max_runs)]
    removed = []

    for entry in to_delete:
        shutil.rmtree(entry, ignore_errors=False)
        removed.append(str(entry))

    return removed


def run_git_command(repo_root, args):
    try:
        completed = subprocess.run(
            ["git"] + list(args),
            cwd=repo_root,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
        )
    except OSError as error:
        return {
            "ran": False,
            "returncode": None,
            "stdout": "",
            "stderr": str(error),
        }

    return {
        "ran": True,
        "returncode": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
    }


def parse_porcelain_path(line):
    if len(line) < 4:
        return None
    raw_path = line[3:].strip()
    if " -> " in raw_path:
        return raw_path.split(" -> ", 1)[1].strip()
    return raw_path


def git_status_paths(repo_root, pathspecs):
    result = run_git_command(
        repo_root,
        ["status", "--porcelain=v1", "--untracked-files=all", "--"] + list(pathspecs),
    )
    if not result["ran"] or result["returncode"] != 0:
        return None

    paths = set()
    for raw_line in result["stdout"].splitlines():
        path = parse_porcelain_path(raw_line)
        if path:
            paths.add(path)
    return paths


def git_staged_paths(repo_root):
    result = run_git_command(repo_root, ["diff", "--cached", "--name-only"])
    if not result["ran"] or result["returncode"] != 0:
        return None
    return {
        line.strip()
        for line in result["stdout"].splitlines()
        if line.strip()
    }


def record_changed_docs_in_git(
    repo_root,
    run_context,
    enabled,
    skip_tests,
    preexisting_doc_paths,
    preexisting_staged_paths,
    overall_status,
):
    result = {
        "enabled": enabled,
        "attempted": False,
        "status": "skipped",
        "reason": None,
        "preexistingDirtyDocPaths": sorted(preexisting_doc_paths or []),
        "preexistingStagedPaths": sorted(preexisting_staged_paths or []),
        "newlyDirtyDocPaths": [],
        "commitMessage": None,
        "commitSha": None,
        "stdout": "",
        "stderr": "",
    }

    if not enabled:
        result["reason"] = "nightly git doc recording disabled"
        return result

    if skip_tests:
        result["reason"] = "--skip-tests mode does not create nightly doc commits"
        return result

    if preexisting_doc_paths is None:
        result["status"] = "failed"
        result["reason"] = "failed to read pre-run git doc status"
        return result

    if preexisting_staged_paths is None:
        result["status"] = "failed"
        result["reason"] = "failed to read pre-run staged paths"
        return result

    if preexisting_staged_paths:
        result["reason"] = "pre-existing staged changes present; nightly doc commit skipped"
        return result

    post_run_doc_paths = git_status_paths(repo_root, ["docs", "README.md"])
    if post_run_doc_paths is None:
        result["status"] = "failed"
        result["reason"] = "failed to read post-run git doc status"
        return result

    newly_dirty = sorted(post_run_doc_paths - preexisting_doc_paths)
    result["newlyDirtyDocPaths"] = newly_dirty

    if not newly_dirty:
        result["reason"] = "no newly changed docs to record"
        return result

    result["attempted"] = True
    add_result = run_git_command(repo_root, ["add", "--"] + newly_dirty)
    if not add_result["ran"] or add_result["returncode"] != 0:
        result["status"] = "failed"
        result["reason"] = "git add failed"
        result["stdout"] = add_result["stdout"]
        result["stderr"] = add_result["stderr"]
        return result

    commit_message = "chore(nightly): record full regression docs {status} {run_id}".format(
        status=(overall_status or "UNKNOWN").upper(),
        run_id=run_context.run_id,
    )
    result["commitMessage"] = commit_message

    commit_result = run_git_command(repo_root, ["commit", "-m", commit_message, "--"] + newly_dirty)
    result["stdout"] = commit_result["stdout"]
    result["stderr"] = commit_result["stderr"]
    if not commit_result["ran"] or commit_result["returncode"] != 0:
        result["status"] = "failed"
        result["reason"] = "git commit failed"
        return result

    rev_result = run_git_command(repo_root, ["rev-parse", "HEAD"])
    if rev_result["ran"] and rev_result["returncode"] == 0:
        result["commitSha"] = rev_result["stdout"].strip() or None

    result["status"] = "committed"
    result["reason"] = "nightly docs committed"
    return result


def write_latest_log_copy(source, latest_path):
    try:
        latest_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, latest_path)
        return None
    except OSError as error:
        return str(error)


def stream_subprocess_to_log(command, cwd, log_path):
    with log_path.open("w", encoding="utf-8") as log_file:
        process = subprocess.Popen(
            command,
            cwd=str(cwd),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            bufsize=1,
        )
        assert process.stdout is not None
        for chunk in process.stdout:
            log_file.write(chunk)
        process.stdout.close()
        return process.wait()


def copy_existing_file(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, destination)


def strip_md_code(value):
    value = value.strip()
    if value.startswith("`") and value.endswith("`") and len(value) >= 2:
        return value[1:-1]
    return value


def parse_report_summary(report_path):
    if not report_path.exists():
        return None

    lines = report_path.read_text(encoding="utf-8").splitlines()
    fields = {}
    failed_task_rows = []
    task_rows = []
    selected_groups = []
    failed_groups = []
    current_section = None

    for line in lines:
        if line.startswith("## "):
            current_section = line.strip()
            continue

        if line.startswith("Generated at: "):
            fields["Generated at"] = line.split(":", 1)[1].strip()
            continue

        if line.startswith("- "):
            match = re.match(r"^- ([^:]+): (.+)$", line)
            if match:
                fields[match.group(1)] = match.group(2).strip()
                continue

        if current_section == "## Failed Task Summary" and line.startswith("| `"):
            cells = [strip_md_code(cell.strip()) for cell in line.strip().strip("|").split("|")]
            if len(cells) >= 3:
                failed_task_rows.append(
                    FailedTask(label=cells[0], group=cells[1], duration=cells[2])
                )
            continue

        if current_section == "## Task Results" and line.startswith("| `"):
            cells = [strip_md_code(cell.strip()) for cell in line.strip().strip("|").split("|")]
            if len(cells) >= 4:
                task_rows.append(
                    TaskResult(
                        label=cells[0],
                        group=cells[1],
                        status=cells[2],
                        duration=cells[3],
                    )
                )

    selected_groups = re.findall(r"`([^`]+)`", fields.get("Selected groups", ""))
    failed_groups = re.findall(r"`([^`]+)`", fields.get("Failed groups", ""))

    def parse_int_field(name):
        raw = fields.get(name)
        if raw is None:
            return None
        match = re.search(r"`(\d+)`", raw)
        if not match:
            return None
        return int(match.group(1))

    overall_match = re.search(r"`([^`]+)`", fields.get("Overall status", ""))
    group_rows = []
    group_map = {}
    for row in task_rows:
        if row.group not in group_map:
            group_map[row.group] = {"total": 0, "passed": 0, "failed": 0}
        group_map[row.group]["total"] += 1
        if row.status == "PASS":
            group_map[row.group]["passed"] += 1
        elif row.status == "FAIL":
            group_map[row.group]["failed"] += 1

    ordered_groups = []
    for row in task_rows:
        if row.group not in ordered_groups:
            ordered_groups.append(row.group)
    for group_name in ordered_groups:
        group_rows.append(
            GroupSummary(
                name=group_name,
                total=group_map[group_name]["total"],
                passed=group_map[group_name]["passed"],
                failed=group_map[group_name]["failed"],
            )
        )

    return ReportSummary(
        generated_at=strip_md_code(fields.get("Generated at", "")) or None,
        overall_status=overall_match.group(1) if overall_match else None,
        attempted_tasks=parse_int_field("Attempted tasks"),
        passed_tasks=parse_int_field("Passed tasks"),
        failed_tasks=parse_int_field("Failed tasks"),
        failed_groups=failed_groups,
        selected_groups=selected_groups,
        failed_task_rows=failed_task_rows,
        task_rows=task_rows,
        group_rows=group_rows,
        task_case_rows=[],
        group_case_rows=[],
        delegated_qos_task_rows=[],
    )


def parse_elapsed_duration(raw_value):
    if not raw_value:
        return None
    match = re.search(r"(\d+)s", raw_value)
    if not match:
        return None
    return "{0}s".format(match.group(1))


def parse_tests_line_counts(raw_value):
    counts = {}
    for match in re.finditer(r"(\d+)\s+(passed|failed|skipped|todo|total)", raw_value):
        counts[match.group(2)] = int(match.group(1))
    if "total" not in counts:
        return None
    failed = counts.get("failed", 0)
    return {"total": counts["total"], "failed": failed}


def parse_task_blocks(log_path, report_summary):
    if not report_summary or not report_summary.task_rows or not log_path.exists():
        return []

    top_level_group_map = {row.label: row.group for row in report_summary.task_rows}
    stack = []
    blocks = []

    for raw_line in log_path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw_line.strip()

        open_match = re.match(r"^==>\s+(?:\[(?P<bracket>[^\]]+)\]|(?P<plain>\S+))$", line)
        if open_match:
            label = open_match.group("bracket") or open_match.group("plain")
            if label in top_level_group_map:
                group = top_level_group_map[label]
            elif stack:
                group = stack[-1].group
            else:
                group = None

            block = ParsedTaskBlock(label, group)
            if stack:
                stack[-1].child_count += 1
            stack.append(block)
            continue

        close_match = re.match(
            r"^<==\s+(?:\[(?P<bracket>[^\]]+)\]|(?P<plain>\S+))\s+(?P<status>PASS|FAIL)(?:\s+\((?P<detail>[^)]*)\))?(?:\s+.*)?$",
            line,
        )
        if close_match:
            label = close_match.group("bracket") or close_match.group("plain")
            while stack:
                block = stack.pop()
                if block.label == label:
                    block.status = close_match.group("status")
                    block.duration = parse_elapsed_duration(close_match.group("detail"))
                blocks.append(block)
                if block.label == label:
                    break
            continue

        if not stack:
            continue

        block = stack[-1]

        gtest_total = re.match(r"^\[==========\] Running (\d+) tests? from ", line)
        if gtest_total:
            block.explicit_total = int(gtest_total.group(1))
            if block.explicit_failed is None:
                block.explicit_failed = 0
            continue

        gtest_failed = re.match(r"^\[\s+FAILED\s+\] (\d+) tests?, listed below:", line)
        if gtest_failed:
            block.explicit_failed = int(gtest_failed.group(1))
            continue

        jest_line = re.match(r"^Tests:\s+(.+)$", line)
        if jest_line:
            counts = parse_tests_line_counts(jest_line.group(1))
            if counts:
                block.explicit_total = counts["total"]
                block.explicit_failed = counts["failed"]
            continue

        matrix_line = re.match(
            r"^(?:cpp-client )?matrix summary: passed=(\d+) failed=(\d+) errors=(\d+) total=(\d+)$",
            line,
        )
        if matrix_line:
            block.explicit_total = int(matrix_line.group(4))
            block.explicit_failed = int(matrix_line.group(2)) + int(matrix_line.group(3))
            continue

        downlink_matrix_line = re.match(
            r"^downlink matrix: passed=(\d+) failed=(\d+) errors=(\d+) total=(\d+)$",
            line,
        )
        if downlink_matrix_line:
            block.explicit_total = int(downlink_matrix_line.group(4))
            block.explicit_failed = int(downlink_matrix_line.group(2)) + int(downlink_matrix_line.group(3))
            continue

        ratio_pass_line = re.match(r"^[A-Za-z0-9_]+:\s+(\d+)/(\d+)\s+passed$", line)
        if ratio_pass_line:
            passed = int(ratio_pass_line.group(1))
            total = int(ratio_pass_line.group(2))
            block.explicit_total = total
            block.explicit_failed = total - passed
            continue

        all_cases_line = re.match(r"^[A-Za-z0-9_]+:\s+all\s+(\d+)\s+cases\s+passed$", line)
        if all_cases_line:
            block.explicit_total = int(all_cases_line.group(1))
            block.explicit_failed = 0
            continue

        if re.match(r"^\[PASS\]\s+case\b", line, re.IGNORECASE):
            block.pass_case_lines += 1
            continue

        if re.match(r"^\[FAIL\]\s+case\b", line, re.IGNORECASE):
            block.fail_case_lines += 1
            continue

    while stack:
        blocks.append(stack.pop())

    return blocks


def build_task_case_summaries(log_path, report_summary):
    blocks = parse_task_blocks(log_path, report_summary)
    results = []

    for block in blocks:
        if not block.group:
            continue

        total_cases = block.explicit_total
        failed_cases = block.explicit_failed

        if total_cases is None and (block.pass_case_lines or block.fail_case_lines):
            total_cases = block.pass_case_lines + block.fail_case_lines
            failed_cases = block.fail_case_lines

        if total_cases is None and block.child_count == 0 and block.status in {"PASS", "FAIL"}:
            total_cases = 1
            failed_cases = 1 if block.status == "FAIL" else 0

        if total_cases is None:
            continue

        if failed_cases is None:
            failed_cases = 0

        results.append(
            CaseSummary(
                label=block.label,
                group=block.group,
                total_cases=total_cases,
                failed_cases=failed_cases,
            )
        )

    return results


def build_group_case_summaries(task_case_rows):
    if not task_case_rows:
        return []

    group_map = {}
    ordered_groups = []
    for row in task_case_rows:
        if row.group not in group_map:
            group_map[row.group] = {"total": 0, "failed": 0}
            ordered_groups.append(row.group)
        group_map[row.group]["total"] += row.total_cases
        group_map[row.group]["failed"] += row.failed_cases

    results = []
    for group_name in ordered_groups:
        total = group_map[group_name]["total"]
        failed = group_map[group_name]["failed"]
        results.append(
            CaseSummary(
                label=group_name,
                group=group_name,
                total_cases=total,
                failed_cases=failed,
            )
        )
    return results


def build_delegated_qos_task_rows(log_path, report_summary):
    blocks = parse_task_blocks(log_path, report_summary)
    if not blocks:
        return []

    top_level_labels = {row.label for row in report_summary.task_rows} if report_summary else set()
    helper_labels = {
        "case-report",
        "cpp-client-case-report",
        "downlink-case-report",
        "downlink-report",
    }

    results = []
    seen = set()
    for block in blocks:
        if block.group != "qos":
            continue
        if not block.label or block.label in top_level_labels:
            continue
        if block.label in helper_labels or block.label.startswith("system:") or block.label.startswith("build:"):
            continue
        if block.status not in {"PASS", "FAIL"}:
            continue
        if block.label in seen:
            continue
        seen.add(block.label)
        results.append(
            TaskResult(
                label=block.label,
                group=block.group,
                status=block.status,
                duration=block.duration or "-",
            )
        )

    return results


def parse_failed_cases(log_path):
    if not log_path.exists():
        return []

    patterns = [
        re.compile(r"^case (?P<label>[A-Za-z0-9_.:-]+) failed: (?P<detail>.+)$"),
        re.compile(r"^\[(?P<label>[A-Za-z0-9_.:-]+)\] FAIL (?P<detail>.+)$"),
        re.compile(r"^\[\s+FAILED\s+\] (?P<label>[A-Za-z0-9_:.\/-]+)(?: .*)?$"),
    ]

    seen = set()
    failures = []
    for raw_line in log_path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw_line.strip()
        for pattern in patterns:
            match = pattern.match(line)
            if not match:
                continue
            label = match.group("label")
            if re.match(r"^\d+$", label):
                break
            detail = match.groupdict().get("detail", "") or "no detail"
            key = (label, detail)
            if key in seen:
                break
            seen.add(key)
            failures.append({"label": label, "detail": detail})
            break
    return failures


def snapshot_attachments(run_context, attachment_paths):
    copied = []
    missing = []
    for source_path in attachment_paths:
        if not source_path.exists():
            missing.append(str(source_path))
            continue
        target_path = run_context.attachments_dir / source_path.name
        copy_existing_file(source_path, target_path)
        copied.append(target_path)
    return copied, missing


def build_subject(prefix, overall_status, run_id, case_rows=None):
    clean_prefix = (prefix or DEFAULT_SUBJECT_PREFIX).strip()
    ratio = case_failure_ratio_string(case_rows or [])
    return "{prefix} {status} {ratio} {run_id}".format(
        prefix=clean_prefix,
        status=overall_status_cn(overall_status or "UNKNOWN"),
        ratio=ratio,
        run_id=run_id,
    )


def pass_rate_string(report_summary):
    if not report_summary:
        return "unknown"
    if not report_summary.attempted_tasks or report_summary.passed_tasks is None:
        return "unknown"
    attempted = report_summary.attempted_tasks
    passed = report_summary.passed_tasks
    rate = (passed / attempted) * 100 if attempted > 0 else 0.0
    return f"{rate:.1f}% ({passed}/{attempted})"


def case_failure_ratio_string(case_rows):
    if not case_rows:
        return "未知"
    total = sum(row.total_cases for row in case_rows)
    failed = sum(row.failed_cases for row in case_rows)
    return "{0}/{1}".format(failed, total)


def overall_status_cn(value):
    mapping = {
        "PASS": "成功",
        "FAIL": "失败",
        "UNKNOWN": "未知",
    }
    return mapping.get(value or "UNKNOWN", value or "未知")


def task_status_cn(value):
    mapping = {
        "PASS": "成功",
        "FAIL": "失败",
        "NOT_RUN": "未运行",
    }
    return mapping.get(value or "", value or "未知")


def render_group_summary_lines(report_summary):
    if not report_summary or not report_summary.group_case_rows:
        return ["- 无可用分组数据"]

    lines = []
    for row in report_summary.group_case_rows:
        lines.append(
            "- {name}：{failed}/{total} 失败".format(
                name=row.group,
                failed=row.failed_cases,
                total=row.total_cases,
            )
        )
    return lines


def render_failed_task_lines(report_summary):
    if not report_summary or not report_summary.failed_task_rows:
        return ["- 无"]

    lines = []
    for row in report_summary.failed_task_rows:
        lines.append(
            "- [{group}] {label}（耗时 {duration}）".format(
                group=row.group,
                label=row.label,
                duration=row.duration,
            )
        )
    return lines


def render_delegated_qos_task_lines(report_summary):
    if not report_summary or not report_summary.delegated_qos_task_rows:
        return ["- 无"]

    lines = []
    for row in report_summary.delegated_qos_task_rows:
        lines.append(
            "- {label}：{status}（耗时 {duration}）".format(
                label=row.label,
                status=task_status_cn(row.status),
                duration=row.duration,
            )
        )
    return lines


def render_failed_case_lines(failed_cases):
    if not failed_cases:
        return ["- 无"]

    lines = []
    limited = failed_cases[:MAX_FAILED_CASES_IN_EMAIL]
    for item in limited:
        detail = item["detail"]
        if detail == "no detail":
            detail = "无详细信息"
        lines.append("- {label}：{detail}".format(label=item["label"], detail=detail))
    remaining = len(failed_cases) - len(limited)
    if remaining > 0:
        lines.append("- 其余 {count} 条失败 case 已省略".format(count=remaining))
    return lines


def render_attachment_lines(run_context, missing_attachments):
    existing = sorted(run_context.attachments_dir.glob("*.md"))
    lines = []
    if existing:
        for attachment in existing:
            lines.append("- {name}".format(name=attachment.name))
    else:
        lines.append("- 无可用附件")
    if missing_attachments:
        lines.append("")
        lines.append("缺失的 Markdown 附件：")
        for item in missing_attachments:
            lines.append("- {path}".format(path=item))
    return lines


def render_email_html(
    run_context,
    exit_code,
    report_summary,
    failed_cases,
    missing_attachments,
    latest_log_path,
    latest_log_error,
    git_record_result,
):
    overall_status = report_summary.overall_status if report_summary else ("PASS" if exit_code == 0 else "FAIL")
    status_cn = overall_status_cn(overall_status)
    group_rows = report_summary.group_case_rows if report_summary else []
    failed_task_rows = report_summary.failed_task_rows if report_summary else []
    delegated_qos_rows = report_summary.delegated_qos_task_rows if report_summary else []
    limited_failed_cases = failed_cases[:MAX_FAILED_CASES_IN_EMAIL]
    attachment_names = [path.name for path in sorted(run_context.attachments_dir.glob("*.md"))]

    def esc(value):
        return html.escape(str(value))

    summary_items = [
        ("整体状态", status_cn),
        ("失败 case 数", case_failure_ratio_string(report_summary.group_case_rows if report_summary else [])),
        ("退出码", exit_code),
        ("生成时间", report_summary.generated_at if report_summary else "未知"),
        ("选中分组", "、".join(report_summary.selected_groups) if report_summary and report_summary.selected_groups else "未知"),
        ("失败分组", "、".join(report_summary.failed_groups) if report_summary and report_summary.failed_groups else "无"),
        ("运行目录", run_context.run_dir),
        ("原始日志", run_context.log_path),
        ("最新日志", "失败（{0}）".format(latest_log_error) if latest_log_error else latest_log_path),
        (
            "文档记录",
            "已提交 {0}".format(git_record_result.get("commitSha"))
            if git_record_result and git_record_result.get("status") == "committed"
            else (git_record_result.get("reason") if git_record_result else "未知"),
        ),
    ]

    html_parts = [
        "<html><body style=\"font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Arial,sans-serif;color:#1f2937;line-height:1.6;\">",
        "<h2 style=\"margin-bottom:8px;\">mediasoup-cpp 夜间全量回归结果：{0}</h2>".format(esc(status_cn)),
        "<table style=\"border-collapse:collapse;margin:12px 0 20px 0;min-width:640px;\">",
    ]
    for key, value in summary_items:
        html_parts.append(
            "<tr>"
            "<td style=\"border:1px solid #d1d5db;padding:8px 12px;background:#f9fafb;font-weight:600;\">{0}</td>"
            "<td style=\"border:1px solid #d1d5db;padding:8px 12px;\">{1}</td>"
            "</tr>".format(esc(key), esc(value))
        )
    html_parts.append("</table>")

    html_parts.append("<h3 style=\"margin:20px 0 8px 0;\">分组结果</h3>")
    if group_rows:
        html_parts.append("<table style=\"border-collapse:collapse;min-width:480px;\">")
        html_parts.append(
            "<tr>"
            "<th style=\"border:1px solid #d1d5db;padding:8px 12px;background:#eef2ff;text-align:left;\">分组</th>"
            "<th style=\"border:1px solid #d1d5db;padding:8px 12px;background:#eef2ff;text-align:left;\">失败/总数</th>"
            "</tr>"
        )
        for row in group_rows:
            html_parts.append(
                "<tr>"
                "<td style=\"border:1px solid #d1d5db;padding:8px 12px;\">{name}</td>"
                "<td style=\"border:1px solid #d1d5db;padding:8px 12px;\">{failed}/{total}</td>"
                "</tr>".format(
                    name=esc(row.group),
                    total=row.total_cases,
                    failed=row.failed_cases,
                )
            )
        html_parts.append("</table>")
    else:
        html_parts.append("<p>无可用分组数据。</p>")

    html_parts.append("<h3 style=\"margin:20px 0 8px 0;\">失败任务</h3>")
    if failed_task_rows:
        html_parts.append("<ul>")
        for row in failed_task_rows:
            html_parts.append(
                "<li><strong>{group}</strong> / {label}（耗时 {duration}）</li>".format(
                    group=esc(row.group),
                    label=esc(row.label),
                    duration=esc(row.duration),
                )
            )
        html_parts.append("</ul>")
    else:
        html_parts.append("<p>无。</p>")

    html_parts.append("<h3 style=\"margin:20px 0 8px 0;\">QoS 委托子任务</h3>")
    if delegated_qos_rows:
        html_parts.append("<table style=\"border-collapse:collapse;min-width:640px;\">")
        html_parts.append(
            "<tr>"
            "<th style=\"border:1px solid #d1d5db;padding:8px 12px;background:#eef2ff;text-align:left;\">任务</th>"
            "<th style=\"border:1px solid #d1d5db;padding:8px 12px;background:#eef2ff;text-align:left;\">状态</th>"
            "<th style=\"border:1px solid #d1d5db;padding:8px 12px;background:#eef2ff;text-align:left;\">耗时</th>"
            "</tr>"
        )
        for row in delegated_qos_rows:
            html_parts.append(
                "<tr>"
                "<td style=\"border:1px solid #d1d5db;padding:8px 12px;\">{label}</td>"
                "<td style=\"border:1px solid #d1d5db;padding:8px 12px;\">{status}</td>"
                "<td style=\"border:1px solid #d1d5db;padding:8px 12px;\">{duration}</td>"
                "</tr>".format(
                    label=esc(row.label),
                    status=esc(task_status_cn(row.status)),
                    duration=esc(row.duration),
                )
            )
        html_parts.append("</table>")
    else:
        html_parts.append("<p>无。</p>")

    html_parts.append("<h3 style=\"margin:20px 0 8px 0;\">失败 case（按原始日志尽力提取）</h3>")
    if limited_failed_cases:
        html_parts.append("<ul>")
        for item in limited_failed_cases:
            detail = item["detail"]
            if detail == "no detail":
                detail = "无详细信息"
            html_parts.append(
                "<li><strong>{label}</strong>：{detail}</li>".format(
                    label=esc(item["label"]),
                    detail=esc(detail),
                )
            )
        if len(failed_cases) > len(limited_failed_cases):
            html_parts.append(
                "<li>其余 {count} 条失败 case 已省略</li>".format(
                    count=len(failed_cases) - len(limited_failed_cases)
                )
            )
        html_parts.append("</ul>")
    else:
        html_parts.append("<p>无。</p>")

    html_parts.append("<h3 style=\"margin:20px 0 8px 0;\">邮件附件</h3>")
    if attachment_names:
        html_parts.append("<ul>")
        for name in attachment_names:
            html_parts.append("<li>{0}</li>".format(esc(name)))
        html_parts.append("</ul>")
    else:
        html_parts.append("<p>无可用附件。</p>")

    if missing_attachments:
        html_parts.append("<h3 style=\"margin:20px 0 8px 0;\">缺失附件</h3><ul>")
        for item in missing_attachments:
            html_parts.append("<li>{0}</li>".format(esc(item)))
        html_parts.append("</ul>")

    if git_record_result:
        html_parts.append("<h3 style=\"margin:20px 0 8px 0;\">文档 git 记录</h3><ul>")
        html_parts.append("<li>状态：{0}</li>".format(esc(git_record_result.get("status", "unknown"))))
        if git_record_result.get("reason"):
            html_parts.append("<li>说明：{0}</li>".format(esc(git_record_result["reason"])))
        if git_record_result.get("commitMessage"):
            html_parts.append("<li>提交消息：{0}</li>".format(esc(git_record_result["commitMessage"])))
        if git_record_result.get("preexistingDirtyDocPaths"):
            html_parts.append(
                "<li>运行前已脏文档：{0}</li>".format(
                    esc("、".join(git_record_result["preexistingDirtyDocPaths"]))
                )
            )
        html_parts.append("</ul>")

    html_parts.append("</body></html>")
    return "".join(html_parts)


def render_email_body(
    run_context,
    exit_code,
    report_summary,
    failed_cases,
    missing_attachments,
    latest_log_path,
    latest_log_error,
    git_record_result,
):
    overall_status = report_summary.overall_status if report_summary else ("PASS" if exit_code == 0 else "FAIL")
    status_cn = overall_status_cn(overall_status)
    lines = [
        "mediasoup-cpp 夜间全量回归结果：{0}".format(status_cn),
        "",
        "基本信息",
        "- 运行批次：{0}".format(run_context.run_id),
        "- 仓库路径：{0}".format(run_context.repo_root),
        "- 运行目录：{0}".format(run_context.run_dir),
        "- 原始日志：{0}".format(run_context.log_path),
    ]

    if latest_log_error:
        lines.append("- 最新日志：失败（{0}）".format(latest_log_error))
    else:
        lines.append("- 最新日志：{0}".format(latest_log_path))

    lines.extend(
        [
            "",
            "总览",
            "- 整体状态：{0}".format(status_cn),
            "- 失败 case 数：{0}".format(case_failure_ratio_string(report_summary.group_case_rows if report_summary else [])),
            "- 退出码：{0}".format(exit_code),
        ]
    )

    if report_summary:
        lines.append("- 报告生成时间：{0}".format(report_summary.generated_at or "未知"))
        lines.append(
            "- 选中分组：{0}".format(
                "、".join(report_summary.selected_groups) if report_summary.selected_groups else "未知"
            )
        )
        lines.append(
            "- 失败分组：{0}".format(
                "、".join(report_summary.failed_groups) if report_summary.failed_groups else "无"
            )
        )
        if report_summary.group_case_rows:
            total_cases = sum(row.total_cases for row in report_summary.group_case_rows)
            failed_case_count = sum(row.failed_cases for row in report_summary.group_case_rows)
            lines.append("- 总 case 数：{0}".format(total_cases))
            lines.append("- 失败 case 数：{0}".format(failed_case_count))
    else:
        lines.append("- 结构化报告：不可用，本次摘要来自原始日志和 wrapper 元数据")

    lines.append("")
    lines.append("分组结果")
    lines.extend(render_group_summary_lines(report_summary))

    lines.append("")
    lines.append("失败任务")
    lines.extend(render_failed_task_lines(report_summary))

    lines.append("")
    lines.append("QoS 委托子任务")
    lines.extend(render_delegated_qos_task_lines(report_summary))

    lines.append("")
    lines.append("失败 case（按原始日志尽力提取）")
    lines.extend(render_failed_case_lines(failed_cases))

    lines.append("")
    lines.append("邮件附件")
    lines.extend(render_attachment_lines(run_context, missing_attachments))

    if git_record_result:
        lines.append("")
        lines.append("文档 git 记录")
        lines.append("- 状态：{0}".format(git_record_result.get("status", "unknown")))
        if git_record_result.get("reason"):
            lines.append("- 说明：{0}".format(git_record_result["reason"]))
        if git_record_result.get("commitMessage"):
            lines.append("- 提交消息：{0}".format(git_record_result["commitMessage"]))
        if git_record_result.get("commitSha"):
            lines.append("- 提交哈希：{0}".format(git_record_result["commitSha"]))
        if git_record_result.get("preexistingDirtyDocPaths"):
            lines.append(
                "- 运行前已脏文档：{0}".format(
                    "、".join(git_record_result["preexistingDirtyDocPaths"])
                )
            )

    return "\n".join(lines) + "\n"


def build_email_message(
    subject,
    body,
    html_body,
    sender,
    recipients,
    attachments,
):
    message = EmailMessage()
    message["Subject"] = subject
    message["From"] = sender
    message["To"] = ", ".join(recipients)
    message.set_content(body)
    if html_body:
        message.add_alternative(html_body, subtype="html")
    for attachment in attachments:
        payload = attachment.read_bytes()
        message.add_attachment(
            payload,
            maintype="text",
            subtype="markdown",
            filename=attachment.name,
        )
    return message


def send_via_smtp(config, message, recipients):
    host = config.get("smtp_host")
    if not host:
        return MailResult("smtp", False, "SMTP_HOST is not configured")

    port = int(config["smtp_port"])
    timeout = int(config["smtp_timeout_seconds"])
    username = config.get("smtp_username")
    password = config.get("smtp_password")
    use_ssl = bool(config["smtp_use_ssl"])
    use_tls = bool(config["smtp_use_tls"])

    try:
        if use_ssl:
            server = smtplib.SMTP_SSL(str(host), port, timeout=timeout)
        else:
            server = smtplib.SMTP(str(host), port, timeout=timeout)
        with server:
            if use_tls and not use_ssl:
                server.starttls()
            if username:
                server.login(str(username), str(password or ""))
            server.send_message(message, to_addrs=recipients)
        return MailResult("smtp", True, f"delivered via {host}:{port}")
    except Exception as error:  # noqa: BLE001
        return MailResult("smtp", False, str(error))


def send_via_mailx(subject, body, sender, recipients, attachments):
    mailx = shutil.which("mailx")
    if not mailx:
        return MailResult("mailx", False, "mailx not found in PATH")

    command = [mailx, "-s", subject]
    if sender:
        command += ["-r", sender]
    for attachment in attachments:
        command += ["-a", str(attachment)]
    command += recipients

    try:
        completed = subprocess.run(
            command,
            input=body,
            universal_newlines=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
    except OSError as error:
        return MailResult("mailx", False, str(error))

    if completed.returncode == 0:
        return MailResult("mailx", True, "delivered via mailx")

    stderr = completed.stderr.strip() or completed.stdout.strip() or f"mailx exited {completed.returncode}"
    return MailResult("mailx", False, stderr)


def send_email(
    config,
    subject,
    body,
    attachments,
    no_mail,
):
    recipients = list(config["mail_to"])
    sender = str(config["mail_from"] or "")

    if no_mail:
        return MailResult("disabled", False, "--no-mail was set")

    if not recipients:
        return MailResult("none", False, "MAIL_TO is not configured")

    transport = str(config["mail_transport"])
    html_body = render_email_html(
        config["run_context"],
        config["run_exit_code"],
        config["report_summary"],
        config["failed_cases"],
        config["missing_attachments"],
        config["latest_log_path"],
        config["latest_log_error"],
        config["git_record_result"],
    )
    message = build_email_message(
        subject,
        body,
        html_body,
        sender or recipients[0],
        recipients,
        attachments,
    )

    if transport == "smtp":
        result = send_via_smtp(config, message, recipients)
        if result.delivered or not shutil.which("mailx"):
            return result
        fallback = send_via_mailx(subject, body, sender, recipients, attachments)
        if fallback.delivered:
            return fallback
        return MailResult(
            "smtp+mailx",
            False,
            f"SMTP failed: {result.detail}; mailx failed: {fallback.detail}",
        )

    if transport == "mailx":
        return send_via_mailx(subject, body, sender, recipients, attachments)

    return MailResult(transport, False, f"unsupported MAIL_TRANSPORT: {transport}")


def write_json(path, payload):
    path.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def relative_or_absolute(repo_root, path):
    try:
        return str(path.relative_to(repo_root))
    except ValueError:
        return str(path)


def handle_run(args):
    repo_root = Path(__file__).resolve().parent.parent
    config_path = abs_path(repo_root, args.config, DEFAULT_CONFIG_PATH)
    config = build_runtime_config(repo_root, config_path)
    run_context = create_run_context(repo_root, config)
    pruned_artifact_run_dirs = prune_timestamped_run_dirs(
        Path(config["artifact_root"]),
        int(config["nightly_max_backup_runs"]),
    )
    preexisting_doc_paths = git_status_paths(repo_root, ["docs", "README.md"])
    preexisting_staged_paths = git_staged_paths(repo_root)

    exit_code = 0
    latest_log_error = None
    source_log_path = Path(args.source_log).resolve() if args.source_log else None
    source_report_path = Path(args.source_report).resolve() if args.source_report else Path(config["report_path"])

    if args.skip_tests:
        if source_log_path and not source_log_path.exists():
            raise FileNotFoundError(f"source log not found: {source_log_path}")
        if source_log_path:
            copy_existing_file(source_log_path, run_context.log_path)
        else:
            run_context.log_path.write_text(
                "nightly_full_regression.py: --skip-tests used without --source-log\n",
                encoding="utf-8",
            )
    else:
        command = [str(config["run_all_tests_path"])] + list(config["run_all_tests_args"])
        exit_code = stream_subprocess_to_log(command, repo_root, run_context.log_path)
        source_report_path = Path(config["report_path"])

    latest_log_error = write_latest_log_copy(run_context.log_path, Path(config["latest_log_path"]))

    attachments, missing_attachments = snapshot_attachments(
        run_context,
        [Path(path) for path in config["attachment_paths"]],
    )
    report_summary = parse_report_summary(source_report_path)
    if report_summary:
        report_summary.task_case_rows = build_task_case_summaries(run_context.log_path, report_summary)
        report_summary.group_case_rows = build_group_case_summaries(report_summary.task_case_rows)
        report_summary.delegated_qos_task_rows = build_delegated_qos_task_rows(
            run_context.log_path,
            report_summary,
        )
    if args.skip_tests and exit_code == 0 and report_summary and report_summary.overall_status == "FAIL":
        exit_code = 1
    failed_cases = parse_failed_cases(run_context.log_path)
    overall_status = report_summary.overall_status if report_summary else ("PASS" if exit_code == 0 else "FAIL")
    git_record_result = record_changed_docs_in_git(
        repo_root,
        run_context,
        bool(config["nightly_git_commit_docs"]),
        args.skip_tests,
        preexisting_doc_paths,
        preexisting_staged_paths,
        overall_status,
    )
    body = render_email_body(
        run_context,
        exit_code,
        report_summary,
        failed_cases,
        missing_attachments,
        str(config["latest_log_path"]),
        latest_log_error,
        git_record_result,
    )
    run_context.email_body_path.write_text(body, encoding="utf-8")

    subject = build_subject(
        str(config["mail_subject_prefix"] or DEFAULT_SUBJECT_PREFIX),
        overall_status,
        run_context.run_id,
        report_summary.group_case_rows if report_summary else [],
    )
    config["run_context"] = run_context
    config["run_exit_code"] = exit_code
    config["report_summary"] = report_summary
    config["failed_cases"] = failed_cases
    config["missing_attachments"] = missing_attachments
    config["latest_log_error"] = latest_log_error
    config["git_record_result"] = git_record_result
    mail_result = send_email(config, subject, body, attachments, args.no_mail)

    summary_payload = {
        "runId": run_context.run_id,
        "repoRoot": str(repo_root),
        "configPath": relative_or_absolute(repo_root, Path(config["config_path"])),
        "exitCode": exit_code,
        "logPath": str(run_context.log_path),
        "latestLogPath": str(config["latest_log_path"]),
        "latestLogCopyError": latest_log_error,
        "prunedArtifactRunDirs": pruned_artifact_run_dirs,
        "reportPath": str(source_report_path),
        "gitDocRecording": git_record_result,
        "mail": {
            "transport": mail_result.transport,
            "delivered": mail_result.delivered,
            "detail": mail_result.detail,
            "subject": subject,
            "recipients": list(config["mail_to"]),
        },
        "attachments": [str(path) for path in attachments],
        "missingAttachments": missing_attachments,
        "failedCases": failed_cases,
        "reportSummary": None
        if report_summary is None
        else {
            "generatedAt": report_summary.generated_at,
            "overallStatus": report_summary.overall_status,
            "attemptedTasks": report_summary.attempted_tasks,
            "passedTasks": report_summary.passed_tasks,
            "failedTasks": report_summary.failed_tasks,
            "failedGroups": report_summary.failed_groups,
            "selectedGroups": report_summary.selected_groups,
            "taskCaseRows": [
                {
                    "label": row.label,
                    "group": row.group,
                    "totalCases": row.total_cases,
                    "failedCases": row.failed_cases,
                }
                for row in report_summary.task_case_rows
            ],
            "groupCaseRows": [
                {
                    "group": row.group,
                    "totalCases": row.total_cases,
                    "failedCases": row.failed_cases,
                }
                for row in report_summary.group_case_rows
            ],
            "failedTaskRows": [
                {
                    "label": row.label,
                    "group": row.group,
                    "duration": row.duration,
                }
                for row in report_summary.failed_task_rows
            ],
            "delegatedQosTaskRows": [
                {
                    "label": row.label,
                    "group": row.group,
                    "status": row.status,
                    "duration": row.duration,
                }
                for row in report_summary.delegated_qos_task_rows
            ],
        },
    }
    write_json(run_context.summary_path, summary_payload)

    if args.print_summary:
        print(body, end="")

    if args.no_mail:
        return exit_code
    if mail_result.delivered:
        return exit_code
    return exit_code or 1


def main():
    args = parse_args()
    if args.command == "run":
        return handle_run(args)
    raise ValueError(f"unsupported command: {args.command}")


if __name__ == "__main__":
    sys.exit(main())
