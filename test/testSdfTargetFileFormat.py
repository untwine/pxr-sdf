# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Modified by Jeremy Retailleau.

from pxr import Plug, Sdf
import os
import unittest

class TestSdfTargetFileFormat(unittest.TestCase):

    def test_CanReadBigCookies(self):
        root = os.environ["TEST_TARGET_FILE_FORMAT_PATH"]
        ext = "test_cookie_format"
        fileFormat = Sdf.FileFormat.FindByExtension(ext)
        self.assertTrue(fileFormat is not None)
        self.assertTrue(
            fileFormat.CanRead(os.path.join(root, "goodCookie.usda")))
        self.assertFalse(
            fileFormat.CanRead(os.path.join(root, "badCookie.usda")))

        # Sanity check: can read any cookie
        ext = "usda"
        fileFormat = Sdf.FileFormat.FindByExtension(ext)
        self.assertTrue(
            fileFormat.CanRead(os.path.join(root, "anyCookie.usda"))
        )

if __name__ == "__main__":
    unittest.main()
