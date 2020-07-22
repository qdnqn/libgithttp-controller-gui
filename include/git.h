#ifndef GIT_H
#define GIT_H

#ifndef GIT_TEMP_DIR
#define GIT_TEMP_DIR ".git/.tmp"
#define GIT_TEMP_ARCHIVE_NAME "deploy"
#define GIT_TEMP_ARCHIVE_TYPE "tar"
#endif

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <sys/stat.h>

struct Pagination {
	int _step = 20;
	int start = 0;
	int end = 0;
	int max = 0;
	int counter = 0;

	std::string status = "";

	Pagination(){
		end += _step;
	}

	void next(){
		if(start < max){
			start += _step;
			end += _step;
			counter = 0;
		}
	}

	void previous(){
		if(start > 0){
			start -= _step;
			end -= _step;
			counter = 0;
		}
	}

	bool step(){
		++counter;

		std::cout << "Start: " << start << " End: " << end << " Counter: " << counter << std::endl;

		if(counter >= start && counter <= end){
			status = "show";
			return true;
		} else if(counter < start){
			status = "less";
		} else if(counter > end){
			counter = 0;
			status = "more";
		} else {
			status = "startorend";
		}

		return false;
	}
};

struct Commit {
	Commit(char* o, const char* m, git_time_t gitTime, bool isNew_ = false){
		oid = o;
		message = m;
		time = gitTime;
		_isNew = isNew_;
	}

	bool isNew(){
		return _isNew;
	}

	std::string oid;
	std::string message;
	time_t time;
	bool _isNew = false;
};

enum print_options {
	SKIP = 1,
	VERBOSE = 2,
	UPDATE = 4,
};

struct print_payload {
	enum print_options options;
	git_repository *repo;
};

class Git {
	private:
		std::string _repo;
		std::string _lastoid;
		const char* _path;
		std::string _pathString;
		std::string _workdir;
		std::string _activeBranch;

		git_status_list *statuses = NULL;
		git_repository * _repoPtr = nullptr;
		std::vector<Commit*> _commits;
		size_t _commitNumber = 0;
		int err = 0;

		bool _haveChanges = false;
	public:
		Pagination pagination;

		Git(const char* pth) : _path{pth}, _pathString{pth} {
			git_libgit2_init();

			int error = git_repository_open(&_repoPtr, _path);

			if(error < 0){
				err = error;
				GitError(error);
			} else {
				_activeBranch = "master";

				if(!git_repository_is_bare(_repoPtr))
					_workdir = git_repository_workdir(_repoPtr);
			}
		}

		~Git(){
			if(_commits.size() > 0){
				for ( auto p : _commits )
    			delete p;
			}

			if(_repoPtr != nullptr){
				git_repository_free(_repoPtr);
			}
		}

		std::string getWorkdir(){
			return _workdir;
		}

		std::string getActiveBranch(){
			return _activeBranch;
		}

		std::string setActiveBranch(std::string branch){
			_activeBranch = branch;
		}

		void reload(){
			git_repository_free(_repoPtr);

			if(git_repository_open(&_repoPtr, _pathString.c_str())){
				_repoPtr = nullptr;
			}
		}

		void readCommits(){
			if(_repoPtr != nullptr){
				git_revwalk * walker = nullptr;
        git_revwalk_new(&walker, _repoPtr);

        // sort log by chronological order
        git_revwalk_sorting(walker, GIT_SORT_NONE);

        // start from HEAD
        git_revwalk_push_head(walker);

        // -- walk the walk --
        git_oid oid;
        _commits.clear();

        int first = true;

        if(pagination.max == 0){
        	while(!git_revwalk_next(&oid, walker)){	          
	          _commitNumber++;
	        }

	        pagination.max = _commitNumber;
        } 

        git_revwalk_push_head(walker);

        while(!git_revwalk_next(&oid, walker)){
          if(pagination.step()){
          	// -- get the current commit --
	          git_commit * commit = nullptr;
	          git_commit_lookup(&commit, _repoPtr, &oid);
	         
	         	if(first){
	          	_commits.push_back(new Commit(git_oid_tostr_s(&oid), git_commit_summary(commit), git_commit_time(commit), true));
	          	first = false;
	          } else {
	          	_commits.push_back(new Commit(git_oid_tostr_s(&oid), git_commit_summary(commit), git_commit_time(commit)));
	          }

	 					_lastoid = std::string(git_oid_tostr_s(&oid));

	          git_commit_free(commit);
          } else {
          	if(pagination.status == "more"){
          		std::cout << _commits.size();
          		break;
          	}
          }
        }

        git_revwalk_free(walker);
			}
		}

