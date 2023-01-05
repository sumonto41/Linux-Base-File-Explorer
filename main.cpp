#include<grp.h>
#include<pwd.h>
#include<ftw.h>
#include<ctype.h>
#include<fstream>
#include<iomanip>
#include<stdlib.h>
#include<stdio.h>
#include<iostream>
#include<dirent.h>
#include<signal.h>
#include<unistd.h>
#include<termios.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/ioctl.h>
#include<bits/stdc++.h>
#define CTRL_KEY(k) ((k) & 0x1f)
using namespace std;
struct termios term;
stack<string> prevs, nxt; // stacks to track directories visited.
int mode=0, x, y, windond,windost;
int term_size(int *rows, int *cols) { //function to find the dimension of the terminal.
  struct winsize ts;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ts) == -1 || ts.ws_col == 0) {
    return -1;
  } else {
    *cols = ts.ws_col;
    *rows = ts.ws_row;
    return 0;
  }
}
void move_to(std::ostream& os, int row, int col) { // move cursor to (row, col).
  os << "\033[" << row << ";" << col << "H";
}
string short_path(string path) { //shrink the path if it is too long for printing.
    if(path.size()<=40)
        return path;
    string res="";
    int pos=0;
     for(int i=path.size()-1; i>=0; i--) {
        if(path[i]=='/' && path.size()-i<=36)
            pos=i;
        if(path.size()-i>36)
            break;
    }
    for(int i=pos; i<path.size();i++)
        res+=path[i];
    res="/..."+res;
    return res;
}
void print_header(const char * dir) { //function to print the header.
    cout << "\033[7;32m "<<"Present directory: "<<"\033[7;37m"<<left<<setw(89)<<short_path(dir)<<"\033[0m\r\n";
    cout <<"\033[1;7;32m  ";
    cout<<" "<<left<<setw(30)<<"File/ Directory";
    cout<<right<<setw(6)<<"Size";
    cout<<setw(12)<<"User";
    cout<<setw(12)<<"Group";
    cout<<setw(15)<<"Permissions";
    cout<<setw(30)<<"Last modified"<<" ";
    cout<<"\033[0m\r\n";
}
char sense() { //function to take input one character at a time.
  int cread;
  char c;
  while ((cread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (cread == -1 && errno != EAGAIN) cout<<"ERROR";
  }
  if (c == '\x1b') {
    if(mode)
        return c;
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
      switch (seq[1]) {
        case 'A': return 'w';
        case 'B': return 's';
        case 'C': return 'd';
        case 'D': return 'a';
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}
void di_raw() { //function to disable raw mode.
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}
void en_raw() { //function to enable raw mode.
  tcgetattr(STDIN_FILENO, &term);
  atexit(di_raw);
  struct termios raw = term;
  raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
string permission(int mode) { //function to convert permissions in string form.
    string s="";
    char chars[] = "rwxrwxrwx";
    for (size_t i = 0; i < 9; i++) {
        s+=((mode & (1 << (8-i))) ? chars[i] : '-');
    }
    return s;
}
string byte_to_XB(int bytes) { //function to convert number of bytes to B, KB, MB and GB.
    string s="";
    if(bytes>=1024*1024*1024) {
        s=to_string(bytes/(1024*1024*1024));
        s+="GB";
    } else if(bytes>=1024*1024) {
        s=to_string(bytes/(1024*1024));
        s+="MB";
    } else if(bytes>=1024) {
        s=to_string(bytes/1024);
        s+="KB";
    } else {
        s=to_string(bytes);
        s+="B";
    }
    return s;
}
string short_name(string name) { //function to shorten the names of files and directories if they are too long.
    string shrt_name="";
    for(int i=0;i<min((int)name.size(), 25); i++)
        shrt_name+=name[i];
    if(name.size()>25)
    shrt_name+="...";
    return shrt_name;
}
string parent(string pth) { //function to find parent directory.
    while(pth.back()!='/')
        pth.pop_back();
    if(pth.size()>1)
        pth.pop_back();
    return pth;
}
string absolute_path(string command) { //function to convert relative path to absolute path.
    if(command[0]!='/') {
        if(command==".")
            command=prevs.top();
        else if(command=="..")
            command=parent(prevs.top());
        else {
            if(command[0]=='~') {
                command="/home/"+(string)getlogin()+command.substr(1, command.size()-1);
            } else if(prevs.top()=="/")
                command="/"+command;
            else
                command=prevs.top()+"/"+command;
        }
    }
    return command;
}
void render(const char * dir, vector<string> files, vector<array<string, 5>> detail,int cursor) { //function to print the output on screen.
    cout<<"\033[H\033[J";
    if(mode)
        cout<<"\x1b[?25h";
    else
        cout<<"\x1b[?25l";
    print_header(dir);
    for(int i=windost; i<=windost+24; i++) {
        if(i<=windond) {
            if(cursor==i)
                cout<<"\033[1;7;36m>>";
            else
                cout<<"\033[7m  ";
            cout<<" "<<left<<setw(30)<<short_name(files[i]);
            cout<<right<<setw(6)<<detail[i][0];
            cout<<setw(12)<<detail[i][1];
            cout<<setw(12)<<detail[i][2];
            cout<<setw(15)<<detail[i][3];
            cout<<setw(30)<<detail[i][4];
            cout<<" "<<"\033[0m\r\n";
        } else {
            cout<<"\033[7m "<<setw(108)<<""<<"\033[0m\r\n";
        }
    }
    int x, y;
    term_size(&x, &y);
    move_to(cout, x, 1);
    if(mode==0)
        cout<<"\033[7mNORMAL MODE\033[0m";
    else if(mode==1)
        cout<<"\033[7mCOMMAND MODE\033[1;0m $ ";
    cout<<flush;
}
bool search(const char * dir, string name) { // function to search for files and directories.
    DIR *directory=opendir(dir);
    if(directory==NULL) return false;
    struct dirent *entity;
    entity=readdir(directory);
    vector<string> files;
    while(entity) {
        files.push_back(entity->d_name);
        entity=readdir(directory);
    }
    for(int i=0; i<files.size(); i++) {
        if(name==files[i])
            return true;
    }
    bool verdict=false;
    struct stat info;
    for(int i=0; i<files.size(); i++) {
        if(files[i]!="." && files[i]!=".." && !stat(((string)dir+(string)"/"+files[i]).c_str(), &info)) {
            if(S_ISDIR(info.st_mode)) {
                verdict|=search(((string)dir+(string)"/"+files[i]).c_str(), name);
            }
        }
    }
    return verdict;
}
void create(string command) { // function to create file and directory.
    if(command.substr(7, 4)=="file") {
        string name="";
        command=command.substr(12, command.size()-12);
        int i=0;
        while(command[i]!=' ') {
            name+=command[i];
            i++;
        }
        command=command.substr(i+1, command.size()-i-1);
        command=absolute_path(command);
        DIR *directory=opendir(command.c_str());
        if(directory==NULL) {
            cout<<"  ** ERROR: Invalid destination path. **    Press ENTER to continue"<<flush;
            sense();
            return;
        }
        else {
            struct dirent *entity;
            entity=readdir(directory);
            while(entity) {
                if((string)(entity->d_name)==name) {
                    cout<<"  ** ERROR: "+name+" already exists in destination. **    Press ENTER to continue"<<flush;
                    sense();
                    return;
                }
                entity=readdir(directory);
            }
        }
        command+="/"+name;
        ofstream file(command.c_str());
        file.close();
    } else {
        string name="";
        command=command.substr(11, command.size()-11);
        int i=0;
        while(command[i]!=' ') {
            name+=command[i];
            i++;
        }
        command=command.substr(i+1, command.size()-i-1);
        command=absolute_path(command);
        DIR *directory=opendir(command.c_str());
        if(directory==NULL) {
            cout<<"  ** ERROR: Invalid destination path. **    Press ENTER to continue"<<flush;
            sense();
            return;
        } else {
            struct dirent *entity;
            entity=readdir(directory);
            while(entity) {
                if((string)(entity->d_name)==name) {
                    cout<<"  ** ERROR: "+name+" already exists in destination. **    Press ENTER to continue"<<flush;
                    sense();
                    return;
                }
                entity=readdir(directory);
            }
        }
        command+="/"+name;
        mkdir(command.c_str(), 0775);
    }
}
static int remove_file(const char *pathname, const struct stat *sbuf, int type, struct FTW *ftwb)
{
    if(remove(pathname) < 0)
    {
        perror("ERROR: remove");
        return -1;
    }
    return 0;
}
void delet(string command) { //function to delete files and directories.
    if(command.substr(7, 4)=="file") {
        command=command.substr(12, command.size()-12);
        command=absolute_path(command);
        remove(command.c_str());
    } else {   
        command=command.substr(11, command.size()-11);
        command=absolute_path(command);
        nftw(command.c_str(), remove_file,10, FTW_DEPTH|FTW_MOUNT|FTW_PHYS);
    }
}
void cpy(string command, bool super) { //function to copy files and directories if super parameter is false, else it works as move.
    command=command.substr(5, command.size()-5);
    vector<string> file_to_copy;
    string temp="";
    int pos;
    for(int i=0; i<command.size(); i++) { // make list of files/ directories to copy.
        if(command[i]==' ') {
            file_to_copy.push_back(temp);
            pos=i;
            temp="";
        } else {
            temp+=command[i];
        }
    }
    command=command.substr(pos+1, command.size()-(pos+1));
    command=absolute_path(command);
    DIR *directory=opendir(command.c_str());
    if(directory==NULL) {
        cout<<"  ** ERROR: Invalid destination path. **    Press ENTER to continue"<<flush;
        sense();
        return;
    }
    for(int i=0; i<file_to_copy.size();i++) {
        pos=0;
        string name="";
        for(int j=0; j<file_to_copy[i].size(); j++)
            if(file_to_copy[i][j]=='/') pos=j+1;
        name=file_to_copy[i].substr(pos, file_to_copy[i].size()-pos);
        struct stat temp_info;
        if(stat((absolute_path(file_to_copy[i])).c_str(), &temp_info)) {
            cout<<"  ** ERROR: "+file_to_copy[i]+" Doesn't exist. **    Press ENTER to continue"<<flush;
            sense();
            return;
        }
        if(S_ISDIR(temp_info.st_mode)) { // if directory is to be copied.
            if(command==absolute_path(file_to_copy[i])) {
                cout<<"  ** WARNING: ACTION ABORTED: You cannot copy a directory into itself **   Press ENTER to continue"<<flush;
                sense();
                return;
            }
            DIR *directory=opendir(absolute_path(file_to_copy[i]).c_str());
            if(directory) {
                struct dirent *entity;
                entity=readdir(directory);
                string files="";
                while(entity) {
                    if((string)entity->d_name!=".." && (string)entity->d_name!=".")
                        files+=absolute_path(file_to_copy[i])+"/"+entity->d_name + " ";
                    entity=readdir(directory);
                }
                create("create_dir "+name+" "+command);
                cpy("copy "+files+command+"/"+name, super);
                if(super)
                    delet("delete_dir "+absolute_path(file_to_copy[i]));
            }
        } else { // if file is to be copied.
            ifstream src((absolute_path(file_to_copy[i])).c_str(), ios::binary);
            ofstream dst((command+(string)"/"+name).c_str(), ios::binary);
            dst<<src.rdbuf();  
            chmod((command+(string)"/"+name).c_str(),temp_info.st_mode);
            src.close();
            dst.close();
            if(super)
                delet("delete_file "+absolute_path(file_to_copy[i]));
        }
    }
}
void manager(const char * dir) { //function to fetch details, call and manage other functions, and handel user interactions.
    DIR *directory=opendir(dir);
    if(directory) {
        struct dirent *entity;
        entity=readdir(directory);
        vector<string> files;
        while(entity) { //find name of files/ directories in present directory.
            files.push_back(entity->d_name);
            entity=readdir(directory);
        }
        sort(files.begin(), files.end());
        vector<array<string, 5>> detail(files.size(),{"","","","",""});
        struct stat info;
        for(int i=0; i<files.size(); i++) { //gather the details of files/ directories present.
            if(!stat(((string)dir+(string)"/"+files[i]).c_str(), &info)) {
                struct passwd *usr = getpwuid(info.st_uid);
                struct group  *grp = getgrgid(info.st_gid);
                time_t t = info.st_mtime;
                struct tm lt;
                localtime_r(&t, &lt);
                char time[100];
                strftime(time, sizeof(time), "%c", &lt);
                detail[i][0]=byte_to_XB(info.st_size);
                detail[i][1]=usr->pw_name;
                detail[i][2]=grp->gr_name;
                detail[i][3]=( (S_ISDIR(info.st_mode)) ? "d" : "-")+permission(info.st_mode);
                detail[i][4]=time;
            }
        }
        int cursor=0;
        windond=min(24,(int)files.size()-1);
        windost=0;
        while(1) { // this while loop handles the user interaction.
            render(dir,files,detail,cursor);
            if(mode==0) { // if in normal mode (mode==0)
                char c=sense(); // get user key press.
                if(c=='w') { // if key is up key.
                    cursor=max(cursor-1, 0);
                    if(cursor<windost) {
                        windost--;
                        windond--;
                    }
                } else if(c=='s') { // if key is down key.
                    cursor=min(cursor+1, (int)files.size()-1);
                    if(cursor>windond) {
                        windost++;
                        windond++;
                    }
                } else if(c=='a') { // if key is left arrow key.
                    if(prevs.size()>1) {
                        nxt.push(prevs.top());
                        prevs.pop();
                    }
                    return;
                } else if(c=='d') { // if key is right arrow key.
                    if(nxt.size()>0) {
                        prevs.push(nxt.top());
                        nxt.pop();
                    }
                    return;
                } else if((int)c==13||(int)c==10||(int)c==127) {// if key is either of ENTER or BACKSPACE.
                    if(files[cursor]==".."||(int)c==127) {
                        if((string)dir!="/")
                            prevs.push(parent((string)dir));
                    } else if(cursor!=0) {
                        if(detail[cursor][3][0]=='d') {
                            if(prevs.top()=="/")
                                prevs.push((string)dir+files[cursor]);
                            else
                            prevs.push((string)dir+(string)"/"+files[cursor]);
                        } else {
                            if (!fork()) { // to open a file in default application.
                                execl("/usr/bin/xdg-open", "xdg-open", (prevs.top()+"/"+files[cursor]).c_str(), NULL);
                            }
                        }
                    }
                    return;
                } else if(c=='h') { // if key pressed is 'h'.
                    prevs.push("/home/"+(string)getlogin());
                    return;
                } else if(c==':') { // if key pressed is ':'.
                    mode=1;
                    return;
                } else if(c=='q') { // if key pressed is 'q'.
                    cout<<"\x1b[?25h"<<flush;
                    closedir(directory);
                    exit(0);
                }
            } else { // if in command mode.
                char c=sense();
                string command="";
                while(c!='\n') { // this while loop constructs the command given by the user by processing the key presses.
                    if(c=='^') return;
                    if((int)c==27) { // if Esc is pressed return to normal mode.
                        mode=0;
                        return;
                    }
                    if((int)c==127 && command.size()>0)
                    {
                        command.pop_back();
                    }
                    else
                        command+=c;
                    render(dir,files,detail,cursor);
                    cout<<command<<flush;
                    c=sense();
                }
                if(command.substr(0,4)!="quit" && command.substr(0,4)!="copy" && command.substr(0,4)!="move" && command.substr(0,4)!="goto" &&
                    command.substr(0,6)!="search" && command.substr(0,6)!="rename" && command.substr(0,11)!="create_file" && 
                    command.substr(0,11)!="delete_file" && command.substr(0,10)!="create_dir" && command.substr(0, 10)!="delete_dir") {
                        // if the command is incorrect.
                    cout<<"  ** ERROR: Command not found. **    Press ENTER to continue"<<flush;
                    sense();
                } else if(command=="quit") { // to quit the program.
                    cout<<" "<<flush;
                    exit(0);
                } else if(command.substr(0,4)=="goto") { // to go to a directory. 
                    command=command.substr(5, command.size()-5);
                    command=absolute_path(command);
                    if(command!=prevs.top())
                        prevs.push(command);
                    return ;
                } else if(command.substr(0,6)=="search") { // search files/ directory using the above implemented search function.
                    command=command.substr(7, command.size()-7);
                    render(dir,files,detail,cursor);
                    cout<<((search(dir, command))? "True":"False")<<"    Press any key."<<flush;
                    char c=sense();
                } else if(command.substr(0, 4)=="copy") { // to copy/ mode files and directories.
                    cpy(command, false);  // cpy() works as copy if second parameter is false and move when second parameter is true.
                } else if(command.substr(0,6)=="delete") { // to delete.
                    delet(command);
                    return;
                } else if(command.substr(0,6)=="rename") { // renaming files.
                    string old_name="";
                    command=command.substr(7, command.size()-7);
                    int i=0;
                    while(command[i]!=' ') {
                        old_name+=command[i];
                        i++;
                    }
                    command=command.substr(i+1,command.size()-i-1);
                    for(string str :files)
                        if(str==command) {
                            cout<<"  ** ERROR: "+command+" already exists in destination. **    Press ENTER to continue"<<flush;
                            sense();
                            return;
                        }
                    ifstream src((prevs.top()+((prevs.top()=="/")?"":"/")+old_name).c_str(), ios::binary);
                    ofstream dst((prevs.top()+((prevs.top()=="/")?"":"/")+command).c_str(), ios::binary);
                    dst<<src.rdbuf();  
                    struct stat temp_info;
                    stat((prevs.top()+((prevs.top()=="/")?"":"/")+old_name).c_str(), &temp_info);
                    chmod((prevs.top()+((prevs.top()=="/")?"":"/")+command).c_str(),temp_info.st_mode);
                    src.close();
                    dst.close();
                    delet("delete_file "+old_name);
                    return;
                } else if(command.substr(0, 6)=="create") { // to create file/ directory.
                    create(command);
                    return;
                } else if(command.substr(0, 4)=="move") { // to move file/ directory using cpy() function passing second parameter as true.
                    cpy("copy"+command.substr(4, command.size()-4), true);
                    return;
                }
            }
        }   
    }
    cout<<" Can't open this directory: Access denied     Press ENTER to continue"<<flush;
    sense();
    prevs.pop();
    closedir(directory);
    return;
}
int main() {
    en_raw();
    char temp[256];
    getcwd(temp, 256);
    prevs.push((string)temp);
    while(1) {
        manager(prevs.top().c_str());
    }
    return 0;
}