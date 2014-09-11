/*
 * Copyright (c) 2014, p3root - Patrik Pfaffenbauer (patrik.pfaffenbauer@p3.co.at) and
 * Norbert Kleininger
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef HELPER_H
#define HELPER_H

#include <string.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <Json.h>
#include <Variant.h>

#include "Request.h"

#define MAX_GUID_CHARS  36

#define P3_UNUSED(x) (void)x;

using namespace std;
namespace P3 { namespace SignalR { namespace Server {

class Helper {

public:
    static string tail(string const& source, size_t const length);
    static bool endWith(string const& source, string const& checkval);

    static bool replace(string& str, const string& from, const string& to);
    static string createGUID();
    static string extractConnectionIdFromToken(const char* connectionToken);
    static string getQueryStringParam(const char* param, const char* query);
    static string getTimeStr();
    static string decode(const char* str);

    static string getHttpParam(const char* param, const char* req);
    static string getStrByIndex(int i,const char* req);
    static string getLeftOfSeparator(const char* str, const char sep);
    static string getRightOfSeparator(const char* str, const char sep);
    static string base64_encode(unsigned char const* , unsigned int len);
    static string base64_decode(string const& s);
    static string getBasicUser(const char* auth);
    static string getBasicPassword(const char* auth);

};
}}}
#endif // HELPER_H
