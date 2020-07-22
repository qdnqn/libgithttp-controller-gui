#include "types.h"
#include <vector>
#include <map>
#include <string>

#include <cpp_redis/cpp_redis>

#include "../include/rapidjson/document.h"
#include "../include/rapidjson/writer.h"
#include "../include/rapidjson/stringbuffer.h"

#include "../include/directory.hpp"

#include "../windows/configuration.hpp"

#include <lua.hpp>

int AddLogStatic(lua_State *state);

struct AppConsole
{
    char                  InputBuf[256];
    ImVector<char*>       Items;
    bool                  ScrollToBottom;
    ImVector<char*>       History;
    int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    ImVector<const char*> Commands;

    std::string active_repo;

    cpp_redis::client client;

    RenderRegistry        *windows;
    std::string path;

    AppConsole* me;

    Directory dir;

    std::map<std::string, std::vector<std::string>> childs;

    AppConsole(RenderRegistry *w, Window_ManagerConfiguration &conf)
    {
        ClearLog();
        memset(InputBuf, 0, sizeof(InputBuf));
        HistoryPos = -1;
        Commands.push_back("config");
        //Commands.push_back("install");
        //Commands.push_back("start");
        //Commands.push_back("stop");
        //Commands.push_back("restart");
        //Commands.push_back("log");
        //Commands.push_back("commits");
        Commands.push_back("users");
        Commands.push_back("repos");
        Commands.push_back("commits");
        Commands.push_back("clear active");
        Commands.push_back("hello");

        AddLog("Continuous integration / Continuous delivery console!");

        path = conf.getPath();

        windows = w;

        client.connect("127.0.0.1", 6379,
           [](const std::string &host, std::size_t port, cpp_redis::connect_state status) {
               if (status == cpp_redis::connect_state::dropped) {
                   std::cout << "client disconnected from " << host << ":" << port << std::endl;
               } 
           }); 
    }

    ~AppConsole()
    {
        ClearLog();
        for (int i = 0; i < History.Size; i++)
            free(History[i]);
    }

    std::string getActivePath(){
        return active_repo;
    }

