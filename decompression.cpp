#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <filesystem>
#include <fstream>

using namespace std;
#define fs filesystem
u_char check=128;
//Error codes
#define DOESNT_EXIST -100
#define EMPTY_PASSWORD -200
#define LONG_PASSWORD -300
#define STD_ERROR -400
#define WRONG_EXTENSION -500
#define WRONG_PASSWORD -600

// Node for binary tree(heap)
struct node{
    node *right=NULL, *left=NULL;
    u_char ch;
    node(node *a, node *b){
        right=a,left=b;
    }
};

// Converting 8 bits number to a character
u_char byteToDecimal(int curBitCount, u_char &curByte, ifstream &cmpFile){
    u_char tmpByte,val;
    tmpByte=u_char(cmpFile.get());
    //
    val=(curByte)|(tmpByte>>curBitCount);
    curByte=tmpByte<<8-curBitCount;
    return val;
}

// Generating Huffman Codes from the compressed file
void generateCode(int &curBitCount,int len,u_char &curByte,node *root,u_char curChar, ifstream &cmpFile){
     
    for(int i=0;i<len;i++){
        if(curBitCount==0){
            curByte=cmpFile.get();
            curBitCount=8;
        }

        if(!(curByte&check)){
            if((root->left)==NULL) root->left= new node(NULL,NULL);
            root=root->left;
        }else if((curByte&check)){
            if((root->right)==NULL) root->right= new node(NULL,NULL);
            root=root->right;
        }
        (curByte)<<=1;
        (curBitCount)--;
    }
    root->ch=curChar;
}
// Destructor for binary tree
void destroyTree(node *root){        
    if(root->left)destroyTree(root->left);
    if(root->right)destroyTree(root->right);
    delete root;
}
// Main function
int decompress(string path,string extension=".cmp",string pwd=""){
    ifstream cmpFile(path,ios_base::binary); // Input stream
    ofstream file; // Output stream
    string nfile="New-"; // New file addition
    int pwd_len=0,letter_cnt=0;

    // Checking if file exists
    if(!fs::exists(fs::path(path))){
        return DOESNT_EXIST;
    }
    // Checking if file has compression extension
    if((fs::path(path).extension().string())!=extension){
        cmpFile.close();
        return WRONG_EXTENSION;
    }

    // Getting the original file size
    long multiplier=1;
    long size=0;
    for(int i=0;i<8;i++){
        size+=u_char(cmpFile.get())*multiplier;
        multiplier*=256;
    }
    //Creating new file's path
    string tpath=fs::path(path).parent_path();
    path=fs::path(path).filename().stem().string();
    path=tpath+'/'+nfile+path;

    // Getting alphabet size
    letter_cnt=cmpFile.get();
    if(letter_cnt==0) letter_cnt=256;

    // Checking if password exists and if it exists it's size
    pwd_len=cmpFile.get();
    
    // Checking if password is valid
    if(pwd_len){
        char realPwd[pwd_len+1];
        cmpFile.read(realPwd,pwd_len);
        string pwd_str;
        for(int i=0;i<pwd_len;i++){
            pwd_str+=realPwd[i];
        }
        if(pwd_str!=pwd){
            cmpFile.close();
            return WRONG_PASSWORD;
        }
    }
    
    u_char curChar,curByte=0;
    int len=0,curBitCount=0;
    node *root = new node(NULL,NULL); // Binary tree root
    
    node *ptr=root;
    // Creating Huffman tree
    for(int i=0;i<letter_cnt;i++){
        
        
        curChar=byteToDecimal(curBitCount,curByte,cmpFile);
        
       
        
        len=byteToDecimal(curBitCount,curByte,cmpFile);
        if(len==0) len=256;
        
        
        
        generateCode(curBitCount,len,curByte,root,curChar,cmpFile);
        
        
    }
    root=ptr;

    file.open(path,ios_base::binary);
    node *nd=nullptr;
    // Translating encoded text to readable text
    for(long i=0;i<size;i++){
        nd=root;
        while((nd->left)||(nd->right)){
            if(curBitCount==0){
                curByte=cmpFile.get();
                curBitCount=8;
            }
            
            if(curByte&check){
                nd=nd->right;
            }else{
                nd=nd->left;
            }
            curBitCount--;
            curByte<<=1;
            
        }
        
        file.put(nd->ch);
    }

    // Closing file streams
    file.close();
    cmpFile.close();

    //Destroying tree to free memory
    destroyTree(root);
    return 0;
}