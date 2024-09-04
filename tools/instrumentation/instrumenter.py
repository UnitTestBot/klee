#!/usr/bin/env python3

# This file is part of TestCov,
# a robust test executor with reliable coverage measurement:
# https://gitlab.com/sosy-lab/software/test-suite-validator/
#
# SPDX-FileCopyrightText: 2019 Dirk Beyer <https://www.sosy-lab.org>
#
# SPDX-License-Identifier: Apache-2.0

import sys
import logging

sys.dont_write_bytecode = True

import src_tooling_runner

sys.exit(src_tooling_runner.main())
