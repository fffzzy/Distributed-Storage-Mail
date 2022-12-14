#include "api_handler.hpp"

void APIHandler::parsePost()
{
    if (buffer.substr(0, 6) == "signup")
    {
        signup();
    }
    else if (buffer.substr(0, 5) == "login")
    {
        login();
    }
    else if (buffer.substr(0, 6) == "logout")
    {
        logout();
    }
    else
    {
    }
}

void APIHandler::parseGet()
{
    if (buffer.substr(0, 6) == "signup")
    {
        signup();
    }
    else if (buffer.substr(0, 5) == "login")
    {
        login();
    }
    else if (buffer.substr(0, 6) == "logout")
    {
        logout();
    }
    else
    {
    }
}

void APIHandler::parseDelete()
{
    if (buffer.substr(0, 6) == "signup")
    {
        signup();
    }
    else if (buffer.substr(0, 5) == "login")
    {
        login();
    }
    else if (buffer.substr(0, 6) == "logout")
    {
        logout();
    }
    else
    {
    }
}

void APIHandler::signup()
{
    auto index = buffer.find("username=");
    if (index == -1)
    {
        cout << "No username found in the request body" << endl;
    }
    else
    {
        auto body = buffer.substr(index);
        size_t i = body.find("=");
        size_t j = body.find("&");
        string username = body.substr(i + 1, j - i - 1);
        i = body.find("=", i + 1);
        j = body.find(" ");
        string password = body.substr(i + 1, j - i - 1);
        if (is_verbose)
        {
            cout << "username: " << username << endl;
            cout << "password: " << password << endl;
        }
        kvstore.Put(username, "password", password);
        string page = "HTTP/1.1 201 successfully created\r\n\r\n";
        write(fd, page.c_str(), page.length());
    }
}

void APIHandler::login()
{
    auto index = buffer.find("username=");
    if (index == -1)
    {
        cout << "No username found in the request body" << endl;
    }
    else
    {
        auto body = buffer.substr(index);
        size_t i = body.find("=");
        size_t j = body.find("&");
        string username = body.substr(i + 1, j - i - 1);
        i = body.find("=", i + 1);
        j = body.find(" ");
        string password = body.substr(i + 1, j - i - 1);
        if (is_verbose)
        {
            cout << "username: " << username << endl;
            cout << "password: " << password << endl;
        }
        string real_pw = kvstore.Get(username, "password");
        string page;
        if (real_pw == password)
        {
            string cookie = username + "=" + urlEncode(username);
            kvstore.Put(username, "cookie", cookie);
            page = "HTTP/1.1 200 success Set-Cookie: " + cookie + "\r\n\r\n";
        }
        else
        {
            page = "HTTP/1.1 401 unauthorized if pwd incorrect\r\n\r\n";
        }
        write(fd, page.c_str(), page.length());
    }
}

void APIHandler::logout()
{
    string username = checkCookie();
    if (!username.empty())
    {
        kvstore.Delete(username, "cookie");
    }
    string page = "HTTP/1.1 200 success\r\n\r\n";
    write(fd, page.c_str(), page.length());
}

string APIHandler::checkCookie()
{
    string cookie_matcher = "Cookie: ";
    size_t cookie_start = buffer.find(cookie_matcher);
    if (cookie_start != string::npos)
    {
        cookie_start += cookie_matcher.size();
        size_t user_name_end_index = buffer.find("=", cookie_start);
        string username = buffer.substr(cookie_start, user_name_end_index - cookie_start);

        size_t cookie_end = buffer.find("\r", cookie_start);
        string cookie_value = buffer.substr(user_name_end_index + 1, cookie_end - cookie_start - 1);
        string real_cookie = kvstore.Get(username, "cookie");
        if (real_cookie == cookie_value)
        {
            return username;
        }
        else
        {
            return "";
        }
    }
    else
    {
        return "";
    }
}

string urlEncode(string str)
{
    string new_str = "";
    char c;
    int ic;
    const char *chars = str.c_str();
    char bufHex[10];
    int len = strlen(chars);

    for (int i = 0; i < len; i++)
    {
        c = chars[i];
        ic = c;
        // uncomment this if you want to encode spaces with +
        /*if (c==' ') new_str += '+';
        else */
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            new_str += c;
        else
        {
            sprintf(bufHex, "%X", c);
            if (ic < 16)
                new_str += "%0";
            else
                new_str += "%";
            new_str += bufHex;
        }
    }
    return new_str;
}