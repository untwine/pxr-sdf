//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/atomicOfstreamWrapper.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/trace.h"

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/fileIO.h"
#include "pxr/usd/sdf/fileIO_Common.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/usdFileFormat.h"
#include "pxr/usd/sdf/usdaFileFormat.h"

#include <ostream>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(SdfUsdaFileFormatTokens, SDF_USDA_FILE_FORMAT_TOKENS);

TF_DEFINE_ENV_SETTING(
    SDF_TEXTFILE_SIZE_WARNING_MB, 0,
    "Warn when reading a text file (.usda or .usda derived) larger than this "
    "number of MB (no warnings if set to 0)");

TF_DEFINE_ENV_SETTING(SDF_FILE_FORMAT_LEGACY_IMPORT, "allow",
    "By default, we allow imported strings with the legacy `#sdf 1.4.32` "
    "header format to be read as .usda version 1.0. When this is set to "
    "'warn,' a warning will be emitted when the usda file format attempts "
    "to import a string with header `#sdf 1.4.32`. When this is set to "
    "'error', strings imported with the sdf header will no longer be ingested "
    "and an error will be emitted.");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (allow)
    (warn)
    (error)
    ((legacyCookie, "#sdf 1.4.32"))
    ((modernCookie, "#usda 1.0"))
);

// Our interface to the parser for parsing to SdfData.
extern bool Sdf_ParseLayer(
    const string& context, 
    const std::shared_ptr<PXR_NS::ArAsset>& asset,
    const string& token,
    const string& version,
    bool metadataOnly,
    PXR_NS::SdfDataRefPtr data,
    PXR_NS::SdfLayerHints *hints);

extern bool Sdf_ParseLayerFromString(
    const std::string & layerString,
    const string& token,
    const string& version,
    PXR_NS::SdfDataRefPtr data,
    PXR_NS::SdfLayerHints *hints);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(SdfUsdaFileFormat, SdfFileFormat);
}

SdfUsdaFileFormat::SdfUsdaFileFormat()
    : SdfFileFormat(
        SdfUsdaFileFormatTokens->Id,
        SdfUsdaFileFormatTokens->Version,
        SdfUsdFileFormatTokens->Target,
        SdfUsdaFileFormatTokens->Id.GetString())
{
    // Do Nothing.
}

SdfUsdaFileFormat::SdfUsdaFileFormat(
    const TfToken& formatId,
    const TfToken& versionString,
    const TfToken& target)
    : SdfFileFormat(formatId,
                    (versionString.IsEmpty()
                     ? SdfUsdaFileFormatTokens->Version : versionString),
                    (target.IsEmpty()
                     ? SdfUsdFileFormatTokens->Target : target),
                    formatId )
{
    // Do Nothing.
}

SdfUsdaFileFormat::~SdfUsdaFileFormat()
{
    // Do Nothing.
}

namespace
{

bool
_CanReadImpl(const std::shared_ptr<ArAsset>& asset,
             const std::string& cookie)
{
    TfErrorMark mark;
    
    constexpr size_t COOKIE_BUFFER_SIZE = 512;
    char local[COOKIE_BUFFER_SIZE];
    std::unique_ptr<char []> remote;
    char *buf = local;
    size_t cookieLength = cookie.length();
    if (cookieLength > COOKIE_BUFFER_SIZE - 1) {
        remote.reset(new char[cookieLength + 1]);
        buf = remote.get();
    }
    if (asset->Read(buf, cookieLength, /* offset = */ 0) != cookieLength) {
        return false;
    }

    buf[cookieLength] = '\0';

    // Don't allow errors to escape this function, since this function is
    // just trying to answer whether the asset can be read.
    return !mark.Clear() && TfStringStartsWith(buf, cookie);
}

} // end anonymous namespace

bool
SdfUsdaFileFormat::CanRead(const string& filePath) const
{
    TRACE_FUNCTION();

    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(
        ArResolvedPath(filePath));
    return asset && _CanReadImpl(asset, GetFileCookie());
}

bool
SdfUsdaFileFormat::_CanReadFromAsset(
    const std::string& resolvedPath,
    const std::shared_ptr<ArAsset>& asset) const
{
    return _CanReadImpl(asset, GetFileCookie());
}

bool
SdfUsdaFileFormat::Read(
    SdfLayer* layer,
    const string& resolvedPath,
    bool metadataOnly) const
{
    TRACE_FUNCTION();

    std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(
        ArResolvedPath(resolvedPath));
    if (!asset) {
        return false;
    }

    return _ReadFromAsset(layer, resolvedPath, asset, metadataOnly);
}

