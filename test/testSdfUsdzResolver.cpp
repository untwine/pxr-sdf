// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/pxr.h>

#include <pxr/ar/asset.h>
#include <pxr/ar/resolvedPath.h>
#include <pxr/ar/resolver.h>

#include <pxr/tf/diagnostic.h>
#include <pxr/arch/fileSystem.h>
#include <pxr/tf/stringUtils.h>
#include <pxr/tf/getenv.h>

#include <iostream>
#include <memory>
#include <string>

SDF_NAMESPACE_USING_DIRECTIVE;

// Test that calling ArResolver::OpenAsset on a file within a .usdz
// file produces the expected result.
static void
TestOpenAsset()
{
    std::cout << "TestOpenAsset..." << std::endl;

    ArResolver& resolver = ArGetResolver();

    auto root = TF_NS::TfGetenv("DATA_PATH");
    auto path = TF_NS::TfStringCatPaths(root, "test.usdz[bogus.file]");

    std::shared_ptr<ArAsset> usdzAsset = 
        resolver.OpenAsset(ArResolvedPath(path));
    TF_AXIOM(!usdzAsset);

    auto testAsset = [&root, &resolver](
        const std::string& packageRelativePath,
        const std::string& srcFilePath,
        size_t expectedSize, size_t expectedOffset) {

        auto packagePath = TF_NS::TfStringCatPaths(root, packageRelativePath);
        auto path = TF_NS::TfStringCatPaths(root, srcFilePath);

        std::cout << "  - " << packageRelativePath << std::endl;

        // Verify that we can open the file within the .usdz file and the
        // size is what we expect.
        std::shared_ptr<ArAsset> asset = 
            resolver.OpenAsset(ArResolvedPath(packagePath));
        TF_AXIOM(asset);
        TF_AXIOM(asset->GetSize() == expectedSize);

        // Read in the file data from the asset in various ways and ensure
        // they match the source file.
        ArchConstFileMapping srcFile = ArchMapFileReadOnly(path);
        TF_AXIOM(srcFile);
        TF_AXIOM(ArchGetFileMappingLength(srcFile) == expectedSize);

        std::shared_ptr<const char> buffer = asset->GetBuffer();
        TF_AXIOM(buffer);
        TF_AXIOM(std::equal(buffer.get(), buffer.get() + expectedSize,
                            srcFile.get()));

        std::unique_ptr<char[]> arr(new char[expectedSize]);
        TF_AXIOM(asset->Read(arr.get(), expectedSize, 0) == expectedSize);
        TF_AXIOM(std::equal(arr.get(), arr.get() + expectedSize, 
                            srcFile.get()));

        size_t offset = 100;
        size_t numToRead = expectedSize - offset;
        arr.reset(new char[expectedSize - offset]);
        TF_AXIOM(asset->Read(arr.get(), numToRead, offset) == numToRead);
        TF_AXIOM(std::equal(arr.get(), arr.get() + numToRead,
                            srcFile.get() + offset));
 
        std::pair<FILE*, size_t> fileAndOffset = asset->GetFileUnsafe();
        TF_AXIOM(fileAndOffset.first != nullptr);
        TF_AXIOM(fileAndOffset.second == expectedOffset);

        ArchConstFileMapping file = ArchMapFileReadOnly(fileAndOffset.first);
        TF_AXIOM(file);
        TF_AXIOM(std::equal(file.get() + fileAndOffset.second,
                            file.get() + fileAndOffset.second + expectedSize,
                            srcFile.get()));
    };

    testAsset(
        "test.usdz[file_1.usdc]", "src/file_1.usdc",
        /* expectedSize = */ 680, /* expectedOffset = */ 64);
    testAsset(
        "test.usdz[nested.usdz]", "src/nested.usdz",
        /* expectedSize = */ 2376, /* expectedOffset = */ 832);
    testAsset(
        "test.usdz[nested.usdz[file_1.usdc]]", "src/file_1.usdc",
        /* expectedSize = */ 680, /* expectedOffset = */ 896);
    testAsset(
        "test.usdz[nested.usdz[file_2.usdc]]", "src/file_2.usdc",
        /* expectedSize = */ 621, /* expectedOffset = */ 1664);
    testAsset(
        "test.usdz[nested.usdz[subdir/file_3.usdc]]", 
        "src/subdir/file_3.usdc",
        /* expectedSize = */ 640, /* expectedOffset = */ 2368);
    testAsset(
        "test.usdz[file_2.usdc]", "src/file_2.usdc",
        /* expectedSize = */ 621, /* expectedOffset = */ 3264);
    testAsset(
        "test.usdz[subdir/file_3.usdc]", "src/subdir/file_3.usdc",
        /* expectedSize = */ 640, /* expectedOffset = */ 3968);


}

int main(int argc, char** argv)
{
    TestOpenAsset();

    std::cout << "Passed!" << std::endl;

    return EXIT_SUCCESS;
}
