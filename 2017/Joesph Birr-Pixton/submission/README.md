The design of `EVP_VerifyFinal`
===============================

OpenSSL's `EVP_VerifyFinal` function has a poor choice of return value
semantics, which means naive callers can accidentally treat invalid
signatures as valid.

There is indeed such vulnerable code scattered around the internet.

The semantics
-------------
The relevent part of `EVP_VerifyFinal` inherits the return values of
`EVP_PKEY_verify`.  The documentation says:

```
  EVP_PKEY_verify_init() and EVP_PKEY_verify() return 1 if the verification was
  successful and 0 if it failed. Unlike other functions the return value 0 from
  EVP_PKEY_verify() only indicates that the signature did not not verify
  successfully (that is tbs did not match the original data or the signature was
  of invalid form) it is not an indication of a more serious error.

  A negative value indicates an error other that signature verification failure.
  In particular a return value of -2 indicates the operation is not supported by
  the public key algorithm.
```

In C, any non-zero integer is 'truthy' while only zero integers are
'falsy'.  This means a naïve caller can achieve a working implementation
along the following lines:

```
   if (EVP_VerifyFinal(ctx, sig, siglen, pubkey)) {
     /* signature valid */
   } else {
     /* signature invalid */
   }
```

However, this code is incorrect if `EVP_VerifyFinal` fails with an
'error other that signature verification failure' (sic).  Such errors
include things like memory allocation failures; so an attacker able
to cause memory pressure can bypass signature checks in such callers.

Example outcome: Bitcoin Core
-----------------------------
Bitcoin core is a fairly typical example of OpenSSL callers[1].  It
contains this code:

```
  EVP_PKEY *pubkey = X509_get_pubkey(signing_cert);
  EVP_MD_CTX_init(ctx);
  if (!EVP_VerifyInit_ex(ctx, digestAlgorithm, NULL) ||
      !EVP_VerifyUpdate(ctx, data_to_verify.data(), data_to_verify.size()) ||
      !EVP_VerifyFinal(ctx, (const unsigned char*)paymentRequest.signature().data(), (unsigned int)paymentRequest.signature().size(), pubkey)) {
      throw SSLVerifyError("Bad signature, invalid payment request.");
  }
```

[1] https://github.com/bitcoin/bitcoin/blob/70145064153aae87245c35e009282e5198e3f60f/src/qt/paymentrequestplus.cpp

BoringSSL
---------
BoringSSL unified its return code semantics, and in doing so addressed
this problem.

An amusing side effect of this change is BoringSSL itself contains
code which calls this function in a truthy way.  This is obviously
safe, but might lead astray OpenSSL users looking for code to copy[2].

[2] http://sources.debian.net/src/qtwebengine-opensource-src/5.7.1%2Bdfsg-6.1/src/3rdparty/chromium/third_party/boringssl/src/ssl/s3_srvr.c/?hl=2074#L2074

LibreSSL
--------
LibreSSL has not addressed this problem.

Related work
------------
- CVE-2008-5077 is a collection of related vulnerabilities within OpenSSL itself
  resulting from this design error.
