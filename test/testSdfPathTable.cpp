// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/pathTable.h>
#include <pxr/sdf/path.h>

#include <pxr/tf/stringUtils.h>
#include <pxr/tf/stopwatch.h>
#include <pxr/tf/diagnostic.h>
#include <pxr/tf/hashmap.h>
#include <pxr/arch/fileSystem.h>

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <utility>

using std::make_pair;
using std::pair;
using std::string;
using std::vector;

using namespace pxr;


template <class FwdIter>
static size_t Count(FwdIter b, FwdIter e, SdfPath const &p)
{
    size_t n = 0;
    while (b != e) {
        if (b->first == p)
            ++n;
        ++b;
    }
    return n;
}

static void DoUnitTest()
{
    typedef SdfPathTable<string> Table;

    Table table;

    // Begins empty.
    TF_AXIOM(table.empty());
    TF_AXIOM(table.size() == 0);
    TF_AXIOM(table.begin() == table.end());

    {
        // Copy construct, and swap.
        Table table2(table);
        TF_AXIOM(table2.empty());
        TF_AXIOM(table2.size() == 0);
        TF_AXIOM(table2.begin() == table2.end());

        // Inserting a path implicitly inserts all ancestors.
        TF_AXIOM(table2.insert(Table::value_type(SdfPath("/a/b/c/d"),
                                                 string())).second);
        TF_AXIOM(table2.insert(Table::value_type(SdfPath("/a/b/x/y"),
                                                 string())).second);
        TF_AXIOM(table2.size() == 7);

        // Make a copy via assignment.
        Table table3;
        table3 = table2;
        TF_AXIOM(table3.size() == 7);

        // Erase a subtree.
        table3.erase(SdfPath("/a/b"));
        TF_AXIOM(table3.size() == 2);
        TF_AXIOM(table2.size() == 7);

        // Swap.
        table3.swap(table2);
        TF_AXIOM(table2.size() == 2);
        TF_AXIOM(table3.size() == 7);

        // Move.
        Table table4(std::move(table3));
        table3 = Table();
        TF_AXIOM(table2.size() == 2);
        TF_AXIOM(table3.empty());
        TF_AXIOM(table4.size() == 7);
        table3 = std::move(table4);
        TF_AXIOM(table3.size() == 7);

        // Clear.
        table2.clear();
        table3.clear();

        TF_AXIOM(table2.empty() && table3.empty());
    }

    // Insertion implicitly inserts ancestors.
    pair<Table::iterator, bool> result =
        table.insert(make_pair(SdfPath("/foo/bar"), "/foo/bar"));
    TF_AXIOM(result.second);
    TF_AXIOM(result.first->first == SdfPath("/foo/bar"));
    TF_AXIOM(result.first->second == "/foo/bar");
    TF_AXIOM(table.size() == 3);
    TF_AXIOM(!table.empty());

    result = table.insert(make_pair(SdfPath("/baz"), "/baz"));
    TF_AXIOM(result.second);
    TF_AXIOM(result.first->first == SdfPath("/baz"));
    TF_AXIOM(result.first->second == "/baz");
    TF_AXIOM(table.size() == 4);
    TF_AXIOM(!table.empty());

    // Test NodeHandle interface.
    Table::NodeHandle node;
    TF_AXIOM(!node);
    node = Table::NodeHandle::New(SdfPath("/foo/node"), "/foo/node");
    TF_AXIOM(node);
    SdfPath const *stablePathAddr = &node.GetKey();
    std::string const *stableMappedAddr = &node.GetMapped();
    TF_AXIOM(stablePathAddr);
    TF_AXIOM(stableMappedAddr);
    result = table.insert(std::move(node));
    TF_AXIOM(result.second);
    TF_AXIOM(result.first->first == SdfPath("/foo/node"));
    TF_AXIOM(result.first->second == "/foo/node");
    TF_AXIOM(&result.first->first == stablePathAddr);
    TF_AXIOM(&result.first->second == stableMappedAddr);
    
    TF_AXIOM(table.size() == 5);

    TF_AXIOM(!node);
    node = Table::NodeHandle::New(SdfPath("/foo/node"), "dupe");
    TF_AXIOM(node);
    result = table.insert(std::move(node));
    TF_AXIOM(!result.second);
    TF_AXIOM(result.first->first == SdfPath("/foo/node"));
    TF_AXIOM(result.first->second == "/foo/node");
    TF_AXIOM(node.GetKey() == SdfPath("/foo/node"));
    TF_AXIOM(node.GetMapped() == "dupe");
    TF_AXIOM(table.size() == 5);

    table.erase(result.first);
    TF_AXIOM(table.size() == 4);

    // Test implicit ancestor insertion.
    result = table.
        insert(make_pair(SdfPath("/foo/anim/chars/MeridaGroup/Merida"),
                         "Merida"));
    TF_AXIOM(result.second);
    TF_AXIOM(result.first->first ==
             SdfPath("/foo/anim/chars/MeridaGroup/Merida"));
    TF_AXIOM(result.first->second == "Merida");
    TF_AXIOM(table.size() == 8);
    TF_AXIOM(!table.empty());


    result = table.insert(make_pair(SdfPath("/foo/sets/Castle"), "Castle"));
    TF_AXIOM(result.second);
    TF_AXIOM(result.first->first == SdfPath("/foo/sets/Castle"));
    TF_AXIOM(result.first->second == "Castle");
    TF_AXIOM(table.size() == 10);

    result = table.
        insert(make_pair(SdfPath("/foo/anim/chars/AngusGroup/Angus"), "Angus"));
    TF_AXIOM(result.second);
    TF_AXIOM(table.size() == 12);

    // Insert using [] operator.
    table[SdfPath("/foo/anim/chars/MeridaGroup/MeridaBow")] = "MeridaBow";
    TF_AXIOM(table.count(SdfPath("/foo/anim/chars/MeridaGroup/MeridaBow")));
    TF_AXIOM(table.size() == 13);

    table[SdfPath("/foo/anim/chars/MeridaGroup/MeridaSword")] = "MeridaSword";
    TF_AXIOM(table.count(SdfPath("/foo/anim/chars/MeridaGroup/MeridaSword")));
    TF_AXIOM(table.size() == 14);

    // find
    TF_AXIOM(table.find(SdfPath("/foo/sets/Castle")) != table.end());
    TF_AXIOM(table.find(SdfPath("/foo/sets/Castle"))->second == "Castle");

    TF_AXIOM(table.find(SdfPath("/foo")) != table.end());
    TF_AXIOM(table.find(SdfPath("/foo"))->second.empty());

    // Find subtree range
    {
        pair<Table::iterator, Table::iterator> range;

        // range should be empty.
        range = table.FindSubtreeRange(SdfPath("/no/such/path/in/table"));
        TF_AXIOM(range.first == range.second);

        // range should contain all elements.
        range = table.FindSubtreeRange(SdfPath("/"));
        TF_AXIOM(static_cast<size_t>(std::distance(range.first, range.second)) 
                    == table.size());

        // range should contain subset of elements: /foo/anim/chars,
        // /foo/anim/chars/MeridaGroup, /foo/anim/chars/MeridaGroup/Merida,
        // /foo/anim/chars/AngusGroup /foo/anim/chars/AngusGroup/Angus
        // /foo/anim/chars/MeridaGroup/MeridaBow
        // /foo/anim/chars/MeridaGroup/MeridaSword.
        range = table.FindSubtreeRange(SdfPath("/foo/anim/chars"));
        using std::count;
        TF_AXIOM(std::distance(range.first, range.second) == 7);
        TF_AXIOM(Count(range.first, range.second,
                       SdfPath("/foo/anim/chars")) == 1);
        TF_AXIOM(Count(range.first, range.second,
                       SdfPath("/foo/anim/chars/MeridaGroup")) == 1);
        TF_AXIOM(Count(range.first, range.second,
                       SdfPath("/foo/anim/chars/MeridaGroup/Merida")) == 1);
        TF_AXIOM(Count(range.first, range.second,
                       SdfPath("/foo/anim/chars/AngusGroup")) == 1);
        TF_AXIOM(Count(range.first, range.second,
                       SdfPath("/foo/anim/chars/AngusGroup/Angus")) == 1);
        TF_AXIOM(Count(range.first, range.second,
                       SdfPath("/foo/anim/chars/MeridaGroup/MeridaBow")) == 1);
        TF_AXIOM(Count(range.first, range.second,
                       SdfPath("/foo/anim/chars/MeridaGroup/MeridaSword")) == 1);
    }

    // build a table<SdfPath, std::string> from the table.
    std::map<SdfPath, std::string> pathMap(table.begin(), table.end());
    TF_AXIOM(pathMap.size() == table.size());
    TF_FOR_ALL(i, pathMap)
        TF_AXIOM(table.count(i->first));

    {
        // Const iterator.
        const Table ct(table);
        std::map<SdfPath, std::string> pathMap(ct.begin(), ct.end());
        TF_AXIOM(pathMap.size() == ct.size());
        TF_FOR_ALL(i, pathMap)
            TF_AXIOM(ct.count(i->first));
    }

    // erase
    TF_AXIOM(table.erase(SdfPath("/foo/anim")) == true);
    TF_AXIOM(table.erase(SdfPath("/foo/sets")) == true);
    TF_AXIOM(table.erase(SdfPath("/NotPresentInTable")) == false);

    table.erase(table.find(SdfPath("/baz")));
    table.erase(table.find(SdfPath("/")));
    TF_AXIOM(table.empty());
    TF_AXIOM(table.size() == 0);

    // build a table and then clear it in parallel
    for (char ch='a'; ch<='z'; ++ch) {
        std::string value(1, ch);
        for (char n='0'; n<='9'; ++n) {
            char p[] = { '/', ch, n, '/', ch, n, '/', ch, n, '/', ch, n, '\0' };
            table.insert({SdfPath(p), value});
        }
    }
    TF_AXIOM(table.size() == 1+26*10*4);
    table.ClearInParallel();
    TF_AXIOM(table.empty());
    TF_AXIOM(table.size() == 0);

    // build a table and then parallel-iterate over it
    for (char ch='a'; ch<='z'; ++ch) {
        std::string value(1, ch);
        for (char n='0'; n<='9'; ++n) {
            char p[] = { '/', ch, n, '/', ch, n, '/', ch, n, '/', ch, n, '\0' };
            table.insert({SdfPath(p), value});
        }
    }
    // const parallel for each...
    std::atomic_int z_count(0);
    const Table& ctable = table;
    ctable.ParallelForEach([&z_count](const SdfPath &p, const string &v) {
        if (v == "z") { ++z_count; }
    });
    TF_AXIOM(z_count.load() == 10);
    // non-const parallel for each...
    table.ParallelForEach([](const SdfPath &p, string &v) {
        if (p.GetName() == "a2") {
            v = "found";
        }
    });
    TF_AXIOM(table.find(SdfPath("/a2/a2/a2/a2"))->second == "found");
    TF_AXIOM(table.find(SdfPath("/a3/a3/a3/a3"))->second == "a");

    // Test iterator.HasChild()
    TF_AXIOM(table.find(SdfPath("/a2/a2/a2")).HasChild() == true);
    TF_AXIOM(table.find(SdfPath("/a2/a2/a2/a2")).HasChild() == false);
}


