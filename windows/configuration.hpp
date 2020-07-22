#ifndef W_CONFIGURATION
#define W_CONFIGURATION

#include <iostream>
#include <string>
#include <fstream>
#include <sqlite3.h>
#include <fstream>

#include "../include/rapidjson/document.h"
#include "../include/rapidjson/writer.h"
#include "../include/rapidjson/stringbuffer.h"

#include "imgui.h"
#include "types.h"

static void ShowHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

class Window_ManagerConfiguration {
private:
  WindowState W_STATE;
  RenderRegistry *windows;

  char input_path[256]        = "Path to repositories";
  char input_rhost[256]       = "Redis host";
  char input_rport[256]       = "Redis port";
  char input_mhost[256]       = "Mongo url";

  rapidjson::Document document;

public:
  Window_ManagerConfiguration(RenderRegistry* w){
    windows = w;

    std::string line, json;

    std::ifstream config;
    std::ofstream configw;
  
    config.open ("config.json");

    if(config.is_open()){
      while (getline(config,line)){
        json += line;
      }

      document.Parse(json.c_str());

      if(!document.HasMember("path") || !document.HasMember("redis_host") || !document.HasMember("redis_port") || !document.HasMember("path"))
        throw "Configuration is invalid!";

      std::strcpy(&input_path[0], document["path"].GetString());
      std::strcpy(&input_rhost[0], document["redis_host"].GetString());
      std::strcpy(&input_rport[0], std::to_string(document["redis_port"].GetInt()).c_str());
      std::strcpy(&input_mhost[0], document["mongo_url"].GetString());

      config.close();      
    } else {
      configw.open ("config.json");

      if(configw.is_open()){
        configw << "{\"path\":\"\",\"redis_host\":\"\",\"redis_port\":0, \"mongo_url\":\"\"}\n";
        configw.close();
      } else {
        throw "Not able to edit configuration file!";
      }
    }
  }

  ~Window_ManagerConfiguration(){
  }

  WindowState wstate(){
    return W_STATE;
  }

  void clear(){
  }

  std::string getPath(){
    return std::string(input_path);
  }

  void render(){
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_MenuBar;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(438, 327));
    ImGui::Begin("Configuration", NULL, window_flags);

    if (ImGui::BeginMenuBar()){
      if(W_STATE == "main"){
        if (ImGui::BeginMenu("Options")){
            if (ImGui::MenuItem("Close")) windows->hide("configuration");
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
    ImGui::Text("Edit your configuration. ");
    ImGui::Separator();

    ImGui::InputText("Path to repositories", input_path, 256);
    ImGui::Separator();

    ImGui::InputText("Redis host", input_rhost, 256);
    ImGui::Separator();

    ImGui::InputText("Redis port", input_rport, 256);
    ImGui::Separator();

    ImGui::InputText("Mongo url", input_mhost, 256);
    ImGui::Separator();

    if(ImGui::Button("Update configuration")){
      std::ofstream configw;

      configw.open ("config.json");

      if(configw.is_open()){
        std::string save = std::string("{\"path\":\"")+
                           std::string(input_path)+
                           std::string("\",\"redis_host\":\"")+
                           std::string(input_rhost)+
                           std::string("\",\"redis_port\":")+
                           std::string(input_rport)+
                           std::string(",\"mongo_url\":\"")+
                           std::string(input_mhost)+
                           std::string("\"}\n");
        configw << save;
        configw.close();
      } else {
        throw "Not able to edit configuration file!";
      }
    }
  }
};

#endif