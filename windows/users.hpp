#ifndef W_USERS
#define W_USERS

#include <iostream>
#include <string>
#include <fstream>
#include <sqlite3.h>
#include <fstream>

#include <cpp_redis/cpp_redis>

#include "../include/rapidjson/document.h"
#include "../include/rapidjson/writer.h"
#include "../include/rapidjson/stringbuffer.h"

#include "imgui.h"
#include "types.h"

class Window_ManagerUsers {
private:
  WindowState W_STATE;
  RenderRegistry *windows;

  char input_username[256]        = "Username";
  char input_password[256]        = "Password";
  char input_repo[256]            = "Repository";

  rapidjson::Document document;
  cpp_redis::client client;

public:
  Window_ManagerUsers(RenderRegistry* w){
    windows = w;

    client.connect("127.0.0.1", 6379,
       [](const std::string &host, std::size_t port, cpp_redis::connect_state status) {
           if (status == cpp_redis::connect_state::dropped) {
               std::cout << "client disconnected from " << host << ":" << port << std::endl;
           } 
       }); 
  }

  ~Window_ManagerUsers(){
  }

  WindowState wstate(){
    return W_STATE;
  }

  void clear(){
  }

  void render(){
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_MenuBar;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(438, 167));
    ImGui::Begin("Users", NULL, window_flags);

    if (ImGui::BeginMenuBar()){
      if(W_STATE == "main"){
        if (ImGui::BeginMenu("Options")){
            if (ImGui::MenuItem("Close")) windows->hide("users");
            ImGui::EndMenu();
        }
      } 

      ImGui::EndMenuBar();
    }

    if(W_STATE == "main"){
      w_main();
    } else {
      //w_invalid();
    }

    ImGui::End();
  }

  void w_main(){
    ImGui::Text("Add new user.");
    ImGui::Separator();

    ImGui::InputText("Username", input_username, 256);
    ImGui::Separator();

    ImGui::InputText("Password", input_password, 256, ImGuiInputTextFlags_Password);
    ImGui::Separator();

    if(ImGui::Button("Add new")){
      std::array<char, 128> buffer;
      std::string result;
      std::string cmd = std::string("openssl passwd -apr1 ") + std::string(input_password);

      std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
      if (!pipe) {
          throw std::runtime_error("popen() failed!");
      }

      while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
          result += buffer.data();
      }

      std::string key = std::string("user:") + std::string(input_username);

      std::cout << key << " password " << result << std::endl;

      client.hset(key.c_str(), "password", result.c_str());
      client.sync_commit();

      memset(input_username, 0, sizeof input_username);
      memset(input_password, 0, sizeof input_password);

      windows->hide("users");
    }
  }
};

#endif