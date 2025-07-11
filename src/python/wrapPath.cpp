// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <set>
#include <utility>
#include <vector>
#include <locale>

#include <pxr/sdf/path.h>
#include <pxr/tf/pyAnnotatedBoolResult.h>
#include <pxr/tf/pyResultConversions.h>
#include <pxr/tf/pyContainerConversions.h>
#include <pxr/tf/hash.h>
#include <pxr/vt/valueFromPython.h>

#include <pxr/boost/python.hpp>

#include <thread>
#include <atomic>

using std::pair;
using std::string;
using std::vector;

using namespace pxr;

using namespace pxr::boost::python;

namespace {

static string
_Repr(const SdfPath &self) {
    if (self.IsEmpty()) {
        return TF_PY_REPR_PREFIX + "Path.emptyPath";
    }
    else {
        return string(TF_PY_REPR_PREFIX +
            "Path(")+TfPyRepr(self.GetAsString())+")";
    }
}

static SdfPathVector
_RemoveDescendentPaths(SdfPathVector paths)
{
    SdfPath::RemoveDescendentPaths(&paths);
    return paths;
}

static SdfPathVector
_RemoveAncestorPaths(SdfPathVector paths)
{
    SdfPath::RemoveAncestorPaths(&paths);
    return paths;
}

static object
_FindPrefixedRange(SdfPathVector const &paths, SdfPath const &prefix)
{
    pair<SdfPathVector::const_iterator, SdfPathVector::const_iterator>
        result = SdfPathFindPrefixedRange(paths.begin(), paths.end(), prefix);
    object start(result.first - paths.begin());
    object stop(result.second - paths.begin());
    handle<> slice(PySlice_New(start.ptr(), stop.ptr(), NULL));
    return object(slice);
}

static object
_FindLongestPrefix(SdfPathVector const &paths, SdfPath const &path)
{
    SdfPathVector::const_iterator result =
        SdfPathFindLongestPrefix(paths.begin(), paths.end(), path);
    if (result == paths.end())
        return object();
    return object(*result);
}

static object
_FindLongestStrictPrefix(SdfPathVector const &paths, SdfPath const &path)
{
    SdfPathVector::const_iterator result =
        SdfPathFindLongestStrictPrefix(paths.begin(), paths.end(), path);
    if (result == paths.end())
        return object();
    return object(*result);
}

struct Sdf_PathIsValidPathStringResult : public TfPyAnnotatedBoolResult<string>
{
    Sdf_PathIsValidPathStringResult(bool val, string const &msg) :
        TfPyAnnotatedBoolResult<string>(val, msg) {}
};

static
Sdf_PathIsValidPathStringResult
_IsValidPathString(string const &pathString)
{
    string errMsg;
    bool valid = SdfPath::IsValidPathString(pathString, &errMsg);
    return Sdf_PathIsValidPathStringResult(valid, errMsg);
}

static
SdfPathVector
_WrapGetAllTargetPathsRecursively(SdfPath const self)
{
    SdfPathVector result;
    self.GetAllTargetPathsRecursively(&result);
    return result;
}

static bool
_NonEmptyPath(SdfPath const &self)
{
    return !self.IsEmpty();
}

constexpr size_t NumStressPaths = 1 << 28;
constexpr size_t NumStressThreads = 16;
constexpr size_t StressIters = 3;
constexpr size_t MaxStressPathSize = 16;

static void _PathStressTask(size_t index, std::vector<SdfPath> &paths)
{
    auto pathsPerThread = NumStressPaths / NumStressThreads;
    auto begin = paths.begin() + pathsPerThread * index;
    auto end = begin + pathsPerThread;
    
    for (size_t stressIter = 0; stressIter != StressIters; ++stressIter) {
        for (auto i = begin; i != end; ++i) {
            SdfPath p = SdfPath::AbsoluteRootPath();
            //size_t offset = (i - begin) * index + stressIter;
            for (size_t j = 0; j != (rand() % MaxStressPathSize); ++j) {
                char name[2];
                name[0] = 'a' + (rand() % 26);
                name[1] = '\0';
                p = p.AppendChild(TfToken(name));
            }
            //if ((i-begin) % 1000 == 0) {
            //printf("%zu: storing path %zu: <%s>\n",
            //       (i-begin), index, p.GetText());
            //}
            *i = p;
        }
        printf("%zu did iter %zu\n", index, stressIter);
    }
}

static void _PathStress()
{
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    std::vector<SdfPath> manyPaths(NumStressPaths);
    std::vector<std::thread> threads(NumStressThreads);

    size_t index = 0;
    for (auto &t: threads) {
        t = std::thread(_PathStressTask, index++, std::ref(manyPaths));
    }
    for (auto &t: threads) {
        t.join();
    }
}


struct Sdf_PyPathAncestorsRangeIterator {
    Sdf_PyPathAncestorsRangeIterator(
        const SdfPathAncestorsRange::iterator& begin,
        const SdfPathAncestorsRange::iterator& end)  
        : _it(begin), _end(end) {}

