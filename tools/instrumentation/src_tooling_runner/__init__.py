# This file is part of TestCov,
# a robust test executor with reliable coverage measurement:
# https://gitlab.com/sosy-lab/software/test-suite-validator/
#
# SPDX-FileCopyrightText: 2019-2020 Dirk Beyer <https://www.sosy-lab.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Main module of testcov."""

import argparse
import os
import re
import sys
from src_tooling_runner import execution
from src_tooling_runner import execution_utils as eu
from src_tooling_runner import _tool_info
from pathlib import Path

import logging


class IllegalArgumentError(Exception):
    pass


class StorePath(argparse.Action):
    def __init__(self, option_strings, dest, nargs=None, **kwargs):
        super().__init__(option_strings, dest, nargs, **kwargs)

    @staticmethod
    def create_path(path) -> str:
        return path

    def __call__(self, parser, namespace, values, option_string=None):
        del parser, option_string
        paths = None
        if values is not None:
            if isinstance(values, str):
                paths = self.create_path(values)
            else:
                paths = [self.create_path(v) for v in values]
        setattr(namespace, self.dest, paths)


class StoreInputPath(StorePath):
    @staticmethod
    def create_path(path) -> str:
        if not os.path.exists(path):
            raise ValueError(f"Given path {path} does not exist")
        return path


def get_parser():
    parser = argparse.ArgumentParser(
        prog=_tool_info.__NAME__, formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    parser.add_argument(
        "--version", "-v", action="version", version=_tool_info.__VERSION__
    )

    parser.add_argument(
        "--goal",
        dest="goal_file",
        action=StoreInputPath,
        required=False,
        help="File that defines coverage goal to measure. The default goal is branch coverage.",
    )

    machine_model_args = parser.add_mutually_exclusive_group()
    machine_model_args.add_argument(
        "-32",
        dest="machine_model",
        action="store_const",
        const=eu.MACHINE_MODEL_32,
        default=eu.MACHINE_MODEL_32,
        help="use 32 bit machine model",
    )
    machine_model_args.add_argument(
        "-64",
        dest="machine_model",
        action="store_const",
        const=eu.MACHINE_MODEL_64,
        default=eu.MACHINE_MODEL_32,
        help="use 64 bit machine model",
    )

    parser.add_argument("file", action=StoreInputPath, help="program file")

    return parser


def parse(argv):
    parser = get_parser()
    args = parser.parse_args(argv)

    if not args.goal_file:
        args.goal = eu.COVER_BRANCHES
    else:
        args.goal = parse_coverage_goal_file(args.goal_file)
    args.check_for_error = isinstance(args.goal, eu.CoverFunc)

    return args


def parse_coverage_goal_file(goal_file: str) -> str:
    with open(goal_file, encoding="UTF-8") as inp:
        content = inp.read().strip()
    prop_match = re.match(
        r"COVER\s*\(\s*init\s*\(\s*main\s*\(\s*\)\s*\)\s*,\s*FQL\s*\(COVER\s+EDGES\s*\((.*)\)\s*\)\s*\)",
        content,
    )
    if not prop_match:
        raise IllegalArgumentError(
            f"No valid coverage goal specification in file {goal_file}: {content[:100]}"
        )

    goal = prop_match.group(1).strip()
    if goal not in eu.COVERAGE_GOALS:
        raise IllegalArgumentError(f"No valid coverage goal specification: {goal}")
    return eu.COVERAGE_GOALS[goal]


def run(cmd: list, argv=None, output_dir=None) -> Path:
    args = parse(argv)

    try:
        try:
            executor = execution.InstrumentationRunner(
                machine_model=args.machine_model,
                goal=args.goal,
                output_dir=(output_dir if output_dir is not None else Path(os.getcwd()))
            )
            return executor._prepare_program(cmd, args.file)
        except FileNotFoundError as e:
            logging.error(e)
        except KeyboardInterrupt:
            logging.info("Execution interrupted by user")
    except KeyboardInterrupt:
        logging.info("Execution interrupted by user")    

def main():
    run(None, sys.argv[1:])
