#include "Request.h"
#include "Helper.h"


namespace P3 { namespace SignalR { namespace Server {

Request::Request(string queryStr, string body, string version, string method, string uri, string user, string password)
    : _queryString(queryStr), _body(body), _version(version), _method(method), _uri(uri), _user(user), _password(password)
{

}
string Request::password() const
{
    return _password;
}


string Request::user() const
{
    return _user;
}

string Request::uri() const
{
    return _uri;
}

string Request::method() const
{
    return _method;
}

string Request::version() const
{
    return _version;
}

string Request::body() const
{
    return _body;
}

string Request::queryString() const
{
    return _queryString;
}


string Request::getParameter(const char* name)
{
    return Helper::decode(Helper::getQueryStringParam(name, _queryString.c_str()).c_str());
}

}}}