bool
SdfUsdaFileFormat::_ReadFromAsset(
    SdfLayer* layer,
    const string& resolvedPath,
    const std::shared_ptr<ArAsset>& asset,
    bool metadataOnly) const
{
    // Quick check to see if the file has the magic cookie before spinning up
    // the parser.
    if (!_CanReadImpl(asset, GetFileCookie())) {
        TF_RUNTIME_ERROR("<%s> is not a valid %s layer",
                         resolvedPath.c_str(),
                         GetFormatId().GetText());
        return false;
    }

    const int fileSizeWarning = TfGetEnvSetting(SDF_TEXTFILE_SIZE_WARNING_MB);
    const size_t toMB = 1048576;

    if (fileSizeWarning > 0 && asset->GetSize() > (fileSizeWarning * toMB)) {
        TF_WARN("Performance warning: reading %lu MB text-based layer <%s>.",
                asset->GetSize() / toMB,
                resolvedPath.c_str());
    }

    SdfLayerHints hints;
    SdfAbstractDataRefPtr data = InitData(layer->GetFileFormatArguments());
    if (!Sdf_ParseLayer(
            resolvedPath, asset, GetFormatId(), GetVersionString(), 
            metadataOnly, TfDynamic_cast<SdfDataRefPtr>(data), &hints)) {
        return false;
    }

    _SetLayerData(layer, data, hints);
    return true;
}

// Predicate for determining fields that should be included in a
// layer's metadata section.
struct Sdf_IsLayerMetadataField : public Sdf_IsMetadataField
{
    Sdf_IsLayerMetadataField() : Sdf_IsMetadataField(SdfSpecTypePseudoRoot) { }
    
    bool operator()(const TfToken& field) const
    { 
        return (Sdf_IsMetadataField::operator()(field) ||
            field == SdfFieldKeys->SubLayers);
    }
};


#define _Write             Sdf_FileIOUtility::Write
#define _WriteQuotedString Sdf_FileIOUtility::WriteQuotedString
#define _WriteAssetPath    Sdf_FileIOUtility::WriteAssetPath
#define _WriteSdPath       Sdf_FileIOUtility::WriteSdPath
#define _WriteNameVector   Sdf_FileIOUtility::WriteNameVector
#define _WriteLayerOffset  Sdf_FileIOUtility::WriteLayerOffset

static bool
_WriteLayer(
    const SdfLayer* l,
    Sdf_TextOutput& out,
    const string& cookie,
    const string& versionString,
    const string& commentOverride)
{
    TRACE_FUNCTION();

    _Write(out, 0, "%s %s\n", cookie.c_str(), versionString.c_str());

    // Grab the pseudo-root, which is where all layer-specific
    // fields live.
    SdfPrimSpecHandle pseudoRoot = l->GetPseudoRoot();

    // Accumulate header metadata in a stringstream buffer,
    // as an easy way to check later if we have any layer
    // metadata to write at all.
    Sdf_StringOutput header;

    // Partition this layer's fields so that all fields to write out are
    // in the range [fields.begin(), metadataFieldsEnd).
    TfTokenVector fields = pseudoRoot->ListFields();
    TfTokenVector::iterator metadataFieldsEnd = std::partition(
        fields.begin(), fields.end(), Sdf_IsLayerMetadataField());

    // Write comment at the top of the metadata section for readability.
    const std::string comment = commentOverride.empty() ?
        l->GetComment() : commentOverride;

    if (!comment.empty()) {
        _WriteQuotedString(header, 1, comment);
        _Write(header, 0, "\n");
    }

    // Write out remaining fields in the metadata section in alphabetical
    // order.
    std::sort(fields.begin(), metadataFieldsEnd);
    for (TfTokenVector::const_iterator fieldIt = fields.begin();
         fieldIt != metadataFieldsEnd; ++fieldIt) {

        const TfToken& field = *fieldIt;

        if (field == SdfFieldKeys->Documentation) {
            if (!l->GetDocumentation().empty()) {
                _Write(header, 1, "doc = ");
                _WriteQuotedString(header, 0, l->GetDocumentation());
                _Write(header, 0, "\n");
            }
        }
        else if (field == SdfFieldKeys->SubLayers) {
            _Write(header, 1, "subLayers = [\n");

            size_t c = l->GetSubLayerPaths().size();
            for(size_t i=0; i<c; i++)
            {
                _WriteAssetPath(header, 2, l->GetSubLayerPaths()[i]);
                _WriteLayerOffset(header, 0, false /* multiLine */, 
                                  l->GetSubLayerOffset(static_cast<int>(i)));
                _Write(header, 0, (i < c-1) ? ",\n" : "\n");
            }
            _Write(header, 1, "]\n");
        }
        else if (field == SdfFieldKeys->HasOwnedSubLayers) {
            if (l->GetHasOwnedSubLayers()) {
                _Write(header, 1, "hasOwnedSubLayers = true\n");
            }
        }
        else {
            Sdf_WriteSimpleField(header, 1, pseudoRoot.GetSpec(), field);
        }

    } // end for each field

    // Add any layer relocates to the header.
    if (l->HasRelocates()) {
        Sdf_FileIOUtility::WriteRelocates(
            header, 1, true, l->GetRelocates());
    }

    // Write header if not empty.
    string headerStr = header.GetString();
    if (!headerStr.empty()) {
        _Write(out, 0, "(\n");
        _Write(out, 0, "%s", headerStr.c_str());
        _Write(out, 0, ")\n");
    }

    // Root prim reorder statement
    const std::vector<TfToken> &rootPrimNames = l->GetRootPrimOrder();

    if (rootPrimNames.size() > 1)
    {
        _Write(out, 0,"\n");
        _Write(out, 0, "reorder rootPrims = ");
        _WriteNameVector(out, 0, rootPrimNames);
        _Write(out, 0, "\n");
    }
        
    // Root prims
    for (const SdfPrimSpecHandle& rootPrim : l->GetRootPrims()) {
        _Write(out, 0,"\n");
        Sdf_WritePrim(rootPrim.GetSpec(), out, 0);
    }

    _Write(out, 0,"\n");

    return true;
}

