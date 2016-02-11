#include "Log.h"
#include "Helper.h"

#include "fstream"

namespace P3 { namespace SignalR { namespace Server {


static Log m_log;

Log::Log()
{
    m_path = LOG_PATH;
    m_enabled = true;
    m_loglevel = LOGLEVEL_WARN;
    m_callback = NULL;
    m_cbData = NULL;
    m_useFileLog = false;
}


Log* Log::GetInstance()
{
    return &m_log;
}

void Log::Write(const char* str, int level)
{
    if (!m_enabled)
        return;

    if (level>m_loglevel)
        return;

    if(m_useFileLog)
    {

        ofstream outfile;
        string line;

        line = Helper::getTimeStr() + ": " + string(str) +"\n";

        outfile.open(m_path.c_str(), ios_base::app);
        outfile << line.c_str();
        outfile.close();
    }
    // Also cal the callback, if defined
    if (m_callback)
        m_callback(str,level,m_cbData);
}

void Log::SetLogFile(const char* path)
{
    m_path.assign(path);
}

void Log::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

void Log::SetLogLevel(int level)
{
    m_loglevel = level;
}

void Log::SetCallback(LogCallback cb, void* data)
{
    m_callback = cb;
    m_cbData = data;
}

void Log::SetUseFileLog(bool fl)
{
    m_useFileLog = fl;
}


}}}
