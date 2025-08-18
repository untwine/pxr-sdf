# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Modified by Jeremy Retailleau.

from pxr import Sdf
import unittest
import os

class TestSdfCustomLayer(unittest.TestCase):
    # Test the customLayerData API via Sdf's Layer API
    def test_BasicUsage(self):
        root = os.environ.get('TEST_CUSTOM_LAYER_DATA_PATH')
        filePath = os.path.join(root, 'layerAccess.usda')
        layer = Sdf.Layer.FindOrOpen(filePath)
        self.assertTrue(layer is not None)

        expectedValue = { 'layerAccessId' : 'id',
                          'layerAccessAssetPath' : Sdf.AssetPath('/layer/access.usda'),
                          'layerAccessRandomNumber' : 5 }
        self.assertEqual(layer.customLayerData, expectedValue)

        self.assertTrue(layer.HasCustomLayerData())
        layer.ClearCustomLayerData()
        self.assertFalse(layer.HasCustomLayerData())

        newValue = { 'newLayerAccessId' : 'newId',
                     'newLayerAccessAssetPath' : Sdf.AssetPath('/new/layer/access.usda'),
                     'newLayerAccessRandomNumber' : 1 }
        layer.customLayerData = newValue
        self.assertEqual(layer.customLayerData, newValue)

        self.assertTrue(layer.HasCustomLayerData())
        layer.ClearCustomLayerData()
        self.assertFalse(layer.HasCustomLayerData())

if __name__ == '__main__':
    unittest.main()
