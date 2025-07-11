# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Modified by Jeremy Retailleau.

from pxr import Sdf
import unittest, itertools

def _ExplicitItems(l):
    return Sdf.IntListOp.CreateExplicit(l)
def _AddedItems(l):
    r = Sdf.IntListOp()
    r.addedItems = l
    return r
def _PrependedItems(l):
    return Sdf.IntListOp.Create(prependedItems=l)
def _AppendedItems(l):
    return Sdf.IntListOp.Create(appendedItems=l)
def _DeletedItems(l):
    return Sdf.IntListOp.Create(deletedItems=l)
def _OrderedItems(l):
    r = Sdf.IntListOp()
    r.orderedItems = l
    return r

class TestSdfListOp(unittest.TestCase):
    def test_Basics(self):
        """Test basic behaviors across all ListOp types"""
        # Cheesy way of getting all ListOp types
        listOpTypes = [
            getattr(Sdf, t) for t in dir(Sdf) if t.endswith("ListOp")
        ]

        for t in listOpTypes:
            listOp = t.Create()
            self.assertFalse(listOp.isExplicit)

            explicitListOp = t.CreateExplicit()
            self.assertTrue(explicitListOp.isExplicit)

            self.assertNotEqual(listOp, explicitListOp)
            self.assertNotEqual(str(listOp), str(explicitListOp))

    def test_BasicSemantics(self):
        # Default empty listop does nothing.
        self.assertEqual(
            Sdf.IntListOp()
            .ApplyOperations([]),
            [])

        # Explicit items replace whatever was there previously.
        self.assertEqual(
            _ExplicitItems([1,2,3])
            .ApplyOperations([]),
            [1,2,3])
        self.assertEqual(
            _ExplicitItems([1,2,3])
            .ApplyOperations([0,3]),
            [1,2,3])
        self.assertEqual(
            _ExplicitItems([1,2,1,3])
            .ApplyOperations([]),
            [1,2,3])

        # Ensure duplicates are removed when using setter methods.
        self.assertEqual(
            _ExplicitItems([1])
            .explicitItems,
            [1])
        self.assertEqual(
            _ExplicitItems([1,1])
            .explicitItems,
            [1])
        self.assertEqual(
            _ExplicitItems([1,2,3])
            .explicitItems,
            [1,2,3])
        self.assertEqual(
            _ExplicitItems([1,2,3,4,5,6,7,8,9,10,11,11])
            .explicitItems,
            [1,2,3,4,5,6,7,8,9,10,11])
        self.assertEqual(
            _ExplicitItems([1,2,1,3])
            .explicitItems,
            [1,2,3])
        self.assertEqual(
            _DeletedItems([1,2,1,3])
            .deletedItems,
            [1,2,3])
        self.assertEqual(
            _AppendedItems([1,2,1,3])
            .appendedItems,
            [2,1,3])
        self.assertEqual(
            _PrependedItems([1,2,1,3])
            .prependedItems,
            [1,2,3])        

        # (deprecated)"Add" leaves existing values in place and appends any new values.
        self.assertEqual(
            _AddedItems([1,2,3])
            .ApplyOperations([]),
            [1,2,3])
        self.assertEqual(
            _AddedItems([1,2,3])
            .ApplyOperations([3,2,1]),
            [3,2,1])
        self.assertEqual(
            _AddedItems([1,2,3])
            .ApplyOperations([0,3]),
            [0,3,1,2])

        # "Delete" removes values and leaves the rest in place, in order.
        self.assertEqual(
            _DeletedItems([1,2,3])
            .ApplyOperations([]),
            [])
        self.assertEqual(
            _DeletedItems([1,2,3])
            .ApplyOperations([0,3,5]),
            [0,5])
        self.assertEqual(
            _DeletedItems([1,2,3])
            .ApplyOperations([1024,1]),
            [1024])

        # "Append" adds the given items to the end of the list, moving
        # them if they existed previously.
        self.assertEqual(
            _AppendedItems([1,2,3])
            .ApplyOperations([]),
            [1,2,3])
        self.assertEqual(
            _AppendedItems([1,2,3])
            .ApplyOperations([2]),
            [1,2,3])
        self.assertEqual(
            _AppendedItems([1,2,3])
            .ApplyOperations([3,4]),
            [4,1,2,3])
        self.assertEqual(
            _AppendedItems([1,2,1,3])
            .ApplyOperations([]),
            [2,1,3])

        # "Prepend" is similar, but for the front of the list.
        self.assertEqual(
            _PrependedItems([1,2,3])
            .ApplyOperations([]),
            [1,2,3])
        self.assertEqual(
            _PrependedItems([1,2,3])
            .ApplyOperations([2]),
            [1,2,3])
        self.assertEqual(
            _PrependedItems([1,2,3])
            .ApplyOperations([0,1]),
            [1,2,3,0])
        self.assertEqual(
            _PrependedItems([1,2,1,3])
            .ApplyOperations([]),
            [1,2,3])

        # (deprecated) "Order" is the most subtle.
        self.assertEqual(
            _OrderedItems([1,2,3])
            .ApplyOperations([]),
            [])
        self.assertEqual(
            _OrderedItems([1,2,3])
            .ApplyOperations([2]),
            [2])
        self.assertEqual(
            _OrderedItems([1,2,3])
            .ApplyOperations([2,1]),
            [1,2])
        self.assertEqual(
            _OrderedItems([1,2,3])
            .ApplyOperations([2,4,1]),
            [1,2,4])
        # Values that were not mentioned in the "ordered items" list
        # will end up in a position that depends on their position relative
        # to the nearest surrounding items that are in the ordered items list.
        self.assertEqual(
            _OrderedItems([1,2,3])
            .ApplyOperations([0,2,4,1,5,3]),
            [0,1,5,2,4,3])

    def test_Compose(self):
        # Confirm that listops using add or reorder are not composable.
        self.assertEqual(
            _OrderedItems([1,2,3]).ApplyOperations(_ExplicitItems([1,2])),
            None)
        self.assertEqual(
            _AddedItems([1,2,3]).ApplyOperations(_ExplicitItems([1,2])),
            None)
        # Explicit ops are composable over anything.
        self.assertEqual(
            _ExplicitItems([1,2,3]).ApplyOperations(_OrderedItems([1])),
            _ExplicitItems([1,2,3]))
        self.assertEqual(
            _ExplicitItems([1,2,3]).ApplyOperations(_AddedItems([1])),
            _ExplicitItems([1,2,3]))

        #
        # Exhaustive check of A(B(x)) == C(x), where C = A(B).
        #
        def powerset(s):
            return itertools.chain.from_iterable(
                    itertools.combinations(s, r) for r in range(len(s)+1))
        def generate_lists(num=3):
            lists = []
            for subset in powerset(tuple(range(num))):
                for perm in itertools.permutations(subset):
                    lists.append(perm)
            return lists
        def generate_composable_listops(num=3):
            lists = generate_lists(num)
            for explicit in lists:
                yield _ExplicitItems(explicit)
            for (a,b,c) in itertools.combinations_with_replacement(lists, 3):
                op = Sdf.IntListOp()
                (op.appendedItems, op.prependedItems, op.deletedItems) = (a,b,c)
                yield op
        ops = generate_composable_listops(3)
        lists = generate_lists(2)
        for (a,b) in itertools.combinations_with_replacement(ops, 2):
            c = a.ApplyOperations(b)
            for l in lists:
                self.assertEqual(
                    a.ApplyOperations(b.ApplyOperations(l)),
                    c.ApplyOperations(l))

    def test_HasItem(self):
        a = [1, 2, 3]

        listOp = _ExplicitItems(a)
        for v in a:
            self.assertTrue(listOp.HasItem(v))
        self.assertFalse(listOp.HasItem(4))

        listOp = _AddedItems(a)
        for v in a:
            self.assertTrue(listOp.HasItem(v))
        self.assertFalse(listOp.HasItem(4))

        listOp = _PrependedItems(a)
        for v in a:
            self.assertTrue(listOp.HasItem(v))
        self.assertFalse(listOp.HasItem(4))

        listOp = _AppendedItems(a)
        for v in a:
            self.assertTrue(listOp.HasItem(v))
        self.assertFalse(listOp.HasItem(4))

        listOp = _OrderedItems(a)
        for v in a:
            self.assertTrue(listOp.HasItem(v))
        self.assertFalse(listOp.HasItem(4))

        listOp = _DeletedItems(a)
        for v in a:
            self.assertTrue(listOp.HasItem(v))
        self.assertFalse(listOp.HasItem(4))

    def test_Hash(self):
        listOp = Sdf.IntListOp.Create(appendedItems = [1, 2, 3],
                                      prependedItems = [0, 8, 9],
                                      deletedItems = [-1, -2])
        self.assertEqual(hash(listOp), hash(listOp))
        self.assertEqual(
            hash(listOp),
            hash(
                Sdf.IntListOp.Create(
                    appendedItems=listOp.appendedItems,
                    prependedItems=listOp.prependedItems,
                    deletedItems=listOp.deletedItems
                )
            )
        )

    # Sanity test for binding, ApplyOperation logic exhaustively tested above
    def test_GetAppliedItems(self):
        e = [1, 2, 3]
        listOp = _ExplicitItems(e)
        items = listOp.GetAppliedItems()
        self.assertEqual(items, e)

        p = [4, 5, 6]
        a = [7, 8, 9]
        listOp = Sdf.IntListOp.Create(prependedItems=p, appendedItems=a)
        items = listOp.GetAppliedItems()
        self.assertEqual(items, p + a)

if __name__ == "__main__":
    unittest.main()
