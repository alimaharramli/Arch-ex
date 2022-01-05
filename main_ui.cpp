#include <gtkmm.h>
#include <gtk/gtk.h>
#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <filesystem>
#include <fstream>
#include "comp_functions.hpp"
using namespace std;

#define DOESNT_EXIST -100
#define EMPTY_PASSWORD -200
#define LONG_PASSWORD -300
#define STD_ERROR -400
#define WRONG_EXTENSION -500
#define WRONG_PASSWORD -600

class ModelColumns : public Gtk::TreeModel::ColumnRecord
  {
  public:

    ModelColumns()
    { 
    add(m_col_id); 
    add(m_col_filename);
    add(m_col_filepath);
    add(m_col_filesize);
    }
    Gtk::TreeModelColumn<unsigned int> m_col_id;
    Gtk::TreeModelColumn<Glib::ustring> m_col_filename;
    Gtk::TreeModelColumn<Glib::ustring> m_col_filepath;
    Gtk::TreeModelColumn<unsigned int> m_col_filesize ;
  };
static void addFiles(Gtk::Window *window,ModelColumns *m_columns,Glib::RefPtr<Gtk::ListStore> *treemodel,vector<string> *filenames){
    auto dia = Gtk::FileChooserDialog(*window,"File chooser",Gtk::FILE_CHOOSER_ACTION_OPEN);
    dia.add_button("_Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    dia.add_button("_Open", Gtk::ResponseType::RESPONSE_OK);
    dia.set_select_multiple(true);
    auto ans=dia.run();
    
    if(ans==-5){
        auto files=dia.get_filenames();
        auto children = (*treemodel)->children();
        string er_fnames;
        unsigned int er_cnt=0;
        for(int i=0;i<files.size();i++){
            filesystem::path p = files[i];
            if(filesystem::is_directory(p)){
                if(er_fnames.empty()) er_fnames+=string(files[i]);
                else er_fnames+=", "+string(files[i]);
                er_cnt++;
                continue;
            }
            int flag=0;
            for(auto j:children){
                if(j.get_value((*m_columns).m_col_filename)==string(p.filename())
                    && j.get_value((*m_columns).m_col_filepath)==string(p.parent_path())){
                        flag=1;
                        break;
                    }
            }
            if(flag) continue;
            Gtk::TreeModel::Row row = *((*treemodel)->append());
            row[(*m_columns).m_col_id] = 0;
            row[(*m_columns).m_col_filename] = string(p.filename());
            row[(*m_columns).m_col_filepath] = string(p.parent_path());
            row[(*m_columns).m_col_filesize] = filesystem::file_size(p);
            
            (*filenames).push_back(files[i]);

            
        }
        if(er_cnt){
            if(er_cnt==1) er_fnames="The file \""+er_fnames+"\" is a folder!\n Please choose a valid file.";
            else er_fnames="The files \""+er_fnames+"\" are folders!\n Please choose valid files.";
            auto erdia = Gtk::MessageDialog(*window,er_fnames,false,Gtk::MessageType::MESSAGE_ERROR);
            erdia.run();
        }
    }
    auto children = (*treemodel)->children();
    unsigned int cnt=1;
    for(auto fil : children){
        fil->set_value((*m_columns).m_col_id,cnt++);
    }
}
static void removeFiles(Gtk::TreeView *tr,ModelColumns *m_columns, Glib::RefPtr<Gtk::ListStore> *treemodel, vector<string> *filenames){
    auto sel = (*tr).get_selection()->get_selected_rows();
    while(!sel.empty()){
        if((*treemodel)->iter_is_valid((*treemodel)->get_iter(*sel.rbegin())))
            (*treemodel)->erase((*treemodel)->get_iter(*sel.rbegin()));
        sel.pop_back();
    }

    auto children = (*treemodel)->children();
    unsigned int cnt=1;
    for(auto fil : children){
        fil->set_value((*m_columns).m_col_id,cnt++);
    }
    filenames->clear();
    for(auto j:(*treemodel)->children()){
        filenames->push_back(j.get_value((*m_columns).m_col_filepath)+"/"+j.get_value((*m_columns).m_col_filename));
    }

}

static void aboutDialog(){
    auto pix = Gdk::Pixbuf::create_from_file("archive.png",80,80);
    auto dia = Gtk::AboutDialog();
    dia.set_program_name("Arch(ex) - Archive and Extract");
    dia.set_title("Arch(ex) - About");
    dia.set_version("Arch(ex) - 0.1");
    dia.set_copyright("(c) Ali Maharramli");
    dia.set_comments("Arch(ex) is a simple tool for compression");
    dia.set_logo(pix);
    dia.run();
}

static void clearFiles(Gtk::TreeView *tr,ModelColumns *m_columns, Glib::RefPtr<Gtk::ListStore> *treemodel, vector<string> *filenames){
    auto children = (*treemodel)->children();
    unsigned int cnt=1;
    while(!children.empty()){
        (*treemodel)->erase(children.begin());
    }
    filenames->clear();
}

static void disableEntry(Gtk::CheckButton *ch, Gtk::Entry *ent){
    if((*ch).get_active())
        (*ent).set_sensitive(true);
    else 
        (*ent).set_sensitive(false);
}

static void disablePw(Gtk::CheckButton *ch, Gtk::Entry *ent, Gtk::CheckButton *show_hide){
    if((*ch).get_active()){
        (*ent).set_sensitive(true);
        (*show_hide).set_sensitive(true);
    }else{
        (*ent).set_sensitive(false);
        (*show_hide).set_sensitive(false);
    }
}

static void showHide(Gtk::CheckButton *show_hide, Gtk::Entry *ent){
    if((*show_hide).get_active()){
        (*ent).set_visibility(true);
    }else{
        (*ent).set_visibility(false);
    }
}

static void compressButton(Gtk::Window *window,Gtk::Entry *ent,Gtk::Entry *pwd, vector<string> *filenames){
    string ext=".cmp";
    int pwCheck=0;
    string pw="";
    if((*ent).get_sensitive())
        ext=(*ent).get_text();
    if((*pwd).get_sensitive()){
        pwCheck=1;
        pw=(*pwd).get_text();
    }
    for(auto i:(*filenames)){
        auto res=compress(i,ext,pwCheck,pw);
        string Msg;
        switch(res){
            case DOESNT_EXIST:
                Msg="File "+i+" doesn't exist.";
            break;
            case EMPTY_PASSWORD:
                Msg="Password is empty";
            break;
            case LONG_PASSWORD:
                Msg="Password cannot be longer than 60 characters.";
            break;
            case STD_ERROR:
                Msg="Compression failed.";
            break;
        }
        if(res<0){
            auto erdia = Gtk::MessageDialog(*window,Msg,false,Gtk::MessageType::MESSAGE_ERROR);
            erdia.run();
        }else{
            int sz1=filesystem::file_size(filesystem::path(i));
            double ptage= 100-double(res)/double(sz1)*100;
            if(sz1<res) Msg="Unfortunately compressed file's size is higher than original file.(%"+to_string(ptage)+")";
            else Msg="Success!!!\nYou have saved %"+to_string(ptage)+"("+to_string(sz1-res)+" bytes). ";
            auto msgdia = Gtk::MessageDialog(*window,Msg,false,Gtk::MessageType::MESSAGE_INFO);
            msgdia.run();
        }
    }
}
static void decompressButton(Gtk::Window *window, Gtk::Entry *ent,Gtk::Entry *pwd, vector<string> *filenames){
    string ext=".cmp";
    string pw="";
    if((*ent).get_sensitive())
        ext=(*ent).get_text();
    if((*pwd).get_sensitive())
        pw=(*pwd).get_text();
    for(auto i:(*filenames)){
        auto res=decompress(i,ext,pw);
        string Msg;
        switch(res){
            case DOESNT_EXIST:
                Msg="File "+i+" doesn't exist.";
            break;
            case EMPTY_PASSWORD:
                Msg="Password is empty";
            break;
            case LONG_PASSWORD:
                Msg="Password cannot be longer than 60 characters.";
            break;
            case STD_ERROR:
                Msg="Decompression failed.";
            break;
            case WRONG_EXTENSION:
                Msg="File "+i+" doesn't have the extension "+ext+'.';
            break; 
            case WRONG_PASSWORD:
                Msg="Your password isn't correct.\nPlease try again";
            break;
        }
        if(res<0){
            auto erdia = Gtk::MessageDialog(*window,Msg,false,Gtk::MessageType::MESSAGE_ERROR);
            erdia.run();
        }else{
            Msg="Success!!!\nThe decompressed file is located at "+filesystem::path(i).parent_path().string()+"/New-"+filesystem::path(i).filename().stem().string();
            auto msgdia = Gtk::MessageDialog(*window,Msg,false,Gtk::MessageType::MESSAGE_INFO);
            msgdia.run();
        }
    }
}

int main(int argc, char **argv){
    auto app = Gtk::Application::create(argc, argv,"thun.archex");
    Gtk::ApplicationWindow window;
    window.set_title("Arch(Ex)");
    Gtk::Box box(Gtk::Orientation::ORIENTATION_VERTICAL);
    window.add(box);
    window.set_default_size(500,400);
    Gtk::Button button("Add file(s)");

    Gtk::MenuBar bar;
    Gtk::Menu file;
    Gtk::MenuItem menu_item("File");
    menu_item.set_submenu(file);
    gtk_menu_shell_append(GTK_MENU_SHELL(bar.gobj()),GTK_WIDGET(menu_item.gobj()));
    
    auto file_item = Gtk::MenuItem("Add File(s)");
    gtk_menu_shell_append(GTK_MENU_SHELL(file.gobj()),GTK_WIDGET(file_item.gobj()));
    Gtk::Menu help;
    menu_item = Gtk::MenuItem("Help");
    menu_item.set_submenu(help);
    gtk_menu_shell_append(GTK_MENU_SHELL(bar.gobj()),GTK_WIDGET(menu_item.gobj()));
    
    auto help_item = Gtk::MenuItem("About");
    gtk_menu_shell_append(GTK_MENU_SHELL(help.gobj()),GTK_WIDGET(help_item.gobj()));

    box.pack_start(bar,0,0,0);
    
    auto fbut = Gtk::FileChooserDialog("Choose file(s)");
    vector<string> files;
    auto tr = Gtk::TreeView();

    ModelColumns m_columns;
    auto treemodel = Gtk::ListStore::create(m_columns);
    tr.get_selection()->set_mode(Gtk::SelectionMode::SELECTION_MULTIPLE);
    tr.set_model(treemodel);
    tr.append_column("ID",m_columns.m_col_id);
    tr.append_column("Filename",m_columns.m_col_filename);
    tr.append_column("File path",m_columns.m_col_filepath);
    tr.append_column("Size",m_columns.m_col_filesize);

    button.signal_clicked().connect(sigc::bind(sigc::ptr_fun(&addFiles),&window,&m_columns,&treemodel,&files));
    file_item.signal_activate().connect(sigc::bind(sigc::ptr_fun(&addFiles),&window,&m_columns,&treemodel,&files));
    help_item.signal_activate().connect(sigc::ptr_fun(&aboutDialog));
    
    auto box5 = Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL);
    auto box2 = Gtk::Box(Gtk::Orientation::ORIENTATION_HORIZONTAL);
    Gtk::Button but2("Remove file(s)");
    Gtk::Button but6("Clear");
    but2.signal_clicked().connect(sigc::bind(sigc::ptr_fun(&removeFiles),&tr,&m_columns,&treemodel,&files));
    but6.signal_clicked().connect(sigc::bind(sigc::ptr_fun(&clearFiles),&tr,&m_columns,&treemodel,&files));
    box5.pack_start(button);
    box5.pack_start(but2);
    box5.pack_start(but6);
    box2.pack_start(tr);
    box2.pack_start(box5,0,0,0);
    box.pack_start(box2);
    auto box3 = Gtk::Box(Gtk::Orientation::ORIENTATION_HORIZONTAL);
    Gtk::Button but3("Decompress");
    Gtk::Button but4("Compress");
    box3.pack_start(but3);
    box3.pack_start(but4);
    auto ent = Gtk::Entry();
    ent.set_sensitive(false);
    auto box4 = Gtk::Box(Gtk::Orientation::ORIENTATION_HORIZONTAL);
    auto ch = Gtk::CheckButton("Custom file extension");
    ch.set_name("Custom file extension");
    ch.signal_toggled().connect(sigc::bind(sigc::ptr_fun(&disableEntry),&ch,&ent));
    ent.set_placeholder_text("Default: .cmp ");
    box4.pack_start(ch,0,0,0);
    box4.pack_start(ent);
    
    auto ent2 = Gtk::Entry();
    ent2.set_sensitive(false);
    auto box6 = Gtk::Box(Gtk::Orientation::ORIENTATION_HORIZONTAL);
    auto ch2 = Gtk::CheckButton("Password");
    auto show_hide = Gtk::CheckButton("Show/Hide");
    ch2.set_name("Password");
    ch2.signal_toggled().connect(sigc::bind(sigc::ptr_fun(&disablePw),&ch2,&ent2,&show_hide));
    ent2.set_visibility(false);

    show_hide.set_name("Show/Hide");
    show_hide.signal_toggled().connect(sigc::bind(sigc::ptr_fun(&showHide),&show_hide,&ent2));
    show_hide.set_sensitive(false);
    but4.signal_clicked().connect(sigc::bind(sigc::ptr_fun(&compressButton),&window,&ent,&ent2,&files));
    but3.signal_clicked().connect(sigc::bind(sigc::ptr_fun(&decompressButton),&window,&ent,&ent2,&files));

    box6.pack_start(ch2,0,0,0);
    box6.pack_start(ent2);
    box6.pack_start(show_hide);


    box.pack_start(box6,0,0,0);
    box.pack_start(box4,0,0,0);
    box.pack_start(box3,0,0,0);
    window.show_all_children();
    return app->run(window);
}
