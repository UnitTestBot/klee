// This file is part of TestCov,
// a robust test executor with reliable coverage measurement:
// https://gitlab.com/sosy-lab/software/test-suite-validator/
//
// SPDX-FileCopyrightText: 2021 Dirk Beyer <https://www.sosy-lab.org>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef OPTIONS_STRUCT_HPP
#define OPTIONS_STRUCT_HPP

#include <string>

typedef struct LabelOptions {
  bool ternaryTrueLabel, ternaryFalseLabel, functionStartLabel,
      functionEndLabel, caseLabel, defaultLabel, ifLabel, elseLabel, inPlace,
      noInfo;
  std::string functionCall;
} LabelOptions;

#endif