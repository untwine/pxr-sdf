// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/path.h>

#include <pxr/tf/stopwatch.h>
#include <pxr/tf/diagnostic.h>
#include <pxr/tf/pxrCLI11/CLI11.h>
#include <pxr/tf/stringUtils.h>

#include <atomic>
#include <ctime>
#include <cstdlib>
#include <mutex>
#include <thread>


using std::string;
using std::vector;

using namespace pxr;
using namespace pxr::CLI;

static unsigned int randomSeed;
static size_t numThreads;
static size_t msecsToRun = 2000;

TF_MAKE_STATIC_DATA(vector<TfToken>, nameTokens)
{
    nameTokens->push_back(TfToken("A"));
    nameTokens->push_back(TfToken("B"));
    nameTokens->push_back(TfToken("C"));

    // Create a large number of candidates to try to exercise paths
    // over the SD_PATH_BINARY_SEARCH_THRESHOLD.
    for (int i=0; i < 64; ++i) {
        std::string s = TfStringPrintf("x_%i", i);
        nameTokens->push_back(TfToken(s));
    }
}

static TfToken
_GetRandomNameToken()
{
    return (*nameTokens)[rand() % nameTokens->size()];
}

static SdfPath
_MakeRandomPrimPath()
{
    static const size_t maxDepth = 2;
    SdfPath ret = SdfPath::AbsoluteRootPath();
    for (size_t i = 0, depth = rand() % maxDepth; i <= depth; ++i)
        ret = ret.AppendChild(_GetRandomNameToken());
    return ret;
}

static SdfPath
_MakeRandomPrimOrPropertyPath()
{
    SdfPath ret = _MakeRandomPrimPath();
    return rand() & 1 ? ret : ret.AppendProperty(_GetRandomNameToken());
}

static SdfPath
_MakeRandomPath(SdfPath const &path = SdfPath::AbsoluteRootPath())
{
    SdfPath ret = path;

    // Absolute root -> prim path.
    if (path == SdfPath::AbsoluteRootPath())
        ret = _MakeRandomPrimPath();

    // Extend a PrimPath.
    if (ret.IsPrimPath() && (rand() & 1)) {
        ret = ret.AppendVariantSelection(_GetRandomNameToken().GetString(),
                                         _GetRandomNameToken().GetString());
    }

    // Extend a PrimPath or a PrimVariantSelectionPath.
    if ((ret.IsPrimPath() || ret.IsPrimVariantSelectionPath())) {
        return (rand() & 1) ? ret :
            _MakeRandomPath(ret.AppendProperty(_GetRandomNameToken()));
    }

    // Extend a PrimPropertyPath
    if (ret.IsPrimPropertyPath()) {
        // options: target path, mapper path, expression path, or leave alone.
        switch (rand() & 3) {
        case 0:
            return _MakeRandomPath(
                ret.AppendTarget(_MakeRandomPrimOrPropertyPath()));
        case 1:
            return _MakeRandomPath(
                ret.AppendMapper(_MakeRandomPrimOrPropertyPath()));
        case 2:
            return _MakeRandomPath(ret.AppendExpression());
        case 3:
            return ret;
        };
    }

    // Extend a TargetPath
    if (ret.IsTargetPath()) {
        return (rand() & 1) ? ret :
            _MakeRandomPath(
            ret.AppendRelationalAttribute(_GetRandomNameToken()));
    }

    // Extend a MapperPath
    if (ret.IsMapperPath()) {
        return (rand() & 1) ? ret :
            _MakeRandomPath(ret.AppendMapperArg(_GetRandomNameToken()));
    }

    // Extend a RelationalAttributePath
    if (ret.IsRelationalAttributePath()) {
        return (rand() & 1) ? ret :
            _MakeRandomPath(ret.AppendTarget(_MakeRandomPrimOrPropertyPath()));
    }

    return ret;
}

TF_MAKE_STATIC_DATA(SdfPathVector, pathCache)
{
    static const size_t pathCacheSize = 32;
    *pathCache = SdfPathVector(pathCacheSize);
    for (size_t i = 0; i != pathCache->size(); ++i) 
        (*pathCache)[i] = _MakeRandomPath();
}

static TfStaticData<std::mutex> pathCacheMutex;

static void _PutPath(SdfPath const &path)
{
    std::lock_guard<std::mutex> lock(*pathCacheMutex);
    size_t index = rand() % pathCache->size();
    (*pathCache)[index] = path;
}

static SdfPath _GetPath()
{
    std::lock_guard<std::mutex> lock(*pathCacheMutex);
    size_t index = rand() % pathCache->size();
    return (*pathCache)[index];
}

static std::atomic_int nIters(0);

static TfStopwatch _DoPathOperations()
{
    TfStopwatch sw;

    while (static_cast<size_t>(sw.GetMilliseconds()) < msecsToRun) {
        sw.Start();
        SdfPath p = (rand() & 1) ? _GetPath() : SdfPath::AbsoluteRootPath();
        // If the path is not very extensible, trim it back to the prim path.
        if (p.IsExpressionPath() || p.IsMapperArgPath() || p.IsMapperPath())
            p = p.GetPrimPath();

        SdfPath randomP = _MakeRandomPath(p);
        _PutPath(randomP);

        sw.Stop();
        ++nIters;
    }

    return sw;
}

int main(int argc, char const **argv)
{
    // Set up arguments and their defaults
    CLI::App app("Tests SdfPath threading", "testSdfPathThreading");
    app.add_option("--seed", randomSeed, "Random seed")
        ->default_val(time(NULL));
    app.add_option("--numThreads", numThreads, "Number of threads to use")
        ->default_val(std::thread::hardware_concurrency());
    app.add_option("--msec", msecsToRun, "Milliseconds to run")
        ->default_val(2000);

    CLI11_PARSE(app, argc, argv);

    // Initialize. 
    srand(randomSeed);
    printf("Using random seed: %d\n", randomSeed);
    printf("Using %zu threads\n", numThreads);

    // Run.
    TfStopwatch sw;
    sw.Start();

    std::vector<std::thread> workers;
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(_DoPathOperations);
    }

    std::for_each(workers.begin(), workers.end(), 
                  [](std::thread& t) { t.join(); });
    
    sw.Stop();

    // Report.
    printf("Ran %d SdfPath operations on %zu thread%s in %.3f sec "
           "(%.3f ops/sec)\n", (int)nIters, numThreads,
           numThreads > 1 ? "s" : "", sw.GetSeconds(),
           double((int)nIters) / sw.GetSeconds());
    return 0;
}

