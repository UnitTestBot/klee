# This file is part of TestCov,
# a robust test executor with reliable coverage measurement:
# https://gitlab.com/sosy-lab/software/test-suite-validator/
#
# Copyright (C) 2018 - 2020  Dirk Beyer
# SPDX-FileCopyrightText: 2019 Dirk Beyer <https://www.sosy-lab.org>
#
# SPDX-License-Identifier: Apache-2.0

"""
Reducer of C program. Create a residual program from an input program and a set of relevant labels.
"""

# pylint: disable=C0103
# to disable snake_case error

import os
import re
import tempfile
from typing import List, Sequence, Optional, Union
from pathlib import Path
import logging
 
from src_tooling_runner import execution_utils as eu


LABEL_PREFIX = "Goal_"
GOTO_PREFIX = "goto " + LABEL_PREFIX


def _preprocess(input_program: str, machine_model: str) -> str:
    """
    Pre-processes the given program.

    :param input_program: program to pre-process
    :param machine_model: machine-model to use for pre-processing
    :return: name of new, pre-processed program file
    """
    preprocessed_file: str = tempfile.mktemp()
    # preprocessed_file = input_program + ".i"
    cmd = [eu.COMPILER, machine_model, "-E", input_program, "-o", preprocessed_file]
    eu.execute(cmd, quiet=True)

    return preprocessed_file


def _get_label_adder_params(coverage_goal) -> Optional[List[str]]:
    if isinstance(coverage_goal, eu.CoverFunc):
        return [
            "--function-call-only",
            f"--function-call={coverage_goal.target_method}",
        ]
    if eu.uses_branch_coverage(coverage_goal):
        return [
            "--labels-branching-only",
            "--labels-switch-only",
            "--labels-ternary-only",
        ]
    return None


def instrument_program(
    cmd, input_program: str, machine_model: str, output_program: str, coverage_goal
):
    if not input_program.endswith(
        ".i"
    ):  # very simple heuristic to decide whether program is preprocessed
        input_program = _preprocess(input_program, machine_model)
    c_code = _get_content(input_program)
    # C function reach_error may contain an arbitrary definition - replace this with an exit() call
    # Q: Only do this if coverage goal is reach_error?
    
    # NOTE: this instrumentation required only for test case reproduction
    # try:
    #     target_method = coverage_goal.target_method
    # except AttributeError:
    #     logging.debug("No target method, so not replacing any")
    # else:
    #     c_code = replace_reach_error(target_method, c_code)

    logging.debug("Adding braces to all control-flow statements, if missing")
    # add braces around all if-, else-, while, for-blocks,
    # so that we can add code to them without
    # caring about changing the control-flow semantics.
    c_code = add_controlflow_braces(c_code)

    logging.debug("Adding program labels")
    adder_params = _get_label_adder_params(coverage_goal)
    if adder_params:
        c_code = _call_label_adder(cmd, adder_params, c_code)

    lines = c_code.split("\n")
    lines = add_gcov_flushes(lines)
    lines = add_goto_goals(lines)
        
    # If we keep preprocessor comments, gcov and lcov may use these to deduce the original file name.
    # While this is nice in general, we already manage the original file name separately, for all goal types.
    # So we remove the comments here to avoid the additional special case where the file name
    # in the gcov file does not match the file name of the transformed file used for compilation.
    lines = remove_preprocessor_comments(lines)
    c_code = "\n".join(lines)

    with open(output_program, "w+", encoding="UTF-8") as outp:
        outp.write(c_code)
        logging.debug("Wrote transformed C program to %s", output_program)


def _call_label_adder(cmd, parameters: Sequence[str], input_content: str):
    with tempfile.NamedTemporaryFile(mode="w", encoding="UTF-8", suffix=".c") as tmp:
        compiliation_commands_path: Path | None = None
        if cmd is not None:
            compiliation_commands_path: Path = eu.form_compiliation_commands(cmd, Path(tmp.name))
    
        tmp.write(input_content)
        tmp.flush()
        cmd = [_get_label_adder_bin(), *parameters, tmp.name]
        if compiliation_commands_path is not None:
            cmd += ["-p", str(compiliation_commands_path)]

        exec_result = eu.execute(cmd, quiet=True)
        return exec_result.stdout


def _get_label_adder_bin() -> str:
    return str(Path(__file__).parent.parent / "label-adder")


def _get_content(program: str) -> str:
    with open(program, encoding="UTF-8") as inp:
        return inp.read()

def replace_reach_error(
    function_name, content: Union[str, Sequence[str]]
) -> Sequence[str]:
    logging.debug("Replacing target function %s with exit()", function_name)
    if isinstance(content, str):
        content = content.splitlines()
    new_content = []
    in_reach_error = False
    contains_reach_error = re.compile(f".*void {function_name}.*")
    single_line_reach_error = re.compile(
        r"^\s*void " + function_name + r"\s*\(.*\)\s*{.*}\s*$"
    )
    multiline_reach_error = re.compile(r"^\s*void " + function_name + r".*{\s*$")
    idx = None
    for idx, line in enumerate(content):
        if in_reach_error:
            if re.match(r"^\s*}\s*$", line):
                break
        elif contains_reach_error.match(line):
            new_content.append("extern void exit (int __status);\n")
            new_content.append("void " + function_name + "() { exit(1); }\n")
            if single_line_reach_error.match(line):
                break
            if multiline_reach_error.match(line):
                in_reach_error = True
            else:
                logging.warning("Unmatched occurence of %s: %s", function_name, line)
        else:
            new_content.append(line)

    if idx is not None:
        new_content += content[(idx + 1) :]

    return "\n".join(new_content)


def add_controlflow_braces(c_code: str):
    """
    Add braces around all if-, else-, while, for-blocks.
    Requires clang-tidy to be installed.
    """
    with tempfile.NamedTemporaryFile(mode="w", suffix=".c", delete=False) as tmp:
        tmp.write(c_code)
        tmp_file = tmp.name
    try:
        cmd = [
            "clang-tidy",
            "--checks=-*,readability-braces-around-statements",
            "-fix-errors",
            tmp_file,
        ]
        eu.execute(cmd, quiet=True)
        with open(tmp_file, encoding="UTF-8") as inp:
            return inp.read()
    finally:
        os.remove(tmp_file)


def add_gcov_flushes(content: Sequence[str]) -> Sequence[str]:
    new_content = ["#ifdef GCOV", "extern void __gcov_dump(void);", "#endif"]
    abort_pattern = re.compile(r"(^|\s+|;|\{)abort\(\);")
    for line in content:
        if abort_pattern.search(line):
            line = re.sub(
                r"(\s*)abort\(\);",
                r"\1{\n\1#ifdef GCOV\n\1__gcov_dump();\n\1#endif\n\1abort();\n\1}",
                line,
            )
        if "__assert_fail" in line and not re.search(r"void[^\{]*__assert_fail", line):
            line = re.sub(
                r"(\s*)__assert_fail",
                r"\n#ifdef GCOV\n\1__gcov_dump();\n\1#endif\n\1__assert_fail",
                line,
            )

        new_content.append(line)
    return new_content


def add_goto_goals(content: Sequence[str]) -> Sequence[str]:
    goal_regex = re.compile(f"({LABEL_PREFIX}[0-9]+):")
    new_content = []
    for line in content:
        match = goal_regex.search(line)
        if match:
            new_content.append(f"goto {match.group(1)};")
        new_content.append(line)
    return new_content


def remove_preprocessor_comments(content: Sequence[str]) -> Sequence[str]:
    return [line for line in content if not line.strip().startswith("# ")]
