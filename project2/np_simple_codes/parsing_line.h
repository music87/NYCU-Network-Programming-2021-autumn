#ifndef parsing_line_h
#define parsing_line_h
#include<iostream>
#include<sstream>
#include<vector>
#include<cstring>
#include<regex>
#include"../np_utils/units.h"
using namespace std;

vector<cmd_unit> parsing(string &line){
    if(line.find("\r")!=string::npos)
        line.replace(line.find("\r"), sizeof("\r")-1, "");
    istringstream strin(line);
    string *word_ptr,*c_ptr=new string;
    vector<const char*> argv;
    vector<cmd_unit> cmd_group;
    regex numbered_pipe_stdout("\\|[0-9]+");
    regex numbered_pipe_stdout_stderr("![0-9]+");
    //regex redirect("[a-z]+\\.[a-z]+");
    while(true){
        word_ptr = new string;
        strin >> (*word_ptr);
        if(argv.size() == 0){
            // start to construct one cmd_unit
            c_ptr->assign(*word_ptr);
        }
        if((*word_ptr) == "|" || regex_match((*word_ptr), numbered_pipe_stdout) || regex_match((*word_ptr), numbered_pipe_stdout_stderr) || (*word_ptr) == ">"){
            // finish constructing one regular cmd_unit
            argv.push_back(nullptr);
            cmd_group.push_back(cmd_unit((*c_ptr),argv));
            if ((*word_ptr) == ">"){
                // redirect is also a cmd_unit
                argv.clear();
                c_ptr = new string(*word_ptr); argv.push_back(word_ptr->c_str()); // ">"
                word_ptr = new string; strin >> (*word_ptr); argv.push_back(word_ptr->c_str());// "file.txt"
                cmd_group.push_back(cmd_unit((*c_ptr), argv));
            } else {
                // pipe family are also a cmd_unit
                cmd_group.push_back(cmd_unit((*word_ptr)));
            }
            argv.clear(); c_ptr = new string;
        } else if((*word_ptr) != ""){
            // continue constructing one regular cmd_unit
            // arguments should involve all terms(include the first term) of one regular cmd_unit
            argv.push_back(word_ptr->c_str());
        }
    if(strin.eof()){
           // pick up the last valid cmd_unit and then break
            if ((*c_ptr) != ""){
                argv.push_back(NULL);
                cmd_group.push_back(cmd_unit((*c_ptr),argv));
       }
            break;
    }
    }
    return cmd_group;

}



#endif /* parsing_line_h */
