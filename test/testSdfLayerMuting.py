# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Modified by Jeremy Retailleau.

from pxr import Sdf, Ar
import unittest
import os

class TestSdfLayerMuting(unittest.TestCase):
    
    def test_UnmuteMutedLayer(self):
        '''Unmuting an initially muted layer should unmute the 
           layer and load the file.'''

        root = os.environ['TEST_LAYER_MUTING_PATH']
        pathA = os.path.join(root, 'a.usda')
        layerA = Sdf.Layer.FindOrOpen(pathA)
        self.assertTrue(layerA is not None)
        self.assertFalse(layerA.IsMuted())
        self.assertFalse(layerA.empty)
        self.assertTrue(layerA.GetPrimAtPath('/Test') is not None)

        pathB = os.path.join(root, 'b.usda')

        # Windows paths must use resolved identifiers (slashes differ)
        # See https://github.com/PixarAnimationStudios/OpenUSD/issues/3721
        idA = Ar.GetResolver().CreateIdentifierForNewAsset(pathA)
        idB = Ar.GetResolver().CreateIdentifierForNewAsset(pathB)

        # Mute layer B before opening it
        Sdf.Layer.AddToMutedLayers(idB)

        layerB = Sdf.Layer.FindOrOpen(pathB)
        self.assertTrue(layerB is not None)
        self.assertTrue(layerB.IsMuted())
        self.assertTrue(layerB.empty)
        self.assertTrue(layerB.GetPrimAtPath('/Test') is None)

        # Now mute layer A and unmute layer B
        Sdf.Layer.AddToMutedLayers(idA)
        Sdf.Layer.RemoveFromMutedLayers(idB)

        self.assertTrue(layerA.IsMuted())
        self.assertTrue(layerA.empty)
        self.assertTrue(layerA.GetPrimAtPath('/Test') is None)

        self.assertFalse(layerB.IsMuted())
        self.assertFalse(layerB.empty)
        self.assertTrue(layerB.GetPrimAtPath('/Test') is not None)
        
if __name__ == '__main__':
    unittest.main()
