# This file is part of TestCov,
# a robust test executor with reliable coverage measurement:
# https://gitlab.com/sosy-lab/software/test-suite-validator/
#
# Copyright (C) 2018 - 2020  Dirk Beyer
# SPDX-FileCopyrightText: 2019 Dirk Beyer <https://www.sosy-lab.org>
#
# SPDX-License-Identifier: Apache-2.0

import subprocess
import tempfile
import time

import json
from pathlib import Path

import logging

ERROR_STRING = "Error found."


class ParseError(Exception):
    def __init__(self, msg=None, cause=None):
        super().__init__(msg, cause)
        self.msg = msg
        self.cause = cause


class CoverFunc:
    def __init__(self, target_method):
        self.target_method = target_method

    def __str__(self):
        return f"{type(self).__name__}({self.target_method})"

    def __repr__(self):
        return str(self)


COVER_LINES = "COVER( init(main()), FQL(COVER EDGES(@BASICBLOCKENTRY)) )"
COVER_BRANCHES = "COVER( init(main()), FQL(COVER EDGES(@DECISIONEDGE)) )"
COVER_CONDITIONS = "COVER( init(main()), FQL(COVER EDGES(@CONDITIONEDGE)) )"

COVERAGE_GOALS = {
    "@DECISIONEDGE": COVER_BRANCHES,
    "@CONDITIONEDGE": COVER_CONDITIONS,
    "@BASICBLOCKENTRY": COVER_LINES,
    "@CALL(__VERIFIER_error)": CoverFunc("__VERIFIER_error"),
    "@CALL(reach_error)": CoverFunc("reach_error"),
}

MACHINE_MODEL_32 = "-m32"
MACHINE_MODEL_64 = "-m64"

COMPILER = "gcc"

EXTERNAL_DECLARATIONS = [
    ("_IO_FILE", "struct _IO_FILE;", "#include<stdio.h>"),
    ("FILE", "typedef struct _IO_FILE FILE;", "#include<stdio.h>"),
    ("stdin", "extern struct _IO_FILE *stdin;", "#include<stdio.h>"),
    ("stderr", "extern struct _IO_FILE *stderr;", "#include<stdio.h>"),
    ("size_t", "typedef long unsigned int size_t;", "#include<stddef.h>"),
    (
        "abort",
        "extern void abort (void) __attribute__ ((__nothrow__ , __leaf__))"
        + " __attribute__ ((__noreturn__));",
        "#include<stdlib.h>",
    ),
    (
        "exit",
        "extern void exit (int __status) __attribute__ ((__nothrow__ , __leaf__))"
        + " __attribute__ ((__noreturn__));",
        "#include<stdlib.h>",
    ),
    (
        "fgets",
        "extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream);",
        "#include<stdio.h>",
    ),
    (
        "sscanf",
        "extern int sscanf (const char *__restrict __s, const char *__restrict __format, ...)"
        + " __attribute__ ((__nothrow__ , __leaf__));",
        "#include<stdio.h>",
    ),
    (
        "strlen",
        " extern size_t strlen (const char *__s __attribute__ ((__nothrow__ , __leaf__))"
        + " __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1))));",
        "#include<string.h>",
    ),
    (
        "fprintf",
        "extern int fprintf (FILE *__restrict __stream, const char *__restrict __format, ...);",
        "#include<stdio.h>",
    ),
    (
        "malloc",
        " extern void *malloc (size_t __size __attribute__ ((__nothrow__ , __leaf__))"
        + " __attribute__ ((__malloc__)));",
        "#include<stdlib.h>",
    ),
    (
        "memcpy",
        " extern void *memcpy (void *__restrict __dest, const void *__restrict __src, size_t __n)"
        + " __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));",
        "#include<stdlib.h>",
    ),
    (
        "strcpy",
        " extern char *strcpy (char *__restrict __dest, const char *__restrict __src)"
        + " __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));",
        "#include<string.h>",
    ),
    (
        "strcat",
        " extern char *strcat (char *__restrict __dest, const char *__restrict __src)"
        + " __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__nonnull__ (1, 2)));",
        "#include<string.h>",
    ),
    (
        "strchr",
        "extern char *strchr (const char *__s, int __c)"
        + " __attribute__ ((__nothrow__ , __leaf__)) __attribute__ ((__pure__)) __attribute__ ((__nonnull__ (1)));",
        "#include<string.h>",
    ),
]

