# Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

{
  'variables': {
    'disable_nacl': 0,
    'proprietary_codecs': 0,
    'v8_use_snapshot': 'true',
    'linux_use_tcmalloc': 1,
    'linux_link_kerberos': 0,
    'use_kerberos': 0,
    'use_gconf' : 0,
    'use_aura' : 1,
    'use_ozone' : 1,
    'embedded' : 1,
    'enable_webrtc' : 0,
    'use_pulseaudio' :0,
    'use_dbus' :0,
    'ozone_platform_gbm' :1,
    'ozone_platform_test' :1,
    'include_tests' :0,
    'toolkit_views' :0,
    'use_cups' :0,
    'use_gnome_keyring': 0,
    'use_openssl' : 1,
  },
}
