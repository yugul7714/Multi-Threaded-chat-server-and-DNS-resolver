#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
using namespace std;
#define BUFFER_SIZE 1024
//maps for clients and group (names, admins and members ) 
unordered_map<int, string> clients;              
unordered_map<string, int> admins; 
unordered_map<string, string> users;       
unordered_map<string, unordered_set<int>> groups;
mutex m;

void loadUsers() 
{
    ifstream file("users.txt");
    if (!file) 
    {
        cerr << "Unable to open file: users.txt" << endl;
        exit(1);
    }
    string line;
    while (getline(file, line)) 
    {
        istringstream iss(line);
        string username, password;
        if (getline(iss, username, ':') && getline(iss, password)) 
        {
            users[username] = password;
        }
    }
    file.close();
}
void privateMessage(const string &username, const string &message, int sender_socket) 
{
    lock_guard<mutex> lock(m);
    if (clients.find(sender_socket) == clients.end()) return; // Check if sender exists
    string sender_username = clients[sender_socket];
    for (const auto &client : clients) 
    {
        if (client.second == username) 
        {
            string full_message = sender_username + ": " + message + "\n";
            send(client.first, full_message.c_str(), full_message.size(), 0);
            return;
        }
    }
    string error = "User " + username + " not found\n";
    send(sender_socket, error.c_str(), error.size(), 0);
}
void broadcastMessage(const string &message, int sender_socket)
{
    lock_guard<mutex> lock(m);
    string sender_username = clients[sender_socket];
    string full_message = sender_username +": "+ message + "\n";
    for (const auto &client : clients)
    {
        if (client.first != sender_socket)
        {
            send(client.first, full_message.c_str(), full_message.size(), 0);
        }
    }
}
void groupMessage(const string &group_name, const string &message, int sender_socket)
{
    lock_guard<mutex> lock(m);
    // check if the group exists
    if (groups.find(group_name) == groups.end()) 
    {
        string error = "Group "+group_name+" does not exist\n";
        send(sender_socket, error.c_str(), error.size(), 0);
        return;
    }
    // check if the sender is part of the group
    if (groups[group_name].find(sender_socket) == groups[group_name].end()) 
    {
        string error = "You are not a member of group " + group_name + "\n";
        send(sender_socket, error.c_str(), error.size(), 0);
        return;
    }
    string sender_username = clients[sender_socket];
    string full_message = "[" + group_name + "] " + sender_username + ": " + message + "\n";
    for (int client_skt : groups[group_name]) 
    {
        if (client_skt != sender_socket) 
        {
            send(client_skt, full_message.c_str(), full_message.size(), 0);
        }
    }
}
void clientCommands(int client_skt) 
{
    char buffer[BUFFER_SIZE];
    // user authentication
    send(client_skt, "Enter username: ", 17, 0);
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_skt, buffer, BUFFER_SIZE, 0);
    string username(buffer);
    send(client_skt, "Enter password: ", 17, 0);
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_skt, buffer, BUFFER_SIZE, 0);
    string password(buffer);
    {
        lock_guard<mutex> lock(m);
        if (users.find(username) == users.end() || users[username] != password) 
        {
            send(client_skt, "Authentication failed\n", 23, 0);
            close(client_skt);
            return;
        }
        clients[client_skt] = username;
    }
    string welcome_message = "Welcome to the chat server!\n";
    send(client_skt, welcome_message.c_str(), welcome_message.size(), 0);
    cout << username << " connected.\n";
    // Notify others about the new connection
    broadcastMessage(" joined the chat.\n", client_skt);
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_skt, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            // Notify others about the disconnection
            {
                lock_guard<mutex> lock(m);
                string disconnect_message = username + " has left the chat\n";
                clients.erase(client_skt);
                broadcastMessage(disconnect_message, client_skt);
            }
            close(client_skt);
            break;
        }
        string message(buffer);
        if (message.rfind("/broadcast ", 0) == 0) 
        {
            broadcastMessage(message.substr(11), client_skt);
        }  
        else if (message.rfind("/create_group ", 0) == 0) 
        {
            string group_name = message.substr(14);
            {
                lock_guard<mutex> lock(m);
                if (groups.find(group_name) != groups.end()) 
                {
                    send(client_skt, ("Group " + group_name + " already exists\n").c_str(), group_name.size() + 20, 0);
                } 
                else 
                {
                    groups[group_name].insert(client_skt);
                    admins[group_name] = client_skt;
                    send(client_skt, ("Group " + group_name + " created.\n").c_str(), group_name.size() + 15, 0);
                    string broadcastMessage = "A new group '" + group_name + "' has been created by "+clients[client_skt]+"\n";
                    cout << "Group '" << group_name << "' created by " << username << "\n";
                    for (const auto &client : clients)
                    {
                        if (client.first != client_skt) 
                        {
                            send(client.first, broadcastMessage.c_str(), broadcastMessage.size(), 0);
                        }
                    }
                }
            }
        }
        else if (message.rfind("/msg ", 0) == 0) 
        {
            size_t space = message.find(' ', 5);
            if (space != string::npos) 
            {
                string target_user = message.substr(5, space - 5);
                string private_msg = message.substr(space + 1);
                privateMessage(target_user, private_msg, client_skt);
            }
        }
        else if (message.rfind("/remove_member ", 0) == 0) 
        {
            istringstream iss(message.substr(14)); 
            string group_name, target_user;
            iss >> group_name >> target_user;
            if (group_name.empty() || target_user.empty()) 
            {
                send(client_skt, "Invalid command format. Use: /remove_member <group_name> <username>\n", 68, 0);
                return;
            }
            lock_guard<mutex> lock(m);
            // Check if the group exists
            if (groups.find(group_name) == groups.end()) 
            {
                cout << "Group not found: " << group_name << endl;
                send(client_skt, ("Group " + group_name + " does not exist\n").c_str(), group_name.size() + 22, 0);
                return;
            }
            // Check if the client is the admin of the group
            if (admins[group_name] != client_skt) 
            {
                send(client_skt, "Only the group creator can remove members\n", 44, 0);
                return;
            }
            // Remove the member
            //auto target = find_if(clients.begin(), clients.end(),
            auto target = std::ranges::find_if(clients.begin(), clients.end(),
                                      [&](const pair<const int, string> &pair) { return pair.second == target_user; });
            if (target == clients.end() || groups[group_name].find(target->first) == groups[group_name].end()) {
                send(client_skt, ("User " + target_user + " is not in group " + group_name + "\n").c_str(),
                    target_user.size() + group_name.size() + 25, 0);
                return;
            }
            groups[group_name].erase(target->first);
            send(target->first, ("You have been removed from group " + group_name + ".\n").c_str(),group_name.size() + 36, 0);
            // Notify the group
            string notification = target_user + " has been removed from group " + group_name + ".\n";
            for (int member : groups[group_name]) 
            {
                send(member, notification.c_str(), notification.size(), 0);
            }
        }
        else if (message.rfind("/join_group ", 0) == 0) 
        {
            string group_name = message.substr(12);
            {
                lock_guard<mutex> lock(m);
                if (groups.find(group_name) != groups.end()) 
                {
                    groups[group_name].insert(client_skt);
                    send(client_skt, ("Joined group " + group_name + ".\n").c_str(), group_name.size() + 15, 0);
                    // Notify all group members about the new member
                    string join_message = clients[client_skt] + " has joined the group " + group_name + "\n";
                    for (int member : groups[group_name]) {
                        if (member != client_skt) { // Exclude the joining client
                            send(member, join_message.c_str(), join_message.size(), 0);
                        }
                    }
                } 
                else 
                {
                    send(client_skt, ("Group " + group_name + " does not exist\n").c_str(), group_name.size() + 22, 0);
                }
            }
        }
        else if (message.rfind("/group_msg ", 0) == 0) 
        {
            size_t space = message.find(' ', 11);
            if (space != string::npos) {
                string group_name = message.substr(11, space - 11);
                string group_msg = message.substr(space + 1);
                groupMessage(group_name, group_msg, client_skt);
            }
        } 
        else if (message.rfind("/leave_group ", 0) == 0) 
        {
            string group_name = message.substr(13);
            {
                lock_guard<mutex> lock(m);
                // Check if the group exists and the client is part of it
                if (groups.find(group_name) == groups.end() || groups[group_name].find(client_skt) == groups[group_name].end()) 
                {
                    send(client_skt, ("You are not in group " + group_name + ".\n").c_str(), group_name.size() + 22, 0);
                    return;
                }
                // Capture the members of the group before modifying it
                unordered_set<int> group_members = groups[group_name];
                // Remove the client from the group
                groups[group_name].erase(client_skt);
                // Notify the client
                send(client_skt, ("Left group " + group_name + "\n").c_str(), group_name.size() + 13, 0);
                if (admins[group_name] == client_skt) 
                {
                    // If the creator is leaving, delete the group
                    groups.erase(group_name);
                    admins.erase(group_name);

                    // Notify all remaining members that the group is deleted
                    string delete_message = "The group " + group_name + " has been deleted by its creator\n";
                    cout << "Group '" << group_name << "' deleted by its creator (" << username << ") \n";
                    for (int member : group_members)
                    {
                        if (member != client_skt) 
                        { // Exclude the leaving creator
                            send(member, delete_message.c_str(), delete_message.size(), 0);
                        }
                    }
                } 
                else 
                {
                    // Notify remaining members that someone has left
                    string leave_message = clients[client_skt] + " has left the group " + group_name + "\n";
                    for (int member : groups[group_name]) {
                        send(member, leave_message.c_str(), leave_message.size(), 0);
                    }
                }
            }
        }
        else if (message == "/exit")
        {
            {
                lock_guard<mutex> lock(m);
                if (clients.find(client_skt) == clients.end())
                {
                    string disconnect_message = " has left the chat\n";
                    broadcastMessage(disconnect_message, client_skt);
                    // Log the exit message on the server
                    cout << clients[client_skt] << disconnect_message;
                }
            }
            clients.erase(client_skt);
            close(client_skt);
            break;
        }


        else 
        {
            send(client_skt, "Invalid command\n", 18, 0);
        }
    }
}

int main() 
{
    loadUsers(); // Load users from users.txt
    int server_skt = socket(AF_INET, SOCK_STREAM, 0);
    if (server_skt < 0) 
    {
        cerr << "Error creating socket\n";
        return 1;
    }
    int opt = 1;
    if (setsockopt(server_skt, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
    {
        cerr << "Error setting socket options\n";
        return 1;
    }
    int port=12345;
    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;
    if (::bind(server_skt, (sockaddr*)&server_address, sizeof(server_address)) < 0)
    {
        cerr << "Error binding socket\n";
        return 1;
    }
    if (listen(server_skt, 10) < 0) 
    {
        cerr << "Error listening on socket\n";
        return 1;
    }
    cout << "Server started on port "<< port <<"\n";
    while (true) 
    {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_skt = accept(server_skt, (sockaddr*)&client_addr, &client_len);
        if (client_skt < 0) {
            cerr << "Error accepting connection\n";
            continue;
        }
        thread client_thread(clientCommands, client_skt);
        client_thread.detach();
    }

    close(server_skt);
    return 0;
}