COVERS = "false"
UNKNOWN = "unknown"
ERROR = "error"
ABORTED = "abort"

class ExecutionResult:
    """Results of a subprocess execution."""

    def __init__(
        self, returncode, stdout, stderr, got_aborted, cpu_time, wall_time, memory_used
    ):
        self.returncode = returncode
        self.stdout = stdout
        self.stderr = stderr
        self.got_aborted = got_aborted
        self.cpu_time = cpu_time
        self.wall_time = wall_time
        self.memory_used = memory_used

    def __repr__(self):
        return f"{self.__class__.__name__}({self.__hash__()})"

    def __str__(self):
        return f"returncode {self.returncode}"


def execute(command, quiet=False, input_str=None, timelimit=None):
    if input_str is not None and not isinstance(input_str, bytes):
        input_str = input_str.encode()

    with tempfile.TemporaryFile() as input_file:
        if input_str is None:
            # If we have no input, use no input file.
            input_file = None
        else:
            # Write the input to a file so that we have an EOF marker after all content
            # is read. This is necessary to make test executables abort
            # when no more inputs are available. With stdin as a pipe,
            # the test executable will wait for a new input until the timeout expires
            # and there is no way to differentiate missing inputs
            # and long-running executions.
            input_file.write(input_str if input_str is not None else b"")
            input_file.flush()
            input_file.seek(0)

        return _execute(command, input_file, quiet=quiet, timelimit=timelimit)


def _execute(command, input_file, quiet=False, timelimit=None):
    def shut_down(process):
        process.kill()
        return process.wait()

    log_cmd = logging.debug if quiet else logging.info
    log_cmd(" ".join(command))

    wall_time_start = time.perf_counter()
    
    with subprocess.Popen(
        command,
        stdin=input_file,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=False,
    ) as process:
        output = None
        err_output = None
        wall_time = None
        try:
            output, err_output = process.communicate(
                timeout=timelimit if timelimit else None
            )
            returncode = process.poll()
            got_aborted = False
        except subprocess.TimeoutExpired:
            logging.debug("Timeout of %ss expired. Killing process.", timelimit)
            returncode = shut_down(process)
            got_aborted = True
    wall_time = time.perf_counter() - wall_time_start

    # We decode output, but we can't decode error output, since it may contain undecodable bytes.
    try:
        output = output.decode() if output else ""
    except UnicodeDecodeError as e:
        # fail silently, continue with encoded output
        logging.info(e, exc_info=True)

    if output:
        logging.debug("Output of execution:\n%s", snip(output))
    if err_output:
        try:
            err_output_for_msg = err_output.decode() if err_output else ""
        except UnicodeDecodeError:
            # fail silently, continue with encoded output
            err_output_for_msg = err_output
        logging.debug("Error output of execution:\n%s", snip(err_output_for_msg))

    return ExecutionResult(
        returncode, output, err_output, got_aborted, None, wall_time, None
    )


def snip(s, max_len=20):
    lines = s.splitlines()
    if len(lines) <= max_len:
        return s
    head = "\n".join(lines[: max_len // 2])
    tail = "\n".join(lines[-max_len // 2 :])
    if head[-1] != "\n":
        head += "\n"
    if tail[0] != "\n":
        tail = "\n" + tail
    return head + "... snip ..." + tail


def uses_line_coverage(goal):
    return goal == COVER_LINES


def uses_branch_coverage(goal):
    return goal == COVER_BRANCHES or isinstance(goal, CoverFunc)


def uses_condition_coverage(goal):
    return goal == COVER_CONDITIONS

def form_compiliation_commands(cmd: list, file: Path) -> Path:
    path_to_file: Path = file.resolve()

    cmd.pop()
    cmd.append(path_to_file)
    
    compilation_commands_content = json.dumps(
        [
            {
                "directory": str(path_to_file.parent),
                "arguments": list(map(str, cmd)),
                "file": path_to_file.name
            }
        ]
    )
    
    path_compilation_commands: str = tempfile.mktemp()
    with open(path_compilation_commands, "w+") as file_compilation_commands:
        file_compilation_commands.write(compilation_commands_content)
    return path_compilation_commands
