// This file is part of TestCov,
// a robust test executor with reliable coverage measurement:
// https://gitlab.com/sosy-lab/software/test-suite-validator/
//
// SPDX-FileCopyrightText: 2021 Dirk Beyer <https://www.sosy-lab.org>
//
// SPDX-License-Identifier: Apache-2.0

#include "Includes.hpp"

#ifndef LABELER_FRONTEND_ACTION_FACTORY_HPP
#define LABELER_FRONTEND_ACTION_FACTORY_HPP

// A Factory Method, generating FrontendActions for each processed file
std::unique_ptr<FrontendActionFactory>
newLabelerFrontendActionFactory(LabelOptions labelOptions);

class LabelerFrontendActionFactory : public FrontendActionFactory {
public:
  LabelerFrontendActionFactory(LabelOptions labelOptions);
  std::unique_ptr<FrontendAction> create() override;

private:
  LabelOptions options;
};

std::unique_ptr<FrontendActionFactory>
newLabelerFrontendActionFactory(LabelOptions labelOptions);

#endif