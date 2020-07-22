#ifndef W_MANAGER_REPO_H
#define W_MANAGER_REPO_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include <libssh/libssh.h>

#include "imgui.h"
#include "types.h"

#include "../include/git.h"
#include "../include/directory.hpp"
#include "../include/repo.h"

#define MAX_VIEW 20

class Window_ManagerRepos {
private:
  WindowState W_STATE;

  RenderRegistry        *windows;
  AppConsole *console;

  Repo repo;
  Git *grepo;

  std::vector<int> pagination;
  std::string error;

  std::string path;

  Directory dir;

  bool OpenPopup = false;
public:
  /**/
  /* Constructors and window related helper functions*/
  
  Window_ManagerRepos(RenderRegistry *w, AppConsole *c){
    pagination.push_back(0);
    pagination.push_back(0);
    pagination.push_back(0);

    windows = w;
    console = c;
  }

  ~Window_ManagerRepos(){
  }

  WindowState wstate(){
    return W_STATE;
  }

  WindowState changeWState(const char* state){
    W_STATE = state;

    if(W_STATE == "main"){
      reborn();
    }

    return W_STATE;
  }

  WindowState reborn(){ 
    pagination.at(0) = 0;
    pagination.at(1) = 0;
    pagination.at(2) = 0;

    return W_STATE;
  }

  /**/
  /* Getters and setters */

  Repo* getActiveRepoPtr(){
    return &repo;
  }

  Git* getActiveGRepoPtr(){
    return grepo;
  }

  void setPath(std::string p){
    path = p;
  }

  /**/
  /* Rendering and display */

  void render(){  
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_MenuBar;
    window_flags |= ImGuiWindowFlags_NoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoMove;

    ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(623, 327));
    ImGui::Begin(dir.getFilename(path).c_str(), NULL, window_flags);
    
    if (ImGui::BeginMenuBar()){
      if (ImGui::BeginMenu("Options")){
          if (ImGui::MenuItem("Close")) windows->hide("commits");
          ImGui::EndMenu();
      }

      ImGui::EndMenuBar();
    }

    if(W_STATE == "main"){
      pagination.at(0) = 0;
      pagination.at(1) = 0;

      w_main(path);
    } else if(W_STATE == "repo"){

      if (ImGui::BeginMenuBar()){
        if (ImGui::BeginMenu("Branches")){
            for(auto const& value: grepo->getBranches()) {
              if (ImGui::MenuItem(value.c_str())) changeWState("main");
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
      }


      w_repo();
    } else {
      w_invalid();
    }

    ImGui::End();
  }

  void w_main(std::string path){
    std::cout << "Trying path: " << path << std::endl;
    path += std::string("/");

    grepo = new Git(path.c_str());
                    
    if(grepo->isGitRepo()){
      grepo->readCommits();
    }
      
    repo.set(path);

    W_STATE = "repo";

    ImGui::Text("Main.");
  }

  void w_invalid(){
    ImGui::Text("Invalid action occured.");
  }

  void w_repo(){
    //title = std::string("Repository: ") + repo.name();

    std::string cmts;

    if(grepo->getNumberOfCommits() > MAX_VIEW){
      cmts = std::string("Commits(")+std::to_string(grepo->getNumberOfCommits())+std::string(") - Showing ") 
                         + std::to_string(grepo->pagination.start) + std::string("/") + std::to_string(grepo->getNumberOfCommits())
                         + std::string(" from ") + grepo->getActiveBranch();
    } else {
      cmts = std::string("Commits(")+std::to_string(grepo->getNumberOfCommits())+std::string(") - Showing ") 
                         + std::to_string(grepo->getNumberOfCommits()) + std::string("/") + std::to_string(grepo->getNumberOfCommits())
                         + std::string(" from ") + grepo->getActiveBranch();
    }

    ImGui::Text(cmts.c_str());
    ImGui::Separator();

    for(auto const& value: grepo->getCommits()) {
      std::string temp = std::string("repo_child")+value->oid;

      ImGui::BeginChild(temp.c_str(), ImVec2(0, ImGui::GetFontSize() * 9.6), true);
        ImGui::Columns(2, "mycolumns"); // 4-ways, with border
        //ImGui::Separator();
        ImGui::Text("Status"); ImGui::NextColumn();

        if(value->oid.c_str() == repo.active_local()){
          ImGui::PushID(1);

          ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(3/7.0f, 0.6f, 0.6f));
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(3/7.0f, 0.7f, 0.7f));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(3/7.0f, 0.8f, 0.8f));
          if(ImGui::SmallButton("In deploy")){
            
          }

          ImGui::PopStyleColor(3);
          ImGui::PopID();
        } else {
          ImGui::TextDisabled("Not in deploy");
        }

        ImGui::NextColumn();
        ImGui::Separator();
        ImGui::Text("Oid"); ImGui::NextColumn();
        ImGui::Text(value->oid.c_str()); ImGui::NextColumn();
        ImGui::Separator();
        ImGui::Text("Message"); ImGui::NextColumn();
        ImGui::Text(value->message.c_str()); ImGui::NextColumn();
        ImGui::Separator();
        ImGui::Text("Deploy"); ImGui::NextColumn();
        
        if (ImGui::Button("Activate trigger")) {
          grepo->checkout(value->oid);  

          std::string trigger = std::string("triggers/")+dir.getFilename(path)+std::string(":trigger.lua");

          if(PathExist(trigger)){
              console->AddLog("[info] Manually activating trigger: %s", trigger.c_str());
              windows->hide("commits");
          } else {
              console->AddLog("[info] Trigger not found: %s", trigger.c_str());
              windows->hide("commits");
          }
        }

        if(value->isNew() && grepo->pagination.start == 0){ // Check if user is on first page of all commits
          ImGui::SameLine();
          ImGui::TextColored(ImVec4(1.0f,1.0f,0.0f,1.0f), "Last commit");
        }
        
        ImGui::NextColumn();
        ImGui::Separator();

        ImGui::Text("Commited on"); ImGui::NextColumn();
        ImGui::Text(ctime(&value->time)); ImGui::NextColumn();
        ImGui::Separator();

        

        //ImGui::Separator();
        ImGui::Columns(1);

      ImGui::EndChild();
    }

    if(grepo->getNumberOfCommits() > MAX_VIEW){
      ImGui::Separator();

      if(ImGui::Button("Previous")) {
        ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y, 0.0);
        grepo->pagination.previous();
        grepo->readCommits();
      }

      ImGui::SameLine();

      if(ImGui::Button("Next")) {
        ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y, 0.0);
        grepo->pagination.next();
        grepo->readCommits();
      } 
    }
  }
};

#endif;