static void ReadPaths(string const &fileName, vector<SdfPath> *out)
{
    printf("Reading paths...");
    fflush(stdout);

    FILE* fp = fopen(fileName.c_str(), "rb");
    if (!fp)
        return;

    const size_t length = ArchGetFileLength(fp);
    auto src = ArchMapFileReadOnly(fp);
    TfStopwatch sw; sw.Start();
    string all(src.get(), length);
    sw.Stop(); printf("reading all took %f sec\n", sw.GetSeconds());
    src.reset();
    fclose(fp);

    sw.Reset(); sw.Start();
    vector<string> lines = TfStringTokenize(all);
    sw.Stop(); printf("tokenize took %f sec\n", sw.GetSeconds());

    sw.Reset(); sw.Start();
    for (const auto& line : lines) {
        out->push_back(SdfPath(line));
    }
    sw.Stop(); printf("building paths took %f sec\n", sw.GetSeconds());

    //printf("  done!  Read %zu paths.\n", out->size() - initialSize);
}

template <class Driver>
static void Bench(size_t numIters,
                  vector<SdfPath> const &paths,
                  Driver *driver)
{
    TfStopwatch sw;

    sw.Start();
    // Insert all paths.
    size_t count = 0;
    for (const auto& path : paths) {
        driver->Insert(path);
        if (++count % 100000 == 0) {
            printf("...inserted %zu paths\n", count);
        }
    }
    sw.Stop();
    printf("Inserted %zu paths in %f seconds\n",
           paths.size(), sw.GetSeconds());

    sw.Reset();
    sw.Start();

    // Erase random subtrees.
    size_t n = numIters;
    while (n--) {
        vector<SdfPath>::const_iterator
            i = paths.begin() + (rand() % paths.size());
        driver->EraseSubtree(*i);
    }
    sw.Stop();
    printf("Erased %zu subtrees in %f seconds\n", numIters, sw.GetSeconds());
}