bool
SdfUsdaFileFormat::WriteToFile(
    const SdfLayer& layer,
    const std::string& filePath,
    const std::string& comment,
    const FileFormatArguments& args) const
{
    std::shared_ptr<ArWritableAsset> asset = 
        ArGetResolver().OpenAssetForWrite(
            ArResolvedPath(filePath), ArResolver::WriteMode::Replace);
    if (!asset) {
        TF_RUNTIME_ERROR(
            "Unable to open %s for write", filePath.c_str());
        return false;
    }

    Sdf_TextOutput out(std::move(asset));

    const bool ok = _WriteLayer(
        &layer, out, GetFileCookie(), GetVersionString(), comment);

    if (ok && !out.Close()) {
        TF_RUNTIME_ERROR("Could not close %s", filePath.c_str());
        return false;
    }

    return ok;
}

bool 
SdfUsdaFileFormat::ReadFromString(
    SdfLayer* layer,
    const std::string& str) const
{
    SdfLayerHints hints;
    SdfAbstractDataRefPtr data = InitData(layer->GetFileFormatArguments());

    // XXX: Its possible that str has leading whitespace, owing to in-code layer
    // constructions. This is currently allowed in flex+bison parser, but will
    // be tightened with the pegtl parser. Note that this whitespace trimming 
    // code will eventually be removed, it's being put in place so as to provide 
    // backward compatibility for in-code layer constructs to work with pegtl 
    // parser also. This code should be removed when (USD-9838) gets worked on.
    std::string trimmedStr = TfStringTrimLeft(str);
    
    // The legacy sdf format is deprecated in favor of the usda format.
    // Since `sdf 1.4.32` is equivalent in content to `usda 1.0`, allow
    // imported strings headed with the former to be read by the latter.
    if (TfStringStartsWith(trimmedStr, _tokens->legacyCookie)) {
        static const std::string readSdf =
            TfGetEnvSetting(SDF_FILE_FORMAT_LEGACY_IMPORT);

        if (_tokens->allow == readSdf || _tokens->warn == readSdf) {
            trimmedStr = _tokens->modernCookie.GetString() + 
                trimmedStr.substr((_tokens->legacyCookie).size());

            if (_tokens->warn == readSdf) {
                TF_WARN("'%s' is a deprecated format for reading. "
                        "Use '%s' instead.",
                        _tokens->legacyCookie.GetText(),
                        _tokens->modernCookie.GetText());
            }
        } else {
            TF_RUNTIME_ERROR("'%s' is not a supported format for reading. "
                             "Use '%s' instead.",
                             _tokens->legacyCookie.GetText(),
                             _tokens->modernCookie.GetText());
            return false;
        }
    }

    if (!Sdf_ParseLayerFromString(
            trimmedStr, GetFormatId(), GetVersionString(),
            TfDynamic_cast<SdfDataRefPtr>(data), &hints)) {
        return false;
    }

    _SetLayerData(layer, data, hints);
    return true;
}

bool 
SdfUsdaFileFormat::WriteToString(
    const SdfLayer& layer,
    std::string* str,
    const std::string& comment) const
{
    Sdf_StringOutput out;

    if (!_WriteLayer(
            &layer, out, GetFileCookie(), GetVersionString(), comment)) {
        return false;
    }

    *str = out.GetString();
    return true;
}

bool 
SdfUsdaFileFormat::WriteToStream(
    const SdfSpecHandle &spec,
    std::ostream& out,
    size_t indent) const
{
    return Sdf_WriteToStream(spec.GetSpec(), out, indent);
}

bool
SdfUsdaFileFormat::_ShouldSkipAnonymousReload() const
{
    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE
