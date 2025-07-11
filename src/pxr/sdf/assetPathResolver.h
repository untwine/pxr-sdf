// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_ASSET_PATH_RESOLVER_H
#define PXR_SDF_ASSET_PATH_RESOLVER_H

/// \file sdf/assetPathResolver.h

#include "./declareHandles.h"
#include "./layer.h"

#include <pxr/ar/assetInfo.h>
#include <pxr/ar/resolvedPath.h>
#include <pxr/ar/resolverContext.h>
#include <pxr/vt/value.h>

#include <string>

namespace pxr {

SDF_DECLARE_HANDLES(SdfLayer);

/// \struct Sdf_AssetInfo
///
/// Container for layer asset information.
///
struct Sdf_AssetInfo
{
    std::string identifier;
    ArResolvedPath resolvedPath;
    ArResolverContext resolverContext;
    ArAssetInfo assetInfo;
};

/// Equality operator for Sdf_AssetInfo structures. Two asset info structures
/// if all fields match exactly.
bool operator==(const Sdf_AssetInfo& lhs, const Sdf_AssetInfo& rhs);

/// Returns true if \p identifier can be used to create a new layer, given
/// characteristics of the identifier itself, and the current path resolver
/// configuration.
bool Sdf_CanCreateNewLayerWithIdentifier(
    const std::string& identifier,
    std::string* whyNot);

/// Returns the resolved path for \p layerPath if an asset exists at that
/// path. The returned path is stripped of its file format arguments.
/// If an asset does not exist at that path, returns an empty ArResolvedPath. 
/// Populates \p assetInfo if it's non-nullptr.
ArResolvedPath Sdf_ResolvePath(
    const std::string& layerPath,
    ArAssetInfo* assetInfo = nullptr);

/// Returns the resolved path for \p layerPath. If no asset exists at that
/// path, returns a resolved path indicating where that asset should be
/// created. Otherwise, returns an empty ArResolvedPath. Populates \p assetInfo
/// if it's non-nullptr.
ArResolvedPath Sdf_ComputeFilePath(
    const std::string& layerPath,
    ArAssetInfo* assetInfo = nullptr);

/// Returns true if a layer can be written to \p resolvedPath.
bool Sdf_CanWriteLayerToPath(
    const ArResolvedPath& resolvedPath);

/// Compute the modification timestamp for the given \p layer.
VtValue Sdf_ComputeLayerModificationTimestamp(
    const SdfLayer& layer);

/// Compute the modification timestamps for the external asset dependencies
/// for \p layer. Returns a dictionary of resolved external asset dependency
/// paths to the timestamp for each asset.
VtDictionary Sdf_ComputeExternalAssetModificationTimestamps(
    const SdfLayer& layer);

/// Returns a newly allocated Sdf_AssetInfo struct with fields computed using
/// the specified \p identifier and \p filePath. If \p fileVersion is
/// specified, it is used over the discovered revision of the file. It is the
/// responsibility of the caller to delete the returned value.
Sdf_AssetInfo* Sdf_ComputeAssetInfoFromIdentifier(
    const std::string& identifier,
    const std::string& filePath,
    const ArAssetInfo& assetInfo,
    const std::string& fileVersion = std::string());

/// Returns the identifierTemplate, placeholders replaced with information
/// from the specified \p layer.
std::string Sdf_ComputeAnonLayerIdentifier(
    const std::string& identifierTemplate,
    const SdfLayer* layer);

/// Returns true if \p identifier is an anonymous layer identifier.
bool Sdf_IsAnonLayerIdentifier(
    const std::string& identifier);

/// Returns the portion of the anonymous layer identifier to be used as the
/// display name. This is either the identifier tag, if one is present, or the
/// empty string.
std::string Sdf_GetAnonLayerDisplayName(
    const std::string& identifier);

/// Returns the anonymous layer identifier template, from which
/// Sdf_ComputeAnonLayerIdentifier can compute an anonymous layer identifier.
std::string Sdf_GetAnonLayerIdentifierTemplate(
    const std::string& tag);

/// If \p identifier contains file format arguments
/// (e.g. foo.sdf:SDF_FORMAT_ARGS:a=b), strip them, assign the resulting string
/// to *strippedIdentifier and return true.  Otherwise just return false.
bool Sdf_StripIdentifierArgumentsIfPresent(
    const std::string &identifier,
    std::string *strippedIdentifier);

/// Splits the given \p identifier into two portions: the layer path and the
/// arguments. For example, given the identifier foo.sdf:SDF_FORMAT_ARGS:a=b,
/// this function will set \p layerPath to "foo.sdf" and \p arguments to
/// ":SDF_FORMAT_ARGS:a=b".
bool Sdf_SplitIdentifier(
    const std::string& identifier,
    std::string* layerPath,
    std::string* arguments);

/// Splits the given \p identifier into the layer path and the arguments.
bool Sdf_SplitIdentifier(
    const std::string& identifier,
    std::string* layerPath,
    SdfLayer::FileFormatArguments* arguments);

/// Joins the given \p layerPath and \p arguments into an identifier.
/// These parameters are expected to be in the format returned by 
/// Sdf_SplitIdentifier.
std::string Sdf_CreateIdentifier(
    const std::string& layerPath,
    const std::string& arguments);

/// Joins the given \p layerPath and \p arguments into an identifier.
std::string Sdf_CreateIdentifier(
    const std::string& layerPath,
    const SdfLayer::FileFormatArguments& arguments);

/// Returns true if the given layer \p identifier contains any file
/// format arguments.
bool Sdf_IdentifierContainsArguments(
    const std::string& identifier);

/// Returns the display name for the layer with the given identifier.
/// The identifier may be an anonymous layer identifier, in which case
/// Sdf_GetAnonLayerDisplayName is called.
std::string Sdf_GetLayerDisplayName(
    const std::string& identifier);

/// Returns the extension of the given identifier used to identify the
/// associated file format. 
std::string Sdf_GetExtension(
    const std::string& identifier);

/// Returns true if \p layer is a package layer or packaged within a package 
/// layer.
bool Sdf_IsPackageOrPackagedLayer(
    const SdfLayerHandle& layer);

/// Returns true if \p fileFormat is a package file format or \p identifier
/// is a package-relative path. This is just a convenience function.
bool Sdf_IsPackageOrPackagedLayer(
    const SdfFileFormatConstPtr& fileFormat,
    const std::string& identifier);

}  // namespace pxr

#endif // PXR_SDF_ASSET_PATH_RESOLVER_H
