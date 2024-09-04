# This file is part of TestCov,
# a robust test executor with reliable coverage measurement:
# https://gitlab.com/sosy-lab/software/test-suite-validator/
#
# Copyright (C) 2018 - 2020  Dirk Beyer
# SPDX-FileCopyrightText: 2019 Dirk Beyer <https://www.sosy-lab.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Module for creation and execution of test harnesses from test-format XML files."""

import os
import tempfile

from src_tooling_runner import execution_utils as eu
from src_tooling_runner import transformer as tr

from pathlib import Path

class InstrumentationRunner:
    def __init__(
        self,
        machine_model,
        goal,
        output_dir : Path
    ):
        self.machine_model = machine_model
        self._goal = goal
        self.output_dir = output_dir.resolve()

    def _get_instrumented_file_name(self, original_file) -> str:
        filename = os.path.basename(original_file)
        return self.output_dir / ("instrumented_" + filename)

    def _prepare_program(self, cmd, program_file) -> Path:
        if eu.uses_line_coverage(self._goal):
            return program_file # no modifications possible without changing line count

        instrumented_program_file: str = self._get_instrumented_file_name(program_file)
        os.makedirs(instrumented_program_file.parent, exist_ok=True)

        tr.instrument_program(
            cmd, program_file, self.machine_model, instrumented_program_file, self._goal
        )

        return Path(instrumented_program_file)
