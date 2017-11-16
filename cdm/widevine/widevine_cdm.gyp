# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    '../../../build/util/version.gypi',
  ],

  # Always provide a target, so we can put the logic about whether there's
  # anything to be done in this file (instead of a higher-level .gyp file).
  'targets': [
    {
      # GN version: //third_party/widevine/cdm:widevinecdmadapter
      'target_name': 'widevinecdmadapter',
      'type': 'none',
      'conditions': [
        [ 'os_posix == 1 and OS != "mac"' and 'enable_widevine == 1 and enable_pepper_cdms == 1', {
          'dependencies': [
            '../../../ppapi/ppapi.gyp:ppapi_cpp',
            '../../../media/media_cdm_adapter.gyp:cdmadapter',
            '../../../third_party/widevine/cdm/widevine_cdm.gyp:widevine_cdm_version_h',
            '../../../third_party/widevine/cdm/widevine_cdm.gyp:widevinecdmadapter_resources',
          ],
          'sources': [
            '<(SHARED_INTERMEDIATE_DIR)/widevinecdmadapter_version.rc',
          ],
          'conditions': [
            [ 'os_posix == 1 and OS != "mac"', {
              'libraries': [
                '-lrt',
                '-lwidevinecdm'
              ],
            }],
          ],
        }],
      ],
    },
    {
      # GN version: //third_party/widevine/cdm:version_h
      'target_name': 'widevine_cdm_version_h',
      'type': 'none',
      'copies': [{
        'destination': '<(SHARED_INTERMEDIATE_DIR)',
        'files': [ '../../../third_party/widevine/cdm/stub/widevine_cdm_version.h' ],
      }],
    },
  ],
}
