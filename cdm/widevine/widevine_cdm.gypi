{
  'conditions': [
    ['enable_pepper_cdms==1 and enable_widevine==1', {
      'targets': [
        {
          'target_name': 'widevinecdmadapter',
          'type': 'none',
          # Check whether the plugin's origin URL is valid.
          'defines': ['CHECK_DOCUMENT_URL'],
          'dependencies': [
            '<(DEPTH)/ppapi/ppapi.gyp:ppapi_cpp',
            '<(DEPTH)/media/media_cdm_adapter.gyp:cdmadapter'
          ],
          'conditions': [
            ['os_posix == 1 and OS != "mac"', {
              'libraries': [
                '<!(echo $DESTDIR)usr/lib/libwidevinecdm.so',
              ],
            }],
          ],
        },
      ],
    }],
  ],
}
