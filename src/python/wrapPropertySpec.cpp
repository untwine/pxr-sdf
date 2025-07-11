// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

/// \file wrapPropertySpec.cpp

#include <pxr/sdf/propertySpec.h>
#include <pxr/sdf/layer.h>
#include <pxr/sdf/primSpec.h>
#include <pxr/sdf/pySpec.h>

#include <pxr/tf/pyContainerConversions.h>
#include <pxr/tf/pyPtrHelpers.h>
#include <pxr/tf/pyResultConversions.h>

#include <pxr/boost/python.hpp>

using namespace pxr;

using namespace pxr::boost::python;

namespace {

static
void
_WrapSetName(SdfPropertySpec &self,
             const std::string &newName)
{
    // Always validate the new name from python.
    self.SetName(newName, true);
}

static
void
_SetSymmetryArguments(SdfPropertySpec const &self,
                      VtDictionary const &dictionary)
{
    self.GetSymmetryArguments() = dictionary;
}

static
void
_SetCustomData(SdfPropertySpec const &self,
               VtDictionary const &dictionary)
{
    self.GetCustomData() = dictionary;
}

static
void
_SetAssetInfo(SdfPropertySpec const &self,
               VtDictionary const &dictionary)
{
    self.GetAssetInfo() = dictionary;
}

} // anonymous namespace 

