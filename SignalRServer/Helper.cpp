#include "Helper.h"
#include <iostream>
#include <uuid/uuid.h>

namespace P3 { namespace SignalR { namespace Server {

string Helper::tail(string const& source, size_t const length)
{
  if (length >= source.size()) { return source; }
  return source.substr(source.size() - length);
}


bool Helper::endWith(string const& source, string const& checkval)
{
    string e = tail(source,checkval.length());
    return (checkval.compare(e)==0);
}

bool Helper::replace(string& str, const string& from, const string& to)
{
    size_t start_pos = str.find(from);
    if(start_pos == string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

string Helper::createGUID()
{
   uuid_t uuid;
   uuid_generate(uuid);
   char chr[16];
   bzero(chr, 16);
   uuid_unparse(uuid, chr);
   string ret;
   ret.assign(chr);
   return ret;
}

string Helper::extractConnectionIdFromToken(const char* connectionToken)
{
    string ret="";
    ret.assign(connectionToken);
    ret = ret.substr(0,MAX_GUID_CHARS);
    return ret;
}


string Helper::getQueryStringParam(const char* param, const char* query)
{
    string val="";
    int paramlen, toklen;
    char *pos;
    char q[1024];

    strcpy(q,query);

    paramlen = strlen(param);

    char* token = strtok (q,"&");
    while (token != NULL)
    {
        // Parameter found ?
        if (memcmp(token, param, paramlen)==0)
        {
            pos = strstr(token,"=");
            if (pos)
            {
                pos++;
                toklen = strlen(token);
                val.assign(pos, toklen-(pos - token));
                break;
            }
        }
        token = strtok (NULL, "&");
    }
    return val;
}


string Helper::getTimeStr()
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,80,"%d-%m-%Y %I:%M:%S",timeinfo);
    string str(buffer);
    return str;
}


string Helper::decode(const char* str)
{
    string ret="";
    char hex[3] = { 0,0,0 };
    char c;

    for (char* ptr=(char*)str;*ptr!=0;ptr++)
    {
        if (*ptr=='%')
        {
            hex[0]=*(++ptr);
            hex[1]=*(++ptr);
            c = (char)strtol(hex, NULL, 16);
            ret+=c;
        }
        else ret+=*ptr;
    }
    return ret;
}


string Helper::getHttpParam(const char* param, const char* req)
{
    string ret="";

    const char* ptr = strstr(req,param);
    if (ptr)
    {
        ptr+=strlen(param);
        const char* eol=strstr(ptr,"\r");
        if (strlen(param))
        {
            ret.assign(ptr,eol-ptr);
            return ret;
        }
    }

    return ret;
}


string Helper::getStrByIndex(int i,const char* req)
{
    char word[1024];
    int index=0;
    char *w=word;
    string ret="";
    for (const char* ptr = req;*ptr!='\r'&&*ptr!='\0';ptr++)
    {
        if (*ptr==' ')
        {
            if (index==i)
            {
                ret.assign(word,(w-word)) ;
                return ret;
            }
            index++;
            w=word;
        }
        else *w++ = *ptr;
    }

    if (index==i)
        ret.assign(word,(w-word)) ;

    return ret;
}


string Helper::getLeftOfSeparator(const char* str, const char sep)
{
    string ret="";
    const char* ptr = strchr(str, sep);
    if (ptr)
        ret.assign(str,(ptr-str));
    else
        ret.assign(str);
    return ret;
}


string Helper::getRightOfSeparator(const char* str, const char sep)
{
    string ret="";
    const char* ptr = strchr(str, sep);
    if (ptr)
    {
        ptr++; // skip separator
        ret.assign(ptr, strlen(str)-(ptr-str));
    }
    return ret;
}


static const string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

string Helper::base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
      ret += '=';

  }

  return ret;

}

string Helper::base64_decode(string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}


string Helper::getBasicUser(const char* auth)
{
    // Authorization: Basic d2lraTpwZWRpYQ==
    string authtype = getStrByIndex(0,auth);
    if (authtype.compare("Basic")==0)
    {
        string token = getStrByIndex(1,auth);
        string usernamepwd = base64_decode(token.c_str()); // e.g. "wiki:pedia"
        return getLeftOfSeparator(usernamepwd.c_str(),':');
    }
    return "";
}

string Helper::getBasicPassword(const char* auth)
{
    // Authorization: Basic d2lraTpwZWRpYQ==
    string authtype = getStrByIndex(0,auth);
    if (authtype.compare("Basic")==0)
    {
        string token = getStrByIndex(1,auth);
        string usernamepwd = base64_decode(token.c_str()); // e.g. "wiki:pedia"
        return getRightOfSeparator(usernamepwd.c_str(),':');
    }
    return "";
}

}}}
