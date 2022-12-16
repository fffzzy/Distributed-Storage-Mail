#include "api_handler.hpp"

void APIHandler::parsePost()
{
    if (header.substr(0, 6) == "signup")
    {
        signup();
    }
    else if (header.substr(0, 5) == "login")
    {
        login();
    }
    else
    {
    }
}

void APIHandler::parseGet()
{
}

void APIHandler::parseDelete()
{
    if (header.substr(0, 6) == "logout")
    {
        logout();
    }
    else
    {
    }
}

void APIHandler::signup()
{
    if (!data.contains("username"))
    {
        cout << "No username found in the request body" << endl;
    }
    else
    {
        string username = data["username"];
        string password = data["password"];
        if (is_verbose)
        {
            cout << "username: " << username << endl;
            cout << "password: " << password << endl;
        }
        kvstore.Put(username, "password", password);
        string page = "HTTP/1.1 201 successfully created\r\n\r\n";
        if (is_verbose)
            cout << page << endl;
        write(fd, page.c_str(), page.length());
    }
}

void APIHandler::login()
{
    if (!data.contains("username"))
    {
        cout << "No username found in the request body" << endl;
    }
    else
    {
        string username = data["username"];
        string password = data["password"];
        if (is_verbose)
        {
            cout << "username: " << username << endl;
            cout << "password: " << password << endl;
        }
        string real_pw = kvstore.Get(username, "password");
        string page;
        if (real_pw == password)
        {
            time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
            string cookie = urlEncode(ctime(&t));
            kvstore.Put(username, "cookie", cookie);
            page = "HTTP/1.1 200 success\r\nSet-Cookie: " + username + "=" + cookie + "; path=/; expires=Thu, Jan 01 2023 00:00:00 UTC \r\n\r\n";
            if (is_verbose)
                cout << page << endl;
        }
        else
        {
            page = "HTTP/1.1 401 unauthorized if pwd incorrect\r\n\r\n";
            cout << "your pw: " + password << "\n"
                 << "real pw: " + real_pw << endl;
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
    string page = "HTTP/1.1 200 success\r\nSet-Cookie: " + username + "=deleted; path=/; expires=Thu, Jan 01 1970 00:00:00 UTC \r\n\r\n";
    write(fd, page.c_str(), page.length());
}

string APIHandler::checkCookie()
{
    string cookie_matcher = "Cookie: ";
    size_t cookie_start = header.find(cookie_matcher);
    if (cookie_start != string::npos)
    {
        cookie_start += cookie_matcher.size();
        size_t user_name_end_index = header.find("=", cookie_start);
        string username = header.substr(cookie_start, user_name_end_index - cookie_start);
        cout << "username in cookie:" << username << endl;
        size_t cookie_end = header.find("\r", cookie_start);
        string cookie_value = header.substr(user_name_end_index + 1, cookie_end - user_name_end_index - 1);
        string real_cookie = kvstore.Get(username, "cookie");
        if (is_verbose)
        {
            cout << "Cookie in browser: " << cookie_value << "length: " << cookie_value.size() << endl;
            cout << "Cookie in database: " << real_cookie << "length: " << real_cookie.size() << endl;
        }
        if (real_cookie == cookie_value)
        {
            return username;
        }
        else
        {
            if (is_verbose) cout << "The cookie sent by browser doesn't exist in database." << endl;
            return "";
        }
    }
    else
    {
        if (is_verbose) cout << "No cookie attached in the header." << endl;
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