# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Modified by Jeremy Retailleau.

import os
import unittest
import zipfile

from pxr import Sdf

class TestSdfZipFile(unittest.TestCase):
    def _ValidateSourceAndZippedFile(self, srcFile, zipFile, fileInZip, 
                                     fixLineEndings=False):
        with open(srcFile, "rb") as f:
            srcData = bytearray(f.read())
            if fixLineEndings:
                srcData = srcData.replace("\r\n".encode("utf-8"),
                                          "\n".encode("utf-8"))

        if isinstance(zipFile, Sdf.ZipFile):
            self.assertEqual(
                srcData, zipFile.GetFile(fileInZip))
        elif isinstance(zipFile, zipfile.ZipFile):
            self.assertEqual(srcData, zipFile.read(fileInZip))

    def test_Reader(self):
        """Test Sdf.ZipFileReader"""
        root = os.environ["TEST_ZIP_FILE_PATH"]
        pathA = os.path.join(root, "src/a.test")
        pathB = os.path.join(root, "src/b.png")
        pathC = os.path.join(root, "src/sub/c.png")
        pathD = os.path.join(root, "src/sub/d.txt")

        zf = Sdf.ZipFile.Open(os.path.join(root, "nonexistent.usdz"))
        self.assertIsNone(zf)

        zf = Sdf.ZipFile.Open(os.path.join(root, "test_reader.usdz"))
        self.assertTrue(zf)
        self.assertEqual(
            zf.GetFileNames(), ["a.test", "b.png", "sub/c.png", "sub/d.txt"])

        self.assertIsNone(zf.GetFile("nonexistent.txt"))
        self.assertIsNone(zf.GetFileInfo("nonexistent.txt"))

        # test_reader.usdz was created with text files with Unix-style
        # line endings. Fix up line endings in the baseline files to
        # accommodate this, in case those files had line ending
        # translations applied.
        fixLineEndings = True

        fileInfo = zf.GetFileInfo("a.test")
        self.assertEqual(fileInfo.dataOffset, 64)
        self.assertEqual(fileInfo.size, 83)
        self.assertEqual(fileInfo.uncompressedSize, 83)
        self.assertEqual(fileInfo.crc, 2187659876)
        self.assertEqual(fileInfo.compressionMethod, 0)
        self.assertFalse(fileInfo.encrypted)
        self._ValidateSourceAndZippedFile(
            pathA, zf, "a.test", fixLineEndings)

        fileInfo = zf.GetFileInfo("b.png")
        self.assertEqual(fileInfo.dataOffset, 192)
        self.assertEqual(fileInfo.size, 7228)
        self.assertEqual(fileInfo.uncompressedSize, 7228)
        self.assertEqual(fileInfo.crc, 384784137)
        self.assertEqual(fileInfo.compressionMethod, 0)
        self.assertFalse(fileInfo.encrypted)
        self._ValidateSourceAndZippedFile(pathB, zf, "b.png")

        fileInfo = zf.GetFileInfo("sub/c.png")
        self.assertEqual(fileInfo.dataOffset, 7488)
        self.assertEqual(fileInfo.size, 6139)
        self.assertEqual(fileInfo.uncompressedSize, 6139)
        self.assertEqual(fileInfo.crc, 2488450460)
        self.assertEqual(fileInfo.compressionMethod, 0)
        self.assertFalse(fileInfo.encrypted)
        self._ValidateSourceAndZippedFile(pathC, zf, "sub/c.png")

        fileInfo = zf.GetFileInfo("sub/d.txt")
        self.assertEqual(fileInfo.dataOffset, 13696)
        self.assertEqual(fileInfo.size, 87)
        self.assertEqual(fileInfo.uncompressedSize, 87)
        self.assertEqual(fileInfo.crc, 2546026356)
        self.assertEqual(fileInfo.compressionMethod, 0)
        self.assertFalse(fileInfo.encrypted)
        self._ValidateSourceAndZippedFile(
            pathD, zf, "sub/d.txt", fixLineEndings)
        
    def test_Writer(self):
        """Test Sdf.ZipFileWriter"""
        root = os.environ["TEST_ZIP_FILE_PATH"]
        pathA = root + "/src/a.test"
        pathB = root + "/src/b.png"

        if os.path.isfile("test_writer.usdz"):
            os.remove("test_writer.usdz")

        with Sdf.ZipFileWriter.CreateNew("test_writer.usdz") as zfw:
            self.assertTrue(zfw)

            # Zip file should not be created until Save() is called on file
            # writer or the writer is destroyed.
            self.assertFalse(os.path.isfile("test_writer.usdz"))
            
            addedFile = zfw.AddFile(pathA)
            self.assertEqual(addedFile, pathA)

            addedFile = zfw.AddFile(pathB, "b.png")
            self.assertEqual(addedFile, "b.png")

        self.assertTrue(os.path.isfile("test_writer.usdz"))
        
        # Verify that the zip file can be read by Sdf.ZipFile.
        zf = Sdf.ZipFile.Open("test_writer.usdz")
        self.assertEqual(zf.GetFileNames(), [pathA, "b.png"])

        # Since we're writing files into a .usdz and then extracting
        # and comparing them to the original file, we don't need to
        # do any line ending fix-ups.
        dontFixLineEndings = False

        def _GetFileSize(file):
            return os.stat(file).st_size

        def _ComputeCRC(file):
            import zlib
            with open(file, "rb") as f:
                return zlib.crc32(f.read())

        fileInfo = zf.GetFileInfo(pathA)
        self.assertEqual(fileInfo.dataOffset, 64)
        self.assertEqual(fileInfo.size, _GetFileSize(pathA))
        self.assertEqual(fileInfo.uncompressedSize, _GetFileSize(pathA))
        self.assertEqual(fileInfo.crc, _ComputeCRC(pathA))
        self.assertEqual(fileInfo.compressionMethod, 0)
        self.assertFalse(fileInfo.encrypted)

        fileInfo = zf.GetFileInfo("b.png")
        self.assertEqual(fileInfo.dataOffset, 192)
        self.assertEqual(fileInfo.size, 7228)
        self.assertEqual(fileInfo.uncompressedSize, 7228)
        self.assertEqual(fileInfo.crc, 384784137)
        self.assertEqual(fileInfo.compressionMethod, 0)
        self.assertFalse(fileInfo.encrypted)

        self._ValidateSourceAndZippedFile(
            pathA, zf, pathA, dontFixLineEndings)
        self._ValidateSourceAndZippedFile(pathB, zf, "b.png")

        # Verify that the data offset for all files in the archive are
        # aligned on 64-byte boundaries.
        for f in zf.GetFileNames():
            self.assertEqual(zf.GetFileInfo(f).dataOffset % 64, 0)

        # Verify that zip file can be read by third-party zip libraries
        # (in this case, Python's zip module)
        zf = zipfile.ZipFile("test_writer.usdz")
        self.assertEqual(zf.namelist(), [pathA, "b.png"])
        self.assertIsNone(zf.testzip())

        self._ValidateSourceAndZippedFile(
            pathA, zf, pathA, dontFixLineEndings)
        self._ValidateSourceAndZippedFile(pathB, zf, "b.png")

    def test_WriterAlignment(self):
        """Test that Sdf.ZipFileWriter writes files so that they're aligned
        according to the .usdz specification"""
        with open("test_align_2.txt", "wb") as f:
            f.write("This is a test file".encode("utf-8"))

        # Create .usdz files with two files, where the size of the first file 
        # varies from 1 byte to 64 bytes, then verify that the second file's 
        # data is aligned to 64 bytes.
        for i in range(1, 65):
            with open("test_align_1.txt", "wb") as f:
                f.write(("a" * i).encode("utf-8"))

            with Sdf.ZipFileWriter.CreateNew("test_align.usdz") as zfw:
                zfw.AddFile("test_align_1.txt")
                zfw.AddFile("test_align_2.txt")
            
            zf = Sdf.ZipFile.Open("test_align.usdz")
            fi = zf.GetFileInfo("test_align_1.txt")
            self.assertEqual(fi.dataOffset % 64, 0)

            fi = zf.GetFileInfo("test_align_2.txt")
            self.assertEqual(fi.dataOffset % 64, 0)
            
    def test_WriterDiscard(self):
        if os.path.isfile("test_discard.usdz"):
            os.remove("test_discard.usdz")

        with Sdf.ZipFileWriter.CreateNew("test_discard.usdz") as zfw:
            self.assertTrue(zfw)
        
            # Zip file should not be written if Discard() is called.
            zfw.Discard()
            self.assertFalse(os.path.isfile("test_discard.usdz"))

        self.assertFalse(os.path.isfile("test_discard.usdz"))

    def test_WriterEmptyArchive(self):
        """Test corner case writing an empty zip archive with 
        Sdf.ZipFileWriter"""
        if os.path.isfile("empty.usdz"):
            os.remove("empty.usdz")

        with Sdf.ZipFileWriter.CreateNew("empty.usdz"):
            pass
        self.assertTrue(os.path.isfile("empty.usdz"))

        # Verify that zip file can be read by Sdf.ZipFile
        zf = Sdf.ZipFile.Open("empty.usdz")
        self.assertTrue(zf)
        self.assertEqual(zf.GetFileNames(), [])

        # Verify that zip file can be read by third-party zip libraries
        # (in this case, Python's zip module)
        zf = zipfile.ZipFile("empty.usdz")
        self.assertIsNone(zf.testzip())
        self.assertEqual(zf.namelist(), [])

if __name__ == "__main__":
    unittest.main()

