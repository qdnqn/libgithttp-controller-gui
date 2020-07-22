#ifndef I_DIRECTORY
#define I_DIRECTORY

#include <experimental/filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/stat.h>

bool PathExist(const std::string &s){
  struct stat buffer;
  return (stat (s.c_str(), &buffer) == 0);
}

bool createDirectory(const std::string &path){
  if(!PathExist(path)){
    const int dir_err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    if(-1 == dir_err){
      return false;
    } 
  }

  return true;
}

std::string dirOrFile(const std::string & path){
  struct stat s;

  if(stat(path.c_str(),&s) == 0){
      if(s.st_mode & S_IFDIR){
          return "dir";
      } else if( s.st_mode & S_IFREG ){
          return "file";
      } else{
          return "undefined"; // It's neither file nor directory 
      }
  } else{
      return "error";
  }
}

namespace fs = std::experimental::filesystem::v1;

class Directory {
private:
  std::vector<std::string> files; 
  std::vector<std::string> dirs;
public:
  bool getContentsRecursive(std::string path, std::vector<std::string> ignore=std::vector<std::string>()){
    files.clear();

    try {
      for (fs::recursive_directory_iterator i(path.c_str()), end; i != end; ++i){
        /* GENERAL IGNORING: if pattern found in string ignore */
        bool flag = false;

        for(auto const& value: ignore) {
          if(std::find(i->path().begin(), i->path().end(), value) != i->path().end()){
            flag = true;    
          }
        }

        if(!flag)
          if (!fs::is_directory(i->path()))
            /* Ignore files */
            if(std::find(ignore.begin(), ignore.end(), getFilename(i->path())) != ignore.end()){
              continue;
            } else {
              files.push_back(i->path());
            }
      }

      return true;
    } catch(...){
      return false;
    }
  }

  bool getContents(std::string path){
    files.clear();
    dirs.clear();

    try {
      for (fs::directory_iterator i(path.c_str()), end; i != end; ++i){
          if (!fs::is_directory(i->path())){
            files.push_back(i->path());
          } else {
            std::cout << i->path() << std::endl;
            dirs.push_back(i->path());
          }
      }

      return true;
    } catch(...){
      return false;
    }
  }

  bool create(std::string path){
    return fs::create_directory(path.c_str());
  }

  std::vector<std::string> getFiles(){
    return files;
  }

  std::vector<std::string> getDirs(){
    return dirs;
  }

  std::string getFilename(const std::string& s){
    char sep = '/';

    #ifdef _WIN32
       sep = '\\';
    #endif

    size_t i = s.rfind(sep, s.length());
    if (i != std::string::npos) {
      return(s.substr(i+1, s.length() - i));
    }

    return("");
   }
 };

#endif