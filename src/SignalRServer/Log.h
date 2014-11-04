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

#ifndef LOG_H
#define LOG_H


#include <string>

using namespace std;
namespace P3 { namespace SignalR { namespace Server {


#define LOG_PATH ".\\SignalRServer.log"

#define LOGLEVEL_ERROR  1 // Logs errors
#define LOGLEVEL_WARN   2 // Logs warnings, infos
#define LOGLEVEL_INFO   3 // Logs errors, warnings, infos
#define LOGLEVEL_DEBUG  4 // Logs errors, warnings, infos, debug
#define LOGLEVEL_TRACE  5 // Logs errors, warnings, infos, debug, trace

typedef void (*LogCallback)(const char* msg, int level, void* data);

class Log
{
public:
    Log();

public:
    static Log* GetInstance();

public:
    void Write(const char* str,int level=LOGLEVEL_INFO);
    void SetLogFile(const char* path);
    void SetEnabled(bool enabled=true);
    void SetLogLevel(int level=LOGLEVEL_INFO);
    void SetCallback(LogCallback cb, void* data=NULL);
    void SetUseFileLog(bool fl=true);

private:
    string m_path;
    bool m_enabled;
    int m_loglevel;
    LogCallback m_callback;
    void* m_cbData;
    bool m_useFileLog;
};

}}}
#endif // LOG_H
