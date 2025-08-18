//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDF_BOOLEAN_EXPRESSION_EVAL_H
#define PXR_USD_SDF_BOOLEAN_EXPRESSION_EVAL_H

#include <pxr/sdf/booleanExpression.h>

SDF_NAMESPACE_OPEN_SCOPE

// Evaluates the provided expression using the provided callback to resolve
// any variables encountered along the way.
bool
Sdf_EvalBooleanExpression(SdfBooleanExpression const& expression,
     SdfBooleanExpression::VariableCallback const& variableCallback);

SDF_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_BOOLEAN_EXPRESSION_EVAL_H
