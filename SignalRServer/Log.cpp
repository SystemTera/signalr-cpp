#include "Log.h"
#include "Helper.h"

#include "fstream"

namespace P3 { namespace SignalR { namespace Server {

static Log m_log;

Log::Log()
{
    m_path = LOG_PATH;
}


Log* Log::GetInstance()
{
    return &m_log;
}

void Log::Write(const char* str)
{
    ofstream outfile;
    string line;

    line = Helper::getTimeStr() + ": " + string(str) +"\n";

    outfile.open(m_path.c_str(), ios_base::app);
    outfile << line.c_str();
    outfile.close();
}

void Log::SetLogFile(const char* path)
{
    m_path.assign(path);
}

}}}
