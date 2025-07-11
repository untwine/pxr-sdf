// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_VARIABLE_EXPRESSION_PARSER_H
#define PXR_SDF_VARIABLE_EXPRESSION_PARSER_H

#include <memory>
#include <string>
#include <vector>

namespace pxr {

namespace Sdf_VariableExpressionImpl
{
    class Node;
}

/// \class Sdf_VariableExpressionParserResult
/// Object containing results of parsing an expression.
class Sdf_VariableExpressionParserResult
{
public:
    std::unique_ptr<Sdf_VariableExpressionImpl::Node> expression;
    std::vector<std::string> errors;
};

/// Parse the given expression.
Sdf_VariableExpressionParserResult
Sdf_ParseVariableExpression(const std::string& expr);

/// Returns true if \p s is recognized as a variable expression.
/// This does not check the syntax of the expression.
bool Sdf_IsVariableExpression(const std::string& s);

}  // namespace pxr

#endif

