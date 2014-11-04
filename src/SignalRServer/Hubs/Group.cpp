#include "Group.h"
#include "Helper.h"

namespace P3 { namespace SignalR { namespace Server {

Group::Group()
{
    _connectionId = "";
    _groupName = "";
}

Group::Group(const char* connectionId,const char* groupName)
{
    _connectionId = connectionId;
    _groupName = groupName;
}

Group::~Group()
{

}

std::string Group::removePrefix()
{
    if (_groupName.find('.') != std::string::npos)
        return Helper::getRightOfSeparator(_groupName.c_str(), ".");
    return _groupName;
}

}}}
