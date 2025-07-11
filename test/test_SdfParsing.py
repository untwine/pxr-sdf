# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Modified by Jeremy Retailleau.

# This tests a set of sample sdf files that are either expected to load
# successfully, or to emit warnings.  Files with _bad_ in the name are
# expected to emit warnings, but in no case should they cause a crash.
#
# For files that we expect to read successfully, we take the further step
# of writing them out, reading in what we wrote, and writing it out again,
# and then comparing the two versions written to make sure they are the
# same.  This is to detect any accumulative error, such as quoting or
# escaping errors.

from __future__ import print_function
import sys, os, difflib, unittest, platform
from pxr import Tf, Sdf

# Default encoding on Windows in python 3+ is not UTF-8, but it is on Linux &
# Mac.  Provide a wrapper here so we specify that in that case.
def _open(*args, **kw):
    if platform.system() == "Windows":
        kw['encoding'] = 'utf8'
        return open(*args, **kw)
    else:
        return open(*args, **kw)


def removeFiles(*filenames):
    """Removes the given files (if one of the args is itself a tuple or list, "unwrap" it.)"""
    for f in filenames:
        if isinstance(f, (tuple, list)):
            removeFiles(*f)
        else:
            try:
                os.remove(f)
            except OSError:
                pass

class TestSdfParsing(unittest.TestCase):
    def test_Basic(self):
        # NOTE: Any file with "_bad_" in the name is a file that should not load successfully.
        # NOTE: This list is reverse sorted.  Add new tests at the top!
        # This will mean that your new test runs first and you can spot
        # failures much quicker.
        testFiles = '''
        225_multiline_with_SplineKnotParamList.sdf
        224_spline_post_shaping_with_comment.sdf
        223_bad_spline_post_shaping_spacing.sdf
        222_dict_key_control_characters.sdf
        221_bad_spline_type.sdf
        220_splines.sdf
        219_utf8_bad_type_name.sdf
        218_utf8_bad_identifier.sdf
        217_utf8_identifiers.sdf
        216_bad_variant_in_relocates_path.sdf
        215_bad_variant_in_specializes_path.sdf
        214_bad_variant_in_inherits_path.sdf
        213_bad_variant_in_payload_path.sdf
        212_bad_variant_in_reference_path.sdf
        211_bad_authored_opaque_attributes.sdf
        210_opaque_attributes.sdf
        209_bad_escaped_string4.sdf
        208_bad_escaped_string3.sdf
        207_bad_escaped_string2.sdf
        206_bad_escaped_string1.sdf
        205_bad_assetPaths.sdf
        204_really_empty.sdf
        203_newlines.sdf
        202_displayGroups.sdf
        201_format_specifiers_in_strings.sdf
        200_bad_emptyFile.sdf
        199_bad_colorSpace_metadata.sdf
        198_colorSpace_metadata.sdf
        197_bad_colorConfiguration_metadata.sdf
        196_colorConfiguration_metadata.sdf
        195_specializes.sdf
        194_bad_customLayerData_metadata.sdf
        193_customLayerData_metadata.sdf
        192_listop_metadata.sdf
        191_instanceable.sdf
        190_property_assetInfo.sdf
        189_prim_assetInfo.sdf
        188_defaultRefTarget_metadata.sdf
        187_displayName_metadata.sdf
        186_bad_prefix_substitution_key.sdf
        185_namespaced_properties.sdf
        184_def_AnyType.sdf
        183_unknown_type_and_metadata.sdf
        183_time_samples.sdf
        182_bad_variant_in_relationship.sdf
        181_bad_variant_in_connection.sdf
        180_asset_paths.sdf
        179_bad_shaped_attr_dimensions1_oldtypes.sdf
        179_bad_shaped_attr_dimensions1.sdf
        178_invalid_typeName.sdf
        177_bad_empty_lists.sdf
        176_empty_lists.sdf
        175_asset_path_with_colons.sdf
        163_bad_variant_selection2.sdf
        162_bad_variant_selection1.sdf
        161_bad_variant_name2.sdf
        160_bad_variant_name1.sdf
        159_symmetryFunction_empty.sdf
        155_bad_relationship_noLoadHint.sdf
        154_relationship_noLoadHint.sdf
        153_bad_payloads.sdf
        152_payloads.sdf
        150_bad_kind_metadata_1.sdf
        149_kind_metadata.sdf
        148_relocates_empty_map.sdf
        147_bad_relocates_formatting_5.sdf
        146_bad_relocates_formatting_4.sdf
        145_bad_relocates_formatting_3.sdf
        144_bad_relocates_formatting_2.sdf
        143_bad_relocates_formatting_1.sdf
        142_bad_relocates_paths_3.sdf
        141_bad_relocates_paths_2.sdf
        140_bad_relocates_paths_1.sdf
        139_relocates_metadata.sdf
        133_bad_reference.sdf
        132_references.sdf
        127_varyingRelationship.sdf
        119_bad_permission_metadata_3.sdf
        118_bad_permission_metadata_2.sdf
        117_bad_permission_metadata.sdf
        116_permission_metadata.sdf
        115_symmetricPeer_metadata.sdf
        114_bad_prefix_metadata.sdf
        113_displayName_metadata.sdf
        112_nested_dictionaries_oldtypes.sdf
        112_nested_dictionaries.sdf
        111_string_arrays.sdf
        108_bad_inheritPath.sdf
        104_uniformAttributes.sdf
        103_bad_attributeVariability.sdf
        100_bad_roleNameChange.sdf
        99_bad_typeNameChange.sdf
        98_bad_valueType.sdf
        97_bad_valueType.sdf
        96_bad_valueType_oldtypes.sdf
        96_bad_valueType.sdf
        95_bad_hiddenRel.sdf
        94_bad_hiddenAttr.sdf
        93_hidden.sdf
        92_bad_variantSelectionType.sdf
        91_bad_valueType.sdf
        90_bad_dupePrim.sdf
        89_bad_attribute_displayUnit.sdf
        88_attribute_displayUnit.sdf
        86_bad_tuple_dimensions5.sdf
        85_bad_tuple_dimensions4_oldtypes.sdf
        85_bad_tuple_dimensions4.sdf
        84_bad_tuple_dimensions3_oldtypes.sdf
        84_bad_tuple_dimensions3.sdf
        83_bad_tuple_dimensions2_oldtypes.sdf
        83_bad_tuple_dimensions2.sdf
        82_bad_tuple_dimensions1_oldtypes.sdf
        82_bad_tuple_dimensions1.sdf
        81_namespace_reorder.sdf
        80_bad_hidden.sdf
        76_relationship_customData.sdf
        75_attribute_customData.sdf
        74_prim_customData.sdf
        71_empty_shaped_attrs.sdf
        70_bad_list.sdf
        69_bad_list.sdf
        66_bad_attrVariability.sdf
        64_bad_boolPrimInstantiate.sdf
        61_bad_primName.sdf
        60_bad_groupListEditing.sdf
        59_bad_connectListEditing.sdf
        58_bad_relListEditing.sdf
        57_bad_relListEditing.sdf
        56_bad_value_oldtypes.sdf
        56_bad_value.sdf
        55_bad_value_oldtypes.sdf
        55_bad_value.sdf
        54_bad_value.sdf
        53_bad_typeName.sdf
        52_bad_attr.sdf
        51_propPath.sdf
        50_bad_primPath.sdf
        49_bad_list.sdf
        47_miscSceneInfo.sdf
        46_weirdStringContent.sdf
        45_rareValueTypes.sdf
        42_bad_noNewlineBetweenComps.sdf
        41_noEndingNewline.sdf
        40_repeater.sdf
        39_variants.sdf
        38_attribute_connections.sdf
        37_keyword_properties.sdf
        36_tasks.sdf
        33_bad_relationship_duplicate_target.sdf
        32_relationship_syntax.sdf
        31_attribute_values_oldtypes.sdf
        31_attribute_values.sdf
        30_bad_specifier.sdf
        29_bad_newline9.sdf
        28_bad_newline8.sdf
        27_bad_newline7.sdf
        26_bad_newline6.sdf
        25_bad_newline5.sdf
        24_bad_newline4.sdf
        23_bad_newline3.sdf
        22_bad_newline2.sdf
        21_bad_newline1.sdf
        20_optionalsemicolons.sdf
        16_bad_list.sdf
        15_bad_list.sdf
        14_bad_value.sdf
        13_bad_value.sdf
        12_bad_value.sdf
        11_debug.sdf
        10_bad_value.sdf
        09_bad_type.sdf
        08_bad_file.sdf
        06_largevalue.sdf
        05_bad_file.sdf
        04_general_oldtypes.sdf
        04_general.sdf
        03_bad_file.sdf
        02_simple.sdf
        01_empty.sdf
        '''.split()
        # NOTE:  READ IF YOU ARE ADDING TEST FILES
        # This list is reverse sorted.  Add new tests at the top!
        # This will mean that your new test runs first and you can spot
        # failures much quicker.

        # Disabled tests for invalid metadata field names because now invalid metadata
        # fields are stored as opaque text and passed through loading and saving
        # instead of causing parse errors
        #
        # 19_bad_relationshipaccess.sdf
        # 18_bad_primaccess.sdf
        # 17_bad_attributeaccess.sdf

        # Disabled tests - this has not failed properly ever, but a bug in this script masked the problem
        # 34_bad_relationship_duplicate_target_attr.sdf

        # Create a temporary file for diffs and choose where to get test data.
        def CreateTempFile(name):
            import tempfile
            layerFileOut = tempfile.NamedTemporaryFile(
                suffix='_' + name + '_testSdfParsing1.sdf', delete=False)
            # Close the temporary file.  We only wanted a temporary file name
            # and we'll open/close/remove this file once per test file.  On
            # Unix this isn't necessary because holding a file open doesn't
            # prevent unlinking it but on Windows we'll get access denied if
            # we don't close our handle.
            layerFileOut.close()
            return layerFileOut
        
        layerFileOut = CreateTempFile('Export')
        layerFileOut2 = CreateTempFile('ExportToString')
        layerFileOut3 = CreateTempFile('MetadataOnly')

        layerDir = os.environ["TEST_PARSING_PATH"]
        baselineDir = os.path.join(layerDir, 'baseline')

        print("LAYERDIR: %s"%layerDir)

        # Register test plugin containing plugin metadata definitions.
        from pxr import Plug
        Plug.Registry().RegisterPlugins(layerDir)

        def GenerateBaselines(testFiles, baselineDir):
            for file in testFiles:
                if file.startswith('#') or file == '' or '_bad_' in file:
                    continue
                
                try:
                    layerFile = "%s/%s" % (layerDir, file)
                    exportFile = "%s/%s" % (baselineDir, file)

                    layer = Sdf.Layer.FindOrOpen(layerFile)
                    layer.Export(exportFile)

                except:
                    print('Unable to export layer %s to %s' % (layerFile, exportFile))

        # Helper code to generate baseline layers. Uncomment to export 'good' layers
        # in the test list to the specified directory.
        #
        # GenerateBaselines(testFiles, '/tmp/baseline')
        # sys.exit(1)

        for file in testFiles:
            if file.startswith('#'):
                continue

            # Remove stale files or files from last pass
            removeFiles(layerFileOut.name)

            layerFile = file
            if file != "":
                layerFile = "%s/%s"%(layerDir, file)

            if (file == "") or ('_bad_' in file):
                print('\nTest bad file "%s"' % layerFile)
                print('\tReading "%s"' % layerFile)
                try:
                    layer = Sdf.Layer.FindOrOpen( layerFile )
                except Tf.ErrorException:
                    # Parsing errors should always be Tf.ErrorExceptions
                    print('\tErrors encountered, as expected')
                    print('\tPassed')
                    continue
                except:
                    # Empty file fails with a different error, and that's ok
                    if file != "":
                        print('\tNon-TfError encountered')
                        print('\tFAILED')
                        raise RuntimeError("failure to load '%s' should cause Tf.ErrorException, not some other failure" % layerFile)
                    else:
                        print('\tErrors encountered, as expected')
                        print('\tPassed')
                        continue
                else:
                    raise RuntimeError("should not be able to load '%s'" % layerFile)

            print('\nTest %s' % layerFile)

            print('\tReading...')
            layer = Sdf.Layer.FindOrOpen(layerFile)
            self.assertTrue(layer is not None,
                            "failed to open @%s@" % layerFile)

            metadataOnlyLayer = Sdf.Layer.OpenAsAnonymous(
                layerFile, metadataOnly=True)
            self.assertTrue(metadataOnlyLayer is not None,
                            "failed to open @%s@ for metadata only" % layerFile)

            print('\tWriting...')
            try:
                # Use Sdf.Layer.Export to write out the layer contents directly.
                self.assertTrue(layer.Export( layerFileOut.name ))

                # Use Sdf.Layer.ExportToString and write out the returned layer
                # string to a file.
                with _open(layerFileOut2.name, 'w') as f:
                    f.write(layer.ExportToString())

                self.assertTrue(metadataOnlyLayer.Export(layerFileOut3.name))
                    
            except Exception as e:
                if '_badwrite_' in file:
                    # Write errors should always be Tf.ErrorExceptions
                    print('\tErrors encountered during write, as expected')
                    print('\tPassed')
                    continue
                else:
                    raise RuntimeError("failure to write '%s': %s" % (layerFile, e))

            # Compare the exported layer against baseline results. Note that we can't
            # simply compare against the original layer, because exporting may have
            # applied formatting and other changes that cause the files to be different,
            # even though the scene description is the same.
            print('\tComparing against expected results...')

            expectedFile = "%s/%s" % (baselineDir, file)

            def doDiff(testFile, expectedFile, metadataOnly=False):

                fd = _open(testFile, "r")
                layerData = fd.readlines()
                fd.close()
                fd = _open(expectedFile, "r")
                expectedLayerData = fd.readlines()
                fd.close()

                # If we're expecting metadata only, find the metadata section in
                # the baseline output by looking for the "(" and ")" delimiters
                # and use that as the expected layer data.
                if metadataOnly:
                    try:
                        mdStart = expectedLayerData.index("(\n", 1)
                        mdEnd = expectedLayerData.index(")\n", mdStart)
                        expectedLayerData = expectedLayerData[0:mdEnd+1]+["\n"]
                    except ValueError:
                        # If there's no layer metadata, we expect to just have
                        # the first line of the baseline with the #sdf cookie.
                        expectedLayerData = [expectedLayerData[0], "\n"]

                diff = list(difflib.unified_diff(
                    layerData, expectedLayerData,
                    testFile, expectedFile))
                if diff:
                    print("ERROR: '%s' and '%s' are different." % \
                          (testFile, expectedFile))
                    for line in diff:
                        print(line, end=' ')
                    sys.exit(1)

            doDiff(layerFileOut.name, expectedFile)
            doDiff(layerFileOut2.name, expectedFile)
            doDiff(layerFileOut3.name, expectedFile, metadataOnly=True)
            
            print('\tPassed')

        removeFiles(layerFileOut.name, layerFileOut2.name)

        self.assertEqual(None, Sdf.Layer.FindOrOpen(''))

        print('\nTest SUCCEEDED')

if __name__ == "__main__":
    unittest.main()