    // Portable helpers
    static int   Stricmp(const char* str1, const char* str2)         { int d; while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; } return d; }
    static int   Strnicmp(const char* str1, const char* str2, int n) { int d = 0; while (n > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; n--; } return d; }
    static char* Strdup(const char *str)                             { size_t len = strlen(str) + 1; void* buff = malloc(len); return (char*)memcpy(buff, (const void*)str, len); }
    static void  Strtrim(char* str)                                  { char* str_end = str + strlen(str); while (str_end > str && str_end[-1] == ' ') str_end--; *str_end = 0; }

    void ClearLog()
    {
        for (int i = 0; i < Items.Size; i++)
            free(Items[i]);
        Items.clear();
        ScrollToBottom = true;
    }

    void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        // FIXME-OPT
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf)-1] = 0;
        va_end(args);
        Items.push_back(Strdup(buf));
        ScrollToBottom = true;
    }

    void newMessage(const std::string& chan, const std::string& msg){
        rapidjson::Document document;

        document.Parse(msg.c_str());

        std::cout << document["type"].GetString() << std::endl;

        if(Stricmp(document["type"].GetString(), "push") == 0){
            AddLog("Push in %s repository by user %s", document["repo"].GetString(), document["user"].GetString());
            AddLog("[%s]", document["new_oid"].GetString());
        
            for (auto it = begin (childs[document["new_oid"].GetString()]); it != end (childs[document["new_oid"].GetString()]); ++it) {
                AddLog(" -- [%s]", it->c_str());
            }


            std::string trigger = std::string("triggers/")+std::string(document["repo"].GetString())+std::string(".repo:trigger.lua");
            std::string repo = document["repo"].GetString();

            if(PathExist(trigger)){
                AddLog("[info] Activating trigger: %s", trigger.c_str());

                lua_State* L;

                L = luaL_newstate();
                            
                luaL_openlibs(L); 

                lua_pushlightuserdata(L, this);
                lua_pushcclosure(L, AddLogStatic, 1);
                lua_setglobal(L, "AddLogStatic");
                            
                if (luaL_loadfile(L, trigger.c_str()) || lua_pcall(L, 0, 0, 0))
                    printf("cannot run configuration file: %s",lua_tostring(L, -1));
                     
                lua_getglobal(L, "trigger");
                lua_pushstring(L, msg.c_str());            
                
                if(lua_pcall(L, 1, 0, 0) != 0)
                    printf("error running function `f': %s \n", lua_tostring(L, -1));
                            
                lua_close(L);
            }

            childs[document["new_oid"].GetString()].clear();
        } else if(Stricmp(document["type"].GetString(), "pull") == 0){
            AddLog("Pull from %s repository by user %s", document["repo"].GetString(), document["user"].GetString());
        
            for (auto it = begin (childs[document["ref_oid"].GetString()]); it != end (childs[document["ref_oid"].GetString()]); ++it) {
                AddLog(" -- [%s]", it->c_str());
            }

            std::string trigger = std::string("triggers/")+std::string(document["repo"].GetString())+std::string(".repo:trigger.lua");
            std::string repo = document["repo"].GetString();

            if(PathExist(trigger)){
                AddLog("[info] Activating trigger: %s", trigger.c_str());

                lua_State* L;

                L = luaL_newstate();
                            
                luaL_openlibs(L); 

                //lua_register(L, "AddLog", AddLogStatic);
                
                if (luaL_loadfile(L, trigger.c_str()) || lua_pcall(L, 0, 0, 0))
                    printf("cannot run configuration file: %s",lua_tostring(L, -1));
                     
                lua_getglobal(L, "trigger");
                lua_pushstring(L, msg.c_str());            
                
                if(lua_pcall(L, 1, 0, 0) != 0)
                    printf("error running function `f': %s \n", lua_tostring(L, -1));
                            
                lua_close(L);
            }
        
            childs[document["new_oid"].GetString()].clear();
        } else if(Stricmp(document["type"].GetString(), "child-commit") == 0){
            childs[document["parent_oid"].GetString()].push_back(document["child_oid"].GetString());
        } else if(Stricmp(document["type"].GetString(), "child-pull") == 0){
            childs[document["parent_oid"].GetString()].push_back(document["child_oid"].GetString());
        } else {
            AddLog("Unknown action occured.");
        }
    }

    void    Draw(const char* title, bool* p_open)
    {
        ImGuiWindowFlags window_flags = 0;
        window_flags |= ImGuiWindowFlags_MenuBar;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::SetNextWindowPosCenter(ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(450,500));

        if (!ImGui::Begin(title, p_open, window_flags))
        {
            ImGui::End();
            return;
        }

        // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar. So e.g. IsItemHovered() will return true when hovering the title bar.
        // Here we create a context menu only available from the title bar.
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Close Console"))
                *p_open = false;
            ImGui::EndPopup();
        }

        // TODO: display items starting from the bottom

        ImGui::Separator();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
        static ImGuiTextFilter filter;
        filter.Draw("Filter (\"incl,-excl\") (\"error\")", 180);
        ImGui::PopStyleVar();
        ImGui::Separator();

        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear")) ClearLog();
            ImGui::EndPopup();
        }

        // Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
        // NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping to only process visible items.
        // You can seek and display only the lines that are visible using the ImGuiListClipper helper, if your elements are evenly spaced and you have cheap random access to the elements.
        // To use the clipper we could replace the 'for (int i = 0; i < Items.Size; i++)' loop with:
        //     ImGuiListClipper clipper(Items.Size);
        //     while (clipper.Step())
        //         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
        // However, note that you can not use this code as is if a filter is active because it breaks the 'cheap random-access' property. We would need random-access on the post-filtered list.
        // A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices that passed the filtering test, recomputing this array when user changes the filter,
        // and appending newly elements as they are inserted. This is left as a task to the user until we can manage to improve this example code!
        // If your items are of variable size you may want to implement code similar to what ImGuiListClipper does. Or split your data into fixed height items to allow random-seeking into your list.
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1)); // Tighten spacing
        //if (copy_to_clipboard)
        //   ImGui::LogToClipboard();
        ImVec4 col_default_text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        for (int i = 0; i < Items.Size; i++)
        {
            const char* item = Items[i];
            if (!filter.PassFilter(item))
                continue;
            ImVec4 col = col_default_text;
            if (strstr(item, "[error]")) col = ImColor(1.0f,0.4f,0.4f,1.0f);
            else if (strstr(item, "[info]")) col = ImColor(1.0f,0.1f,0.5f,1.0f);
            else if (strncmp(item, "# ", 2) == 0) col = ImColor(1.0f,0.78f,0.58f,1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, col);
            ImGui::TextUnformatted(item);
            ImGui::PopStyleColor();
        }
        
        //if (copy_to_clipboard)
        //    ImGui::LogFinish();
        if (ScrollToBottom)
             ImGui::SetScrollHereY(1.0f);
        
        ScrollToBottom = false;
        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        // Command-line
        bool reclaim_focus = false;

        if(active_repo.size() > 0){
            if (ImGui::InputText(dir.getFilename(active_repo).c_str(), InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCompletion|ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
            {
                char* s = InputBuf;
                Strtrim(s);
                if (s[0])
                    ExecCommand(s);
                strcpy(s, "");
                reclaim_focus = true;
            }
        } else {
            if (ImGui::InputText("Input", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCompletion|ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this))
            {
                char* s = InputBuf;
                Strtrim(s);
                if (s[0])
                    ExecCommand(s);
                strcpy(s, "");
                reclaim_focus = true;
            }
        }

        // Auto-focus on window apparition
        ImGui::SetItemDefaultFocus();
        if (reclaim_focus)
            ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

        ImGui::End();
    }

    void    ExecCommand(const char* command_line)
    {
        AddLog("# %s\n", command_line);

        // Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or optimal.
        HistoryPos = -1;
        for (int i = History.Size-1; i >= 0; i--)
            if (Stricmp(History[i], command_line) == 0)
            {
                free(History[i]);
                History.erase(History.begin() + i);
                break;
            }
        History.push_back(Strdup(command_line));

        // Process command
        if (Stricmp(command_line, "CLEAR") == 0)
        {
            ClearLog();
        } else if (Stricmp(command_line, "config") == 0){
            windows->show("configuration");
        } else if (Stricmp(command_line, "commits") == 0){
            if(active_repo.size() > 0){
                windows->hide("configuration");
                windows->show("commits");
                windows->show("commits-once");
            } else {
                AddLog("[error] You must set active repo first!");
            }
        } else if(Stricmp(command_line, "repos") == 0){
            dir.getContents(path);

            std::string basename;

            for (auto & element : dir.getDirs()) {
                basename = dir.getFilename(element);
                const char* x = basename.c_str();
                AddLog(" - %s", x);
                Commands.push_back(x);
            }

        }  else if(Stricmp(command_line, "clear active") == 0){
            windows->hide("commits");
            active_repo = "";

        } else if(Stricmp(command_line, "users") == 0){
            client.keys("user:*", [this](cpp_redis::reply &reply) {
                for (unsigned int i = 0; i < reply.as_array().size(); i+=1) {
                    std::string key =  reply.as_array()[i].as_string();

                    AddLog(" -%s", key.c_str());
                }
            });

            client.sync_commit();
        } else if(Stricmp(command_line, "users add") == 0){
            windows->show("users");
        } else if (Stricmp(command_line, "HELP") == 0) {
            AddLog("Commands:");
            for (int i = 0; i < Commands.Size; i++)
                AddLog("- %s", Commands[i]);
        }
        else if (Stricmp(command_line, "HISTORY") == 0)
        {
            int first = History.Size - 10;
            for (int i = first > 0 ? first : 0; i < History.Size; i++)
                AddLog("%3d: %s\n", i, History[i]);
        }
        else if (Stricmp(command_line, "hello") == 0)
        {
            AddLog("Hello to you too.");
        }
        else
        {
            if (strstr(command_line, "set active")){
                dir.getContents(path);

                std::string basename;

                for (auto & element : dir.getDirs()) {
                    basename = std::string("set active ")+dir.getFilename(element);
                    
                    if(Stricmp(command_line, basename.c_str()) == 0){
                        AddLog("Changed active repo to: %s", dir.getFilename(element).c_str());
                        active_repo = element;
                    }
                }
            } else if (strstr(command_line, "users allow")) {
                bool flag = false;
                std::string key;

                client.keys("user:*", [this, command_line, &key, &flag](cpp_redis::reply &reply) {
                    for (unsigned int i = 0; i < reply.as_array().size(); i+=1) {
                        std::string comp = std::string("users allow ") + reply.as_array()[i].as_string();
                        key = reply.as_array()[i].as_string();
                        
                        if(Stricmp(comp.c_str(), command_line) == 0){
                            AddLog("%s is now allowed on %s.", key.c_str(), dir.getFilename(active_repo).c_str());
                            flag = true;
                            break;
                        }
                    }
                });

                client.sync_commit();

                if(flag){
                    client.hset(key.c_str(), dir.getFilename(active_repo).c_str(), "1");
                    client.sync_commit();
                }

            } else {
                AddLog("Unknown command: '%s'\n", command_line);
            }
        }
    }

    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data) // In C++11 you are better off using lambdas for this sort of forwarding callbacks
    {
        AppConsole* console = (AppConsole*)data->UserData;
        return console->TextEditCallback(data);
    }

    int     TextEditCallback(ImGuiInputTextCallbackData* data)
    {
        //AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
        switch (data->EventFlag)
        {
        case ImGuiInputTextFlags_CallbackCompletion:
            {
                // Example of TEXT COMPLETION

                // Locate beginning of current word
                const char* word_end = data->Buf + data->CursorPos;
                const char* word_start = word_end;
                while (word_start > data->Buf)
                {
                    const char c = word_start[-1];
                    if (c == ' ' || c == '\t' || c == ',' || c == ';')
                        break;
                    word_start--;
                }

                // Build a list of candidates
                ImVector<const char*> candidates;
                for (int i = 0; i < Commands.Size; i++)
                    if (Strnicmp(Commands[i], word_start, (int)(word_end-word_start)) == 0)
                        candidates.push_back(Commands[i]);

                if (candidates.Size == 0)
                {
                    // No match
                    AddLog("No match for \"%.*s\"!\n", (int)(word_end-word_start), word_start);
                }
                else if (candidates.Size == 1)
                {
                    // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
                    data->DeleteChars((int)(word_start-data->Buf), (int)(word_end-word_start));
                    data->InsertChars(data->CursorPos, candidates[0]);
                    data->InsertChars(data->CursorPos, " ");
                }
                else
                {
                    // Multiple matches. Complete as much as we can, so inputing "C" will complete to "CL" and display "CLEAR" and "CLASSIFY"
                    int match_len = (int)(word_end - word_start);
                    for (;;)
                    {
                        int c = 0;
                        bool all_candidates_matches = true;
                        for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
                            if (i == 0)
                                c = toupper(candidates[i][match_len]);
                            else if (c == 0 || c != toupper(candidates[i][match_len]))
                                all_candidates_matches = false;
                        if (!all_candidates_matches)
                            break;
                        match_len++;
                    }

                    if (match_len > 0)
                    {
                        data->DeleteChars((int)(word_start - data->Buf), (int)(word_end-word_start));
                        data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
                    }

                    // List matches
                    AddLog("Possible matches:\n");
                    for (int i = 0; i < candidates.Size; i++)
                        AddLog("- %s\n", candidates[i]);
                }

                break;
            }
        case ImGuiInputTextFlags_CallbackHistory:
            {
                // Example of HISTORY
                const int prev_history_pos = HistoryPos;
                if (data->EventKey == ImGuiKey_UpArrow)
                {
                    if (HistoryPos == -1)
                        HistoryPos = History.Size - 1;
                    else if (HistoryPos > 0)
                        HistoryPos--;
                }
                else if (data->EventKey == ImGuiKey_DownArrow)
                {
                    if (HistoryPos != -1)
                        if (++HistoryPos >= History.Size)
                            HistoryPos = -1;
                }

                // A better implementation would preserve the data on the current input line along with cursor position.
                if (prev_history_pos != HistoryPos)
                {
                    const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, history_str);
                }
            }
        }
        return 0;
    }
};

int AddLogStatic(lua_State *state){
    AppConsole *klass = (AppConsole *) lua_touserdata(state, lua_upvalueindex(1));

    const char* buf = lua_tostring(state, 1);
    klass->AddLog("%s", buf);
}