struct PathTableDriver
{
    void Insert(SdfPath const &path) {
        map.insert(make_pair(path, 0));
    }

    void EraseSubtree(SdfPath const &path) {
        map.erase(path);
    }

    SdfPathTable<int> map;
};

struct HashAndSetDriver
{
    void Insert(SdfPath const &path) {
        hash.insert(make_pair(path, 0));
        pathSet.insert(path);
    }

    void EraseSubtree(SdfPath const &path) {
        SdfPathSet::iterator i = pathSet.lower_bound(path);
        while (*i == path || i->HasPrefix(path)) {
            hash.erase(*i);
            pathSet.erase(i++);
        }
    }

    TfHashMap<SdfPath, int, SdfPath::Hash> hash;
    SdfPathSet pathSet;
};

int
main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s {HashAndSet, PathTable} "
                "pathsFile\n", TfGetBaseName(argv[0]).c_str());
        fprintf(stderr, "running unit test.\n");
        DoUnitTest();
        exit(0);
    }

    vector<SdfPath> paths;
    ReadPaths(argv[2], &paths);

    srand(100);

    if (string(argv[1]) == "HashAndSet") {
        HashAndSetDriver driver;
        Bench(paths.size(), paths, &driver);
    } else if (string(argv[1]) == "PathTable") {
        PathTableDriver driver;
        Bench(paths.size(), paths, &driver);
    } else {
        fprintf(stderr, "invalid driver name\n");
    }

    printf(">>> Test SUCCEEDED\n");
    return 0;
}