		void commit(std::string message, std::string name, std::string mail){
			if(_repoPtr != nullptr){
				git_oid tree_id, parent_id, commit_id;
				git_tree *tree;
				git_commit *parent;
				git_index *index;

				/* Get the index and write it to a tree */
				git_repository_index(&index, _repoPtr);
				git_index_write_tree(&tree_id, index);
	
				git_reference_name_to_id(&parent_id, _repoPtr, "HEAD");
				git_commit_lookup(&parent, _repoPtr, &parent_id);

				git_tree_lookup(&tree, _repoPtr, &tree_id);

				git_signature *me = NULL;
				int error = git_signature_now(&me, name.c_str(), mail.c_str());

				std::cout << error << "XX" << std::endl;
							
				int err = git_commit_create_v(
				    &commit_id,
				    _repoPtr,
				    "HEAD",     
				    me,
				    me,
				    NULL,       
				    message.c_str(),
				    tree,      
				    1,          
				    parent      
				);

				git_index_free(index);
				git_commit_free(parent);
			} else {
				std::cout << "NOT LOADED"; 
			}
		}

		void add(){ 
			if(_repoPtr != nullptr){
				git_repository_set_workdir(_repoPtr, _pathString.c_str(), 1);

				git_index_matched_path_cb matched_cb = NULL;
				git_index *index;
				git_strarray array = {0};
				
				int options = 0, count = 0;
				struct print_payload payload;

				git_repository_index(&index, _repoPtr);
				git_index_add_all(index, &array, 0, matched_cb, &payload);
				git_index_write(index);
				git_index_free(index);
			} else {
				std::cout << "NULL PTR PROBLEM" << std::endl;
			}
		}

