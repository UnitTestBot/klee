// This file is part of TestCov,
// a robust test executor with reliable coverage measurement:
// https://gitlab.com/sosy-lab/software/test-suite-validator/
//
// SPDX-FileCopyrightText: 2021 Dirk Beyer <https://www.sosy-lab.org>
//
// SPDX-License-Identifier: Apache-2.0

#include "LabelerFrontendActionFactory.hpp"
#include "Includes.hpp"
#include "LabelerFrontendAction.hpp"

LabelerFrontendActionFactory::LabelerFrontendActionFactory(
    LabelOptions labelOptions)
    : options(labelOptions) {}

std::unique_ptr<FrontendAction> LabelerFrontendActionFactory::create() {
  return std::make_unique<LabelerFrontendAction>(options);
}

std::unique_ptr<FrontendActionFactory>
newLabelerFrontendActionFactory(LabelOptions labelOptions) {
  return std::unique_ptr<FrontendActionFactory>(
      new LabelerFrontendActionFactory(labelOptions));
}