void wrapPropertySpec()
{
    typedef SdfPropertySpec This;

    // Register python conversions for vector<SdfPropertySpecHandle>
    to_python_converter<
        SdfPropertySpecHandleVector,
        TfPySequenceToPython<SdfPropertySpecHandleVector> >();

    TfPyContainerConversions::from_python_sequence<
        SdfPropertySpecHandleVector,
        TfPyContainerConversions::variable_capacity_policy >();

    // Register python conversions for vector<SdfPropertySpecConstHandle>
    to_python_converter<
        SdfPropertySpecConstHandleVector,
        TfPySequenceToPython<SdfPropertySpecConstHandleVector> >();

    TfPyContainerConversions::from_python_sequence<
        SdfPropertySpecConstHandleVector,
        TfPyContainerConversions::variable_capacity_policy >();

    class_<This, SdfHandle<This>, 
           bases<SdfSpec>, noncopyable>
        ("PropertySpec", no_init)
        .def(SdfPyAbstractSpec())

        .add_property("name",
            make_function(&This::GetName,
                          return_value_policy<return_by_value>()),
            &_WrapSetName,
            "The name of the property.")

        .add_property("comment",
            &This::GetComment,
            &This::SetComment,
            "A comment describing the property.")

        .add_property("documentation",
            &This::GetDocumentation,
            &This::SetDocumentation,
            "Documentation for the property.")

        .add_property("displayGroup",
            &This::GetDisplayGroup,
            &This::SetDisplayGroup,
            "DisplayGroup for the property.")
        .add_property("displayName",
            &This::GetDisplayName,
            &This::SetDisplayName,
            "DisplayName for the property.")
        .add_property("prefix",
            &This::GetPrefix,
            &This::SetPrefix,
            "Prefix for the property.")

        .add_property("variability",
            &This::GetVariability,
            "Returns the variability of the property.\n\n"
            "An attribute's variability may be Varying\n"
            "Uniform, Config or Computed.\n"
            "For an attribute, the default is Varying, for a relationship "
            "the default is Uniform.\n\n"
            "Varying relationships may be directly authored 'animating' target"
            "paths over time.\n"
            "Varying attributes may be directly authored, animated and \n"
            "affected on by Actions.  They are the most flexible.\n\n"
            "Uniform attributes may be authored only with non-animated values\n"
            "(default values).  They cannot be affected by Actions, but they\n"
            "can be connected to other Uniform attributes.\n\n"
            "Config attributes are the same as Uniform except that a Prim\n"
            "can choose to alter its collection of built-in properties based\n"
            "on the values of its Config attributes.\n\n"
            "Computed attributes may not be authored in scene description.\n"
            "Prims determine the values of their Computed attributes through\n"
            "Prim-specific computation.  They may not be connected.")

        .add_property("hidden",
            &This::GetHidden,
            &This::SetHidden,
            "Whether this property will be hidden in browsers.")

        .add_property("permission",
            &This::GetPermission,
            &This::SetPermission,
            "The property's permission restriction.")

        .add_property("custom",
            &This::IsCustom,
            &This::SetCustom,
            "Whether this property spec declares a custom attribute.")

        .add_property("symmetryFunction",
            &This::GetSymmetryFunction,
            &This::SetSymmetryFunction,
            "The property's symmetry function.")

        .add_property("symmetryArguments",
            &This::GetSymmetryArguments,
            &_SetSymmetryArguments,
            "Dictionary with property symmetry arguments.\n\n"
            "Although this property is marked read-only, you can "
            "modify the contents to add, change, and clear symmetry arguments.")

        .add_property("symmetricPeer",
            &This::GetSymmetricPeer,
            &This::SetSymmetricPeer,
            "The property's symmetric peer.")

        .add_property("customData",
            &This::GetCustomData,
            &_SetCustomData,
            "The property's custom data.\n\n"
            "The default value for custom data is an empty dictionary.\n\n"
            "Custom data is for use by plugins or other non-tools supplied \n"
            "extensions that need to be able to store data attached to arbitrary\n"
            "scene objects.  Note that if the only objects you want to store data\n"
            "on are prims, using custom attributes is probably a better choice.\n"
            "But if you need to possibly store this data on attributes or \n"
            "relationships or as annotations on reference arcs, then custom data\n"
            "is an appropriate choice.")

        .add_property("assetInfo",
            &This::GetAssetInfo,
            &_SetAssetInfo,
            "Returns the asset info dictionary for this property.\n\n"
            "The default value is an empty dictionary.\n\n"
            "The asset info dictionary is used to annotate SdfAssetPath-valued "
            "attributes pointing to the root-prims of assets (generally "
            "organized as models) with various data related to asset "
            "management. For example, asset name, root layer identifier, "
            "asset version etc.\n\n"
            "Note: It is only valid to author assetInfo on attributes that "
            "are of type SdfAssetPath.\n")

        .add_property("owner",
            &This::GetOwner,
            "The owner of this property.  Either a relationship or a prim.")

        .add_property("default",
            &This::GetDefaultValue,
            &This::SetDefaultValue,
            "The default value of this property.")

        .def("HasDefaultValue", &This::HasDefaultValue)
        .def("ClearDefaultValue", &This::ClearDefaultValue)

        .add_property("hasOnlyRequiredFields", &This::HasOnlyRequiredFields,
              "Indicates whether this spec has any significant data other \n"
              "than just what is necessary for instantiation.\n\n"
              "This is a less strict version of isInert, returning True if \n"
              "the spec contains as much as the type and name.")

        // static properties
        .setattr("AssetInfoKey", SdfFieldKeys->AssetInfo)
        .setattr("CommentKey", SdfFieldKeys->Comment)
        .setattr("CustomDataKey", SdfFieldKeys->CustomData)
        .setattr("CustomKey", SdfFieldKeys->Custom)
        .setattr("DisplayGroupKey", SdfFieldKeys->DisplayGroup)
        .setattr("DisplayNameKey", SdfFieldKeys->DisplayName)
        .setattr("DocumentationKey", SdfFieldKeys->Documentation)
        .setattr("HiddenKey", SdfFieldKeys->Hidden)
        .setattr("PermissionKey", SdfFieldKeys->Permission)
        .setattr("PrefixKey", SdfFieldKeys->Prefix)
        .setattr("SymmetricPeerKey", SdfFieldKeys->SymmetricPeer)
        .setattr("SymmetryArgumentsKey", SdfFieldKeys->SymmetryArguments)
        .setattr("SymmetryFunctionKey", SdfFieldKeys->SymmetryFunction)
        ;
}