		void checkout(std::string oid){
			std::string tempDir = _pathString + std::string("/") + std::string(GIT_TEMP_DIR);
				
			if (!createDirectory(tempDir)){
			    printf("Error creating directory!n");
			} else {
				std::ofstream fs;

				/* Prevent changes when working with repository (Git hooks) */
				std::string hook = _pathString + std::string("/.git/hooks/pre-commit");
  			fs.open (hook.c_str());
  			fs << "#!/bin/bash\n";
  			fs << "echo \"Can't commit right now. Please try again.\"\n";
  			fs << "exit 1\n";
  			fs.close();

  			/* Make it executable */
  			chmod(hook.c_str(), S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH|S_IXUSR);

				git_repository_set_workdir(_repoPtr, tempDir.c_str(), 0);
				
				git_commit *commit;
				git_revparse_single((git_object **)&commit, _repoPtr, oid.c_str());

				git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
				opts.checkout_strategy = GIT_CHECKOUT_FORCE;

				git_checkout_tree(_repoPtr, (git_object *)commit, &opts);
			
				git_commit_free(commit);

				reload();

				git_repository_set_workdir(_repoPtr, _pathString.c_str(), 1);

				git_index_matched_path_cb matched_cb = NULL;
				git_index *index;
				git_strarray array = {0};
				
				int options = 0, count = 0;
				struct print_payload payload;

				git_repository_index(&index, _repoPtr);
				git_index_add_all(index, &array, 0, matched_cb, &payload);
				git_index_write(index);
				git_index_free(index);

				/*
				
			  */
			}

			/* 
			List status GIT -> Future use maybe? Discarded in this method.

			git_status_options opts2 = GIT_STATUS_OPTIONS_INIT;
			git_status_list *statuses = NULL;
			
			git_status_list_new(&statuses, _repoPtr, &opts2);

			size_t count = git_status_list_entrycount(statuses);

			for (size_t i=0; i<count; ++i) {
			  const git_status_entry *entry = git_status_byindex(statuses, i);
			  
			  if(entry->status == GIT_STATUS_WT_NEW){
			  	std::cout << "Untracked: " << entry->index_to_workdir->old_file.path << std::endl;
			  } else {
			  	if (entry->status & GIT_STATUS_WT_MODIFIED)
						wstatus = "modified: ";
					if (entry->status & GIT_STATUS_WT_DELETED)
						wstatus = "deleted:  ";
					if (entry->status & GIT_STATUS_WT_RENAMED)
						wstatus = "renamed:  ";
					if (entry->status & GIT_STATUS_WT_TYPECHANGE)
						wstatus = "typechange:";

					if (entry->status & GIT_STATUS_INDEX_NEW)
						wstatus = "new file: ";
					if (entry->status & GIT_STATUS_INDEX_MODIFIED)
						wstatus = "modified: ";
					if (entry->status & GIT_STATUS_INDEX_DELETED){
						wstatus = "deleted:  ";

						std::cout << "Deleted: " << entry->head_to_index->new_file.path << std::endl;

						
					}
					if (entry->status & GIT_STATUS_INDEX_RENAMED)
						wstatus = "renamed:  ";
					if (entry->status & GIT_STATUS_INDEX_TYPECHANGE)
						wstatus = "typechange:";
			  }
			}
			*/
		}

		std::vector<const git_status_entry*> getStatus(){
			std::vector<const git_status_entry*> files;
			
			git_status_options opts = GIT_STATUS_OPTIONS_INIT;

			opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
			opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX | GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;
			
			//git_status_options opts2 = GIT_STATUS_OPTIONS_INIT;
						
			git_status_list_new(&statuses, _repoPtr, &opts);

			size_t count = git_status_list_entrycount(statuses);

			for (size_t i=0; i<count; ++i) {
			  const git_status_entry *entry = git_status_byindex(statuses, i);
			  
			  files.push_back(entry);
			}

			if(count > 0){
				_haveChanges = true;
			} else {
				_haveChanges = false;
			}

			return files;
		}

		void statusFree(){
			git_status_list_free(statuses);
		}

		std::vector<std::string> getBranches(){
			std::vector<std::string> branches;

			git_branch_iterator *it;
			git_reference *outref;
			git_branch_t x;
			git_branch_t list_flags = GIT_BRANCH_LOCAL;

			git_branch_iterator_new(&it, _repoPtr, list_flags);

			int error = 0;

			while((error = git_branch_next(&outref, &x, it)) == 0){
				if(error == GIT_ITEROVER){
					break;
				}

				branches.push_back(git_reference_shorthand(outref));
				git_reference_free(outref);
			}

			git_branch_iterator_free(it);

			return branches;
		}

		bool haveChanges(){
			return _haveChanges;
		}

		bool isGitRepo(){
			if(_repoPtr != nullptr){
				return true;
			} else {
				return false;
			}
		}

		void GitError(int error){
			const git_error *e = giterr_last();
			printf("Error %d/%d: %s\n", error, e->klass, e->message);
		}

		std::string getLastError(){
			const git_error *e = giterr_last();
			return std::string("Git error: ") + e->message;
		}

		bool isSuccess(){
			if(err == 0){
				return true;
			} else {
				return false;
			}
		}

		/*Commit getCommit(std::string oid){
			return _commits;
		}*/

		std::vector<Commit*> getCommits(){
			return _commits;
		}

		std::string getLastOid(){
			return _lastoid;
		}

		size_t getNumberOfCommits(){
			return _commitNumber;
		}

};

#endif
