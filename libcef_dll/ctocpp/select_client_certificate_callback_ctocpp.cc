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

#include "libcef_dll/ctocpp/select_client_certificate_callback_ctocpp.h"


// VIRTUAL METHODS - Body may be edited by hand.

void CefSelectClientCertificateCallbackCToCpp::Run(CefString cert_path,
    CefString key_path)
{
  cef_select_client_certificate_callback_t* _struct = GetStruct();
  if (CEF_MEMBER_MISSING(_struct, run))
    return;

  _struct->run(
    _struct,
    cert_path.GetStruct(),
    key_path.GetStruct());
}

// CONSTRUCTOR - Do not edit by hand.

CefSelectClientCertificateCallbackCToCpp::CefSelectClientCertificateCallbackCToCpp(
    ) {
}

template<> cef_select_client_certificate_callback_t* CefCToCpp<CefSelectClientCertificateCallbackCToCpp,
    CefSelectClientCertificateCallback,
    cef_select_client_certificate_callback_t>::UnwrapDerived(
    CefWrapperType type, CefSelectClientCertificateCallback* c) {
  NOTREACHED() << "Unexpected class type: " << type;
  return NULL;
}

#ifndef NDEBUG
template<> base::AtomicRefCount CefCToCpp<CefSelectClientCertificateCallbackCToCpp,
    CefSelectClientCertificateCallback,
    cef_select_client_certificate_callback_t>::DebugObjCt = 0;
#endif

template<> CefWrapperType CefCToCpp<CefSelectClientCertificateCallbackCToCpp,
    CefSelectClientCertificateCallback,
    cef_select_client_certificate_callback_t>::kWrapperType =
    WT_SELECT_CLIENT_CERTIFICATE_CALLBACK;