    SdfPath next() {
        _RaiseIfAtEnd();
        if (_didFirst) {
            ++_it;
            _RaiseIfAtEnd();
        }
        _didFirst = true;
        return *_it;
    }

private:
    void _RaiseIfAtEnd() const {
        if (_it == _end) {
            PyErr_SetString(PyExc_StopIteration, "Iterator at end");
            throw_error_already_set();
        }
    }

    SdfPathAncestorsRange::iterator _it, _end;
    bool _didFirst = false;
};


Sdf_PyPathAncestorsRangeIterator
Sdf_GetIterator(const SdfPathAncestorsRange& range)
{
    return {range.begin(), range.end()};
}


void Sdf_wrapAncestorsRange()
{
    using This = SdfPathAncestorsRange;

    scope s = class_<This>("AncestorsRange", init<const SdfPath&>() )
        .def("GetPath", &This::GetPath,
             return_value_policy<return_by_value>())
        .def("__iter__", &Sdf_GetIterator)
        ;

    using Iter = Sdf_PyPathAncestorsRangeIterator;
    class_<Iter>("_iterator", no_init)
        .def("__next__", &Iter::next)
        ;
}


} // anonymous namespace 

void wrapPath() {    
    typedef SdfPath This;

    def("_PathGetDebuggerPathText", Sdf_PathGetDebuggerPathText);
    def("_PathStress", &_PathStress);
    def("_DumpPathStats", &Sdf_DumpPathStats);

    scope s = class_<This> ( "Path", init< const string & >() )
        .def( init<const SdfPath &>() )
        .def( init<>() )

        // XXX: Document constants
        .def_readonly("absoluteRootPath", &SdfPath::AbsoluteRootPath(),
            "The absolute path representing the top of the \n"
            "namespace hierarchy (</>).")
        .def_readonly("reflexiveRelativePath", &SdfPath::ReflexiveRelativePath(), 
            "The relative path representing 'self' (<.>).")
        .def_readonly("emptyPath", &SdfPath::EmptyPath(), 
            "The empty path.")

        .add_property("pathElementCount", &This::GetPathElementCount, 
            "The number of path elements in this path.")
        .add_property("pathString",
            make_function(&This::GetAsString),
            "The string representation of this path.")
        .add_property("name", make_function(&This::GetName, 
                    return_value_policy<copy_const_reference>()), 
            "The name of the prim, property or relational\n"
            "attribute identified by the path.\n\n"
            "'' for EmptyPath.  '.' for ReflexiveRelativePath.\n"
            "'..' for a path ending in ParentPathElement.\n")
        .add_property("elementString",
            make_function(&This::GetElementString, 
                return_value_policy<return_by_value>()),
            "The string representation of the terminal component of this path."
            "\nThis path can be reconstructed via \n"
            "thisPath.GetParentPath().AppendElementString(thisPath.element).\n"
            "None of absoluteRootPath, reflexiveRelativePath, nor emptyPath\n"
            "possess the above quality; their .elementString is the empty string.")
        .add_property("targetPath", make_function(&This::GetTargetPath, 
                    return_value_policy<copy_const_reference>()), 
            "The relational attribute target path for this path.\n\n"
            "EmptyPath if this is not a relational attribute path.")

        .def("GetAllTargetPathsRecursively", &_WrapGetAllTargetPathsRecursively,
             return_value_policy<TfPySequenceToList>())

        .def("GetVariantSelection", &This::GetVariantSelection,
             return_value_policy<TfPyPairToTuple>())

        .def("IsAbsolutePath", &This::IsAbsolutePath)
        .def("IsAbsoluteRootPath", &This::IsAbsoluteRootPath)
        .def("IsPrimPath", &This::IsPrimPath)
        .def("IsAbsoluteRootOrPrimPath", &This::IsAbsoluteRootOrPrimPath)
        .def("IsRootPrimPath", &This::IsRootPrimPath)
        .def("IsPropertyPath", &This::IsPropertyPath)
        .def("IsPrimPropertyPath", &This::IsPrimPropertyPath)
        .def("IsNamespacedPropertyPath", &This::IsNamespacedPropertyPath)
        .def("IsPrimVariantSelectionPath", &This::IsPrimVariantSelectionPath)
        .def("ContainsPrimVariantSelection", &This::ContainsPrimVariantSelection)
        .def("ContainsPropertyElements", &This::ContainsPropertyElements)
        .def("IsRelationalAttributePath", &This::IsRelationalAttributePath)
        .def("IsTargetPath", &This::IsTargetPath)
        .def("ContainsTargetPath", &This::ContainsTargetPath)
        .def("IsMapperPath", &This::IsMapperPath)
        .def("IsMapperArgPath", &This::IsMapperArgPath)
        .def("IsExpressionPath", &This::IsExpressionPath)

        .add_property("isEmpty", &This::IsEmpty)
        
        .def("HasPrefix", &This::HasPrefix)

        .def("MakeAbsolutePath", &This::MakeAbsolutePath)
        .def("MakeRelativePath", &This::MakeRelativePath)

        .def("GetPrefixes",
             +[](SdfPath const &self, size_t n) { return self.GetPrefixes(n); },
             (arg("numPrefixes") = 0),
             return_value_policy<TfPySequenceToList>())
        
        .def("GetAncestorsRange", &This::GetAncestorsRange)

        .def("GetParentPath", &This::GetParentPath)
        .def("GetPrimPath", &This::GetPrimPath)
        .def("GetPrimOrPrimVariantSelectionPath",
             &This::GetPrimOrPrimVariantSelectionPath)
        .def("GetAbsoluteRootOrPrimPath", &This::GetAbsoluteRootOrPrimPath)
        .def("StripAllVariantSelections", &This::StripAllVariantSelections)

        .def("AppendPath", &This::AppendPath)
        .def("AppendChild", &This::AppendChild)
        .def("AppendProperty", &This::AppendProperty)
        .def("AppendVariantSelection", &This::AppendVariantSelection)
        .def("AppendTarget", &This::AppendTarget)
        .def("AppendRelationalAttribute", &This::AppendRelationalAttribute)
        .def("AppendMapper", &This::AppendMapper)
        .def("AppendMapperArg", &This::AppendMapperArg)
        .def("AppendExpression", &This::AppendExpression)
        .def("AppendElementString", &This::AppendElementString)

        .def("ReplacePrefix", &This::ReplacePrefix,
             ( arg("oldPrefix"),
               arg("newPrefix"),
               arg("fixTargetPaths") = true))
        .def("GetCommonPrefix", &This::GetCommonPrefix)
        .def("RemoveCommonSuffix", &This::RemoveCommonSuffix,
            arg("stopAtRootPrim") = false,
            return_value_policy< TfPyPairToTuple >())
        .def("ReplaceName", &This::ReplaceName)
        .def("ReplaceTargetPath", &This::ReplaceTargetPath)
        .def("MakeAbsolutePath", &This::MakeAbsolutePath)
        .def("MakeRelativePath", &This::MakeRelativePath)

        .def("GetConciseRelativePaths", &This::GetConciseRelativePaths,
            return_value_policy< TfPySequenceToList >())
            .staticmethod("GetConciseRelativePaths")

        .def("RemoveDescendentPaths", _RemoveDescendentPaths,
             return_value_policy< TfPySequenceToList >())
            .staticmethod("RemoveDescendentPaths")
        .def("RemoveAncestorPaths", _RemoveAncestorPaths,
             return_value_policy< TfPySequenceToList >())
            .staticmethod("RemoveAncestorPaths")

        .def("IsValidIdentifier", &This::IsValidIdentifier)
            .staticmethod("IsValidIdentifier")

        .def("IsValidNamespacedIdentifier", &This::IsValidNamespacedIdentifier)
            .staticmethod("IsValidNamespacedIdentifier")

        .def("TokenizeIdentifier",
             &This::TokenizeIdentifier)
            .staticmethod("TokenizeIdentifier")
        .def("JoinIdentifier",
             (std::string (*)(const std::vector<std::string>&))
                 &This::JoinIdentifier)
        .def("JoinIdentifier",
             (std::string (*)(const std::string&, const std::string&))
                 &This::JoinIdentifier)
            .staticmethod("JoinIdentifier")

        .def("StripNamespace",
             (std::string (*)(const std::string&))
                 &This::StripNamespace)
            .staticmethod("StripNamespace")
        .def("StripPrefixNamespace", &This::StripPrefixNamespace, 
                return_value_policy<TfPyPairToTuple>())
            .staticmethod("StripPrefixNamespace")

        .def("IsValidPathString", &_IsValidPathString)
             .staticmethod("IsValidPathString")

        .def("FindPrefixedRange", _FindPrefixedRange)
            .staticmethod("FindPrefixedRange")

        .def("FindLongestPrefix", _FindLongestPrefix)
            .staticmethod("FindLongestPrefix")

        .def("FindLongestStrictPrefix", _FindLongestStrictPrefix)
            .staticmethod("FindLongestStrictPrefix")

        .def("__str__", make_function(&This::GetAsString))

        .def("__bool__", _NonEmptyPath)

        .def(self == self)
        .def(self != self)
        .def(self < self)
        .def(self > self)
        .def(self <= self)
        .def(self >= self)
        .def("__repr__", _Repr)
        .def("__hash__", &This::GetHash)
        ;

    s.attr("absoluteIndicator") = &SdfPathTokens->absoluteIndicator; 
    s.attr("childDelimiter") = &SdfPathTokens->childDelimiter; 
    s.attr("propertyDelimiter") = &SdfPathTokens->propertyDelimiter; 
    s.attr("relationshipTargetStart") = &SdfPathTokens->relationshipTargetStart; 
    s.attr("relationshipTargetEnd") = &SdfPathTokens->relationshipTargetEnd; 
    s.attr("parentPathElement") = &SdfPathTokens->parentPathElement; 
    s.attr("mapperIndicator") = &SdfPathTokens->mapperIndicator; 
    s.attr("expressionIndicator") = &SdfPathTokens->expressionIndicator; 
    s.attr("mapperArgDelimiter") = &SdfPathTokens->mapperArgDelimiter; 
    s.attr("namespaceDelimiter") = &SdfPathTokens->namespaceDelimiter; 

    // Register conversion for python list <-> vector<SdfPath>
    to_python_converter<SdfPathVector, TfPySequenceToPython<SdfPathVector> >();
    TfPyContainerConversions::from_python_sequence<
        SdfPathVector,
        TfPyContainerConversions::
            variable_capacity_all_items_convertible_policy >();

    // Register conversion for python list <-> set<SdfPath>
    to_python_converter<SdfPathSet, TfPySequenceToPython<SdfPathSet> >();
    TfPyContainerConversions::from_python_sequence<
        std::set<SdfPath>,
        TfPyContainerConversions::set_policy >();

    implicitly_convertible<string, This>();

    VtValueFromPython<SdfPath>();

    Sdf_PathIsValidPathStringResult::
        Wrap<Sdf_PathIsValidPathStringResult>("_IsValidPathStringResult",
                                            "errorMessage");

    Sdf_wrapAncestorsRange();
}
