#ifndef REPO_H
#define REPO_H

#include <string>
#include "directory.hpp"

class Repo {
private:
	std::string name_;
	std::string path_;
	std::string id_;
	std::string active_local_;
	std::string workdir_ = "Affected files: ";

	std::string message_;
public:
	Repo(){}
	Repo(std::string name, std::string path): name_{name},path_{path_}{}
	Repo(std::string name, std::string path, std::string active_local): name_{name},path_{path_}, active_local_{active_local}{}
	Repo(std::string path): path_{path_}{
		Directory dir;
		name_ = dir.getFilename(path);
	}

	Repo(const Repo& obj){
		name_ = obj.name_;
		path_ = obj.path_;
	}

	Repo& operator=(const Repo& obj){
		name_ = obj.name_;
		path_ = obj.path_;
		active_local_ = obj.active_local_;

		return *this;
	}

	void set(std::string name, std::string path, std::string active_local, std::string workdir){
		name_ = name;
		path_ = path;
		active_local_ = active_local;
		workdir_ = workdir;
	}

	void set(std::string name, std::string path, std::string active_local){
		name_ = name;
		path_ = path;
		active_local_ = active_local;
	}

	void set(std::string name, std::string path){
		name_ = name;
		path_ = path;
	}

	void set(std::string path){
		Directory dir;
		name_ = dir.getFilename(path);
		path_ = path;
	}

	void workdir(std::string workdir){
		workdir_ = workdir;
	}

	void message(std::string m){
		message_ = m;
	}

	std::string message(){
		return message_;
	}

	std::string workdir(){
		return workdir_;
	}

	std::string name(){
		return name_;
	}

	std::string path(){
		return path_;
	}

	std::string id(){
		return id_;
	}

	std::string active_local(){
		return active_local_;
	}

	std::string active_local(std::string active_local){
		active_local_ = active_local;
	}
};

#endif
