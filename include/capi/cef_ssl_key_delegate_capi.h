// Copyright (c) 2016 Marshall A. Greenblatt. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the name Chromium Embedded
// Framework nor the names of its contributors may be used to endorse
// or promote products derived from this software without specific prior
// written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool and should not edited
// by hand. See the translator.README.txt file in the tools directory for
// more information.
//

#ifndef CEF_INCLUDE_CAPI_CEF_SSL_KEY_DELEGATE_CAPI_H_
#define CEF_INCLUDE_CAPI_CEF_SSL_KEY_DELEGATE_CAPI_H_
#pragma once

#include "include/capi/cef_base_capi.h"

#ifdef __cplusplus
extern "C" {
#endif


///
// Structure used to delegate to the client SSL key manipulation
///
typedef struct _cef_sslkey_delegate_t {
  ///
  // Base structure.
  ///
  cef_base_t base;

  ///
  // Return the hash types supported by the client
  ///
  void (CEF_CALLBACK *get_digest_preferences)(
      struct _cef_sslkey_delegate_t* self, cef_hash_type_t **supported,
      int *count);

  ///
  // Return the maximum length of the signature in bytes
  ///
  size_t (CEF_CALLBACK *get_max_signature_length_in_bytes)(
      struct _cef_sslkey_delegate_t* self);

  ///
  // Used by the library to sign a digest
  ///
  int (CEF_CALLBACK *sign_digest)(struct _cef_sslkey_delegate_t* self,
      cef_key_type_t key_type, cef_hash_type_t hash_type, const uint8* digest,
      const uint32 digest_len, uint8 *sig, uint32 *sig_len);
} cef_sslkey_delegate_t;


#ifdef __cplusplus
}
#endif

#endif  // CEF_INCLUDE_CAPI_CEF_SSL_KEY_DELEGATE_CAPI_H_