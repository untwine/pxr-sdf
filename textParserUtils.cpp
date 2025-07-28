//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file Sdf/textParserUtils.cpp

#include "pxr/usd/sdf/textFileFormatParser.h"
#include "pxr/usd/sdf/textParserUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

VtValue
Sdf_ParseValueFromString(
    const std::string& input,
    const SdfValueTypeName& expectedSdfType)
{
    VtValue outputValue;
    if (Sdf_TextFileFormatParser::Sdf_ParseValueFromString(
            input, expectedSdfType, &outputValue)) {
        return outputValue;
    }
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
