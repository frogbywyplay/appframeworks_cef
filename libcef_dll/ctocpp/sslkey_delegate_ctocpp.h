// Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool. If making changes by
// hand only do so within the body of existing method and function
// implementations. See the translator.README.txt file in the tools directory
// for more information.
//

#ifndef CEF_LIBCEF_DLL_CTOCPP_SSLKEY_DELEGATE_CTOCPP_H_
#define CEF_LIBCEF_DLL_CTOCPP_SSLKEY_DELEGATE_CTOCPP_H_
#pragma once

#ifndef BUILDING_CEF_SHARED
#pragma message("Warning: "__FILE__" may be accessed DLL-side only")
#else  // BUILDING_CEF_SHARED

#include "include/cef_ssl_key_delegate.h"
#include "include/capi/cef_ssl_key_delegate_capi.h"
#include "libcef_dll/ctocpp/ctocpp.h"

// Wrap a C structure with a C++ class.
// This class may be instantiated and accessed DLL-side only.
class CefSSLKeyDelegateCToCpp
    : public CefCToCpp<CefSSLKeyDelegateCToCpp, CefSSLKeyDelegate,
        cef_sslkey_delegate_t> {
 public:
  CefSSLKeyDelegateCToCpp();

  // CefSSLKeyDelegate methods.
  void GetDigestPreferences(cef_hash_type_t **supported, int *count) override;
  size_t GetMaxSignatureLengthInBytes() override;
  int SignDigest(cef_key_type_t key_type, cef_hash_type_t hash_type,
      const uint8* digest, const uint32 digest_len, uint8 *sig,
      uint32 *sig_len) override;
};

#endif  // BUILDING_CEF_SHARED
#endif  // CEF_LIBCEF_DLL_CTOCPP_SSLKEY_DELEGATE_CTOCPP_H_