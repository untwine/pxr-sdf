# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Modified by Jeremy Retailleau.

from pxr import Sdf, Tf
import sys, unittest

MatchEval = Sdf._MakeBasicMatchEval

class TestSdfPathExpression(unittest.TestCase):

    def test_Basics(self):
        # Empty expr.
        pe = Sdf.PathExpression()
        self.assertTrue(pe.IsEmpty())
        self.assertEqual(pe, Sdf.PathExpression.Nothing())
        self.assertEqual(Sdf.PathExpression.MakeComplement(pe),
                         Sdf.PathExpression.Everything())
        self.assertEqual(
            pe, Sdf.PathExpression.MakeComplement(
                Sdf.PathExpression.Everything()))
        self.assertEqual(pe, Sdf.PathExpression(''))
        self.assertFalse(pe)

        # Leading & trailing whitespace.
        self.assertEqual(
            Sdf.PathExpression("  /foo//bar").GetText(), "/foo//bar")
        self.assertEqual(
            Sdf.PathExpression("  /foo//bar ").GetText(), "/foo//bar")
        self.assertEqual(
            Sdf.PathExpression("/foo//bar ").GetText(), "/foo//bar")
        self.assertEqual(
            Sdf.PathExpression("  /foo /bar").GetText(), "/foo /bar")
        self.assertEqual(
            Sdf.PathExpression("  /foo /bar ").GetText(), "/foo /bar")
        self.assertEqual(
            Sdf.PathExpression("/foo /bar ").GetText(), "/foo /bar")

        # Complement of complement should cancel.
        self.assertEqual(
            Sdf.PathExpression('~(~a)'), Sdf.PathExpression('a'))
        self.assertEqual(
            Sdf.PathExpression('~(~(~a))'), Sdf.PathExpression('~a'))
        self.assertEqual(
            Sdf.PathExpression('~(~(~(~a)))'), Sdf.PathExpression('a'))
        self.assertEqual(
            Sdf.PathExpression('// - a'), Sdf.PathExpression('~a'))
        self.assertEqual(
            Sdf.PathExpression('~(// - a)'), Sdf.PathExpression('a'))
        self.assertEqual(
            Sdf.PathExpression('~(// - ~a)'), Sdf.PathExpression('~a'))

    def test_Matching(self):

        evl = MatchEval('/foo/bar/*') 
        self.assertFalse(evl.Match('/foo'))
        self.assertFalse(evl.Match('/foo/bar'))
        self.assertTrue(evl.Match('/foo/bar/a'))
        self.assertTrue(evl.Match('/foo/bar/b'))
        self.assertTrue(evl.Match('/foo/bar/c'))
        self.assertFalse(evl.Match('/foo/bar/a/x'))
        self.assertFalse(evl.Match('/foo/bar/a/y'))
        self.assertFalse(evl.Match('/foo/bar/a/z'))
        
        evl = MatchEval('/foo//bar')
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz:buz")))

        evl = MatchEval("//foo/bar/baz/qux/quux")

        self.assertFalse(evl.Match(Sdf.Path("/foo")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/bar/baz/qux")))

        self.assertTrue(evl.Match(Sdf.Path("/foo/bar/baz/qux/quux")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/foo/bar/baz/qux/quux")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar/foo/bar/baz/qux/quux")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar/baz/foo/bar/baz/qux/quux")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar/baz/qux/foo/bar/baz/qux/quux")))

        evl = MatchEval("/foo*//bar")
        
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz:buz")))

        self.assertTrue(evl.Match(Sdf.Path("/foo1/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo12/x/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/fooBar/x/y/z/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/fooX/x/y/z/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/fooY/x/y/z/bar.baz")))
        self.assertFalse(evl.Match(Sdf.Path("/fooY/x/y/z/bar.baz:buz")))

        evl = MatchEval("/foo*//bar{isPrimPath}")
        
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar.baz:buz")))

        self.assertTrue(evl.Match(Sdf.Path("/foo1/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/foo12/x/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/fooBar/x/y/z/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/fooX/x/y/z/bar/baz")))
        self.assertFalse(evl.Match(Sdf.Path("/fooY/x/y/z/bar.baz")))
        self.assertFalse(evl.Match(Sdf.Path("/fooY/x/y/z/bar.baz:buz")))

        evl = MatchEval("/foo*//bar//{isPrimPath}")
        
        self.assertTrue(evl.Match(Sdf.Path("/foo/bar/a")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/bar/b")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar/c")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz/qux")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz.attr")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz/qux.attr")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/x/y/z/bar/baz/qux.ns:attr")))

        self.assertTrue(evl.Match(Sdf.Path("/fooXYZ/bar/a")))
        self.assertTrue(evl.Match(Sdf.Path("/fooABC/x/bar/a/b/c")))
        self.assertTrue(evl.Match(Sdf.Path("/foo123/x/y/z/bar/x")))
        self.assertTrue(evl.Match(Sdf.Path("/fooASDF/x/y/z/bar/baz")))
        self.assertTrue(evl.Match(Sdf.Path("/foo___/x/y/z/bar/baz/qux")))
        self.assertFalse(evl.Match(Sdf.Path("/foo_bar/x/y/z/bar/baz.attr")))
        self.assertFalse(evl.Match(Sdf.Path("/foo_baz/x/y/z/bar/baz/qux.attr")))
        self.assertFalse(
            evl.Match(Sdf.Path("/foo_baz/x/y/z/bar/baz/qux.ns:attr")))

        evl = MatchEval("/a /b /c /d/e/f")

        self.assertTrue(evl.Match(Sdf.Path("/a")))
        self.assertTrue(evl.Match(Sdf.Path("/b")))
        self.assertTrue(evl.Match(Sdf.Path("/c")))
        self.assertTrue(evl.Match(Sdf.Path("/d/e/f")))

        self.assertFalse(evl.Match(Sdf.Path("/a/b")))
        self.assertFalse(evl.Match(Sdf.Path("/b/c")))
        self.assertFalse(evl.Match(Sdf.Path("/c/d")))
        self.assertFalse(evl.Match(Sdf.Path("/d/e")))

        evl = MatchEval("/a// - /a/b/c")

        self.assertTrue(evl.Match(Sdf.Path("/a")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b/c")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/c/d")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/x")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/y")))

        evl = MatchEval("/a//{isPropertyPath} - /a/b.c")

        self.assertFalse(evl.Match(Sdf.Path("/a")))
        self.assertTrue(evl.Match(Sdf.Path("/a.b")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b.c")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b.ns:c")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b.yes")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b.ns:yes")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b/c")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/c.d")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/c.ns:d")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b/x")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/x.y")))
        self.assertTrue(evl.Match(Sdf.Path("/a/b/x.ns:y")))

    def test_ComposeOver(self):
        # ComposeOver
        a = Sdf.PathExpression("/a")
        b = Sdf.PathExpression("%_ /b")
        c = Sdf.PathExpression("%_ /c")

        self.assertFalse(a.ContainsExpressionReferences())
        self.assertFalse(a.ContainsWeakerExpressionReference())
        self.assertTrue(b.ContainsExpressionReferences())
        self.assertTrue(b.ContainsWeakerExpressionReference())
        self.assertTrue(c.ContainsExpressionReferences())
        self.assertTrue(c.ContainsWeakerExpressionReference())
        
        composed = c.ComposeOver(b).ComposeOver(a)

        self.assertFalse(composed.ContainsExpressionReferences())
        self.assertFalse(composed.ContainsWeakerExpressionReference())
        self.assertTrue(composed.IsComplete())
        
        evl = MatchEval(composed.GetText())

        self.assertTrue(evl.Match(Sdf.Path("/a")))
        self.assertTrue(evl.Match(Sdf.Path("/b")))
        self.assertTrue(evl.Match(Sdf.Path("/c")))
        self.assertFalse(evl.Match(Sdf.Path("/d")))

    def test_ResolveReferences(self):

        refs = Sdf.PathExpression("/a %_ %:foo - %:bar")
        weaker = Sdf.PathExpression("/weaker")
        foo = Sdf.PathExpression("/foo//")
        bar = Sdf.PathExpression("/foo/bar//")
        
        self.assertTrue(refs.ContainsExpressionReferences())
        self.assertFalse(weaker.ContainsExpressionReferences())
        self.assertFalse(foo.ContainsExpressionReferences())
        self.assertFalse(bar.ContainsExpressionReferences())

        def resolveRefs(ref):
            if ref.name == "_":
                return weaker
            elif ref.name == "foo":
                return foo
            elif ref.name == "bar":
                return bar
            else:
                return Sdf.PathExpression()

        resolved = refs.ResolveReferences(resolveRefs)

        self.assertFalse(resolved.ContainsExpressionReferences())
        self.assertTrue(resolved.IsComplete())

        # Resolved should be "/a /weaker /foo// - /foo/bar//"
        evl = MatchEval(resolved.GetText())

        self.assertTrue(evl.Match(Sdf.Path("/a")))
        self.assertTrue(evl.Match(Sdf.Path("/weaker")))
        self.assertTrue(evl.Match(Sdf.Path("/foo")))
        self.assertTrue(evl.Match(Sdf.Path("/foo/child")))
        self.assertFalse(evl.Match(Sdf.Path("/a/b")))
        self.assertFalse(evl.Match(Sdf.Path("/weaker/c")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/bar")))
        self.assertFalse(evl.Match(Sdf.Path("/foo/bar/baz")))

        # ResolveReferences() with the empty expression should produce the empty
        # expression.
        self.assertTrue(
            Sdf.PathExpression().ResolveReferences(resolveRefs).IsEmpty())

    def test_MakeAbsolute(self):
        # Check MakeAbsolute.
        e = Sdf.PathExpression("foo ../bar baz//qux")
        self.assertFalse(e.IsAbsolute())
        self.assertFalse(e.ContainsExpressionReferences())
        abso = e.MakeAbsolute(Sdf.Path("/World/test"))
        # abso should be: "/World/test/foo /World/bar /World/test/baz//qux"
        self.assertTrue(abso.IsAbsolute())
        self.assertTrue(abso.IsComplete())

        evl = MatchEval(abso.GetText())

        self.assertTrue(evl.Match(Sdf.Path("/World/test/foo")))
        self.assertFalse(evl.Match(Sdf.Path("/World/test/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/World/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/World/test/baz/qux")))
        self.assertTrue(evl.Match(Sdf.Path("/World/test/baz/a/b/c/qux")))

    def test_ReplacePrefix(self):
        abso = Sdf.PathExpression(
            "/World/test/foo /World/bar /World/test/baz//qux")
        home = abso.ReplacePrefix(Sdf.Path("/World"), Sdf.Path("/Home"))
            
        evl = MatchEval(home.GetText())

        self.assertTrue(evl.Match(Sdf.Path("/Home/test/foo")))
        self.assertFalse(evl.Match(Sdf.Path("/Home/test/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/Home/bar")))
        self.assertTrue(evl.Match(Sdf.Path("/Home/test/baz/qux")))
        self.assertTrue(evl.Match(Sdf.Path("/Home/test/baz/a/b/c/qux")))

    def test_PrefixConstancy(self):
        # Check constancy wrt prefix relations.
        evl = MatchEval("/prefix/path//")

        self.assertFalse(evl.Match(Sdf.Path("/prefix")))
        self.assertFalse(evl.Match(Sdf.Path("/prefix")).IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/prefix/path")))
        self.assertTrue(evl.Match(Sdf.Path("/prefix/path")).IsConstant())
        self.assertFalse(evl.Match(Sdf.Path("/prefix/wrong")))
        self.assertTrue(evl.Match(Sdf.Path("/prefix/wrong")).IsConstant())
        
        evl = MatchEval("//World//")
        self.assertTrue(evl.Match(Sdf.Path("/World")))
        self.assertTrue(evl.Match(Sdf.Path("/World")).IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/World/Foo")))
        self.assertTrue(evl.Match(Sdf.Path("/World/Foo")).IsConstant())

        evl = MatchEval("//World//Foo/Bar//")
        self.assertTrue(evl.Match(Sdf.Path("/World/Foo/Bar")))
        self.assertTrue(evl.Match(Sdf.Path("/World/Foo/Bar")).IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Foo/Bar")))
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Foo/Bar")).IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar")))
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar")).IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar/Baz")))
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar/Baz"))
                        .IsConstant())
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar/Baz/Qux")))
        self.assertTrue(evl.Match(Sdf.Path("/World/X/Y/Foo/Bar/Baz/Qux"))
                        .IsConstant())

    def test_SceneDescription(self):
        l = Sdf.Layer.CreateAnonymous()
        prim = Sdf.CreatePrimInLayer(l, "/prim")
        attr = Sdf.AttributeSpec(
            prim, "attr", Sdf.ValueTypeNames.PathExpression)
        self.assertTrue(attr)
        attr.default = Sdf.PathExpression("child")
        # Should have been made absolute:
        self.assertEqual(attr.default, Sdf.PathExpression("/prim/child"))

    def test_PathPattern(self):
        self.assertIs(Sdf.PathPattern, Sdf.PathExpression.PathPattern)
        
        pat = Sdf.PathPattern()
        self.assertFalse(pat)
        self.assertFalse(pat.HasTrailingStretch())
        self.assertTrue(pat.GetPrefix().isEmpty)
        self.assertTrue(pat.CanAppendChild(''))
        self.assertTrue(pat.AppendChild(''))
        self.assertEqual(pat, Sdf.PathPattern.EveryDescendant())
        self.assertTrue(pat.HasTrailingStretch())
        self.assertEqual(pat.GetPrefix(), Sdf.Path.reflexiveRelativePath)
        self.assertFalse(pat.HasLeadingStretch())

        # Set prefix to '/', should become Everything().
        pat.SetPrefix(Sdf.Path.absoluteRootPath)
        self.assertEqual(pat, Sdf.PathPattern.Everything())
        self.assertTrue(pat.HasLeadingStretch())
        self.assertTrue(pat.HasTrailingStretch())

        # Remove trailing stretch, should become just '/'
        pat.RemoveTrailingStretch()
        self.assertFalse(pat.HasLeadingStretch())
        self.assertFalse(pat.HasTrailingStretch())
        self.assertEqual(pat.GetPrefix(), Sdf.Path.absoluteRootPath)

        # Add some components.
        pat.AppendChild("foo").AppendChild("bar").AppendChild("baz")
        # This should have modified the prefix path, rather than appending
        # matching components.
        self.assertEqual(pat.GetPrefix(), Sdf.Path("/foo/bar/baz"))

        # Appending a property to a pattern with trailing stretch has to append
        # a prim wildcard '*'.
        pat.AppendStretchIfPossible().AppendProperty("prop")
        self.assertTrue(pat.IsProperty())
        self.assertEqual(pat.GetText(), "/foo/bar/baz//*.prop")

        # Can't append children or properties to property patterns.
        self.assertFalse(pat.CanAppendChild("foo"))
        self.assertFalse(pat.CanAppendProperty("foo"))

        pat.RemoveTrailingComponent()
        self.assertEqual(pat.GetText(), "/foo/bar/baz//*")
        pat.RemoveTrailingComponent()
        self.assertEqual(pat.GetText(), "/foo/bar/baz//")
        pat.RemoveTrailingComponent()
        self.assertEqual(pat.GetText(), "/foo/bar/baz")
        pat.RemoveTrailingComponent() # No more components, only prefix.
        self.assertEqual(pat.GetText(), "/foo/bar/baz")

if __name__ == '__main__':
    unittest.main()
