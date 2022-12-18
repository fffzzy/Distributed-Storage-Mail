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
    else if (header.substr(0, 12) == "mail/compose")
    {
        sendEmail();
    }
    else
    {
    }
}

void APIHandler::parseGet()
{
    if (header.substr(0, 4) == "mail")
    {
        getEmailList();
    }
    else
    {
    }
}

void APIHandler::parseDelete()
{
    if (header.substr(0, 6) == "logout")
    {
        logout();
    }
    else if (header.substr(0, 11) == "mail/delete")
    {
        deleteEmail();
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
        if (!password.empty() && real_pw == password)
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

void APIHandler::sendEmail()
{
    string username = checkCookie();

    string page;
    if (username.empty())
    {
        page = "HTTP/1.1 401 not logged in\r\n\r\n";
    }
    else
    {
        string name, host;
        if (is_verbose)
            cout << data.dump(4) << endl;

        parseEmail(data["sender"], name, host);
        if (name != username)
        {
            cout << "Sender in Email is different from current user!" << endl;
        }

        time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
        data["time"] = ctime(&t);

        cout << data << endl;

        for (const auto &recipient : data["recipients"])
        {
            parseEmail(recipient, name, host);

            if (host == "localhost")
            {
                cout << "To send a email to localhost soon" << endl;
                string mail_string = kvstore.Get(name, "mails");
                if (mail_string.empty())
                {
                    cout << name << " has no email in the mailbox currently!" << endl;
                    mail_string = "[]";
                }
                json mailList = json::parse(mail_string);

                if (mailList.empty())
                {
                    data["mailId"] = 1;
                }
                else
                {
                    int mailId = mailList[mailList.size() - 1]["mailId"];
                    data["mailId"] = mailId + 1;
                }

                mailList.push_back(data);
                kvstore.Put(name, "mails", mailList.dump());
            }
            else
            {
                MailService email;
                email.sendOut(data);
            }
        }

        page = "HTTP/1.1 200 success\r\n\r\n";
    }
    write(fd, page.c_str(), page.length());
}

void APIHandler::getEmailList()
{
    string username = checkCookie();

    string page;
    if (username.empty())
    {
        page = "HTTP/1.1 401 not logged in\r\n\r\n";
    }
    else
    {
        string mailList = kvstore.Get(username, "mails");
        if (mailList.empty())
        {
            mailList = "[]";
        }
        page = "HTTP/1.1 200 success\r\n\r\n" + mailList;
        cout << page << endl;
    }
    write(fd, page.c_str(), page.length());
}

void APIHandler::deleteEmail()
{
    string username = checkCookie();
    string page;
    if (username.empty())
    {
        page = "HTTP/1.1 401 not logged in\r\n\r\n";
    }
    else
    {
        string mail_matcher = "mailId=";
        size_t index = header.find(mail_matcher);
        size_t id_end = header.find(" ", index);

        bool matched = false;

        int mailId = stoi(header.substr(index + mail_matcher.size(), id_end - index - mail_matcher.size()));
        if (is_verbose)
            cout << "MailId detected in the header: " + mailId << endl;

        string mail_string = kvstore.Get(username, "mails");
        if (!mail_string.empty())
        {
            json mailList = json::parse(mail_string);
            // find email with that mailId in the email list
            for (auto it = mailList.begin(); it != mailList.end(); it++)
            {
                if ((*it)["mailId"] == mailId)
                {
                    mailList.erase(it);
                    matched = true;
                }
            }
            kvstore.Put(username, "mails", mailList.dump());
        }
        if (matched)
        {
            page = "HTTP/1.1 200 success\r\n\r\n";
        }
        else
        {
            page = "HTTP/1.1 404 not found\r\n\r\n";
        }
    }
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
        if (!real_cookie.empty() && real_cookie == cookie_value)
        {
            return username;
        }
        else
        {
            if (is_verbose)
                cout << "The cookie sent by browser doesn't exist in database." << endl;
            return "";
        }
    }
    else
    {
        if (is_verbose)
            cout << "No cookie attached in the header." << endl;
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

void APIHandler::parseEmail(string email, string &user, string &host)
{
    auto separator = email.find("@");
    user = email.substr(0, separator);
    host = email.substr(separator + 1);

    if (is_verbose)
    {
        cout << "Email: " + email + ", User: " + user + ", Host: " + host << endl;
    }
}