#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <dirent.h>

using namespace std;
#define fs filesystem

//Error codes
#define DOESNT_EXIST -100
#define EMPTY_PASSWORD -200
#define LONG_PASSWORD -300
#define STD_ERROR -400

// It is not possible to write bit by bit to files
// so we can mix two chars to get a complete byte
void writeuChar(u_char uChar,int bitCount,u_char &curByte,ofstream *cmpFile){
    curByte<<=(8-bitCount);
    curByte|=(uChar>>bitCount);
    cmpFile->put(curByte);
    curByte=uChar;   
}
// Binary tree(heap) node
struct node{
    node *left, *right;
    long num; string bit;
    u_char ch;
};
// Binary tree constructor
void buildTree(node *arr,int letter_cnt){
    node *cur=arr+letter_cnt,*lf=arr+2,*notlf=arr+letter_cnt;
    node *mi1=arr,*mi2=arr+1;
    for(uint i=0;i<letter_cnt-1;i++){
        cur->num = (mi1 -> num) + (mi2 -> num);
        cur->left = mi1;
        cur->right = mi2;
        cur++;
        mi1->bit="1"; mi2->bit="0";

        if(lf>=arr+letter_cnt){
            mi1=notlf;
            notlf++;
        }else{
            if( (lf -> num) < (notlf -> num)){
                mi1=lf;
                lf++;
            }else{
                mi1=notlf;
                notlf++;
            }
        }

        if(lf>=arr+letter_cnt){
            mi2=notlf;
            notlf++;
        }else if(notlf >= cur){
            mi2=lf;
            lf++;
        }else{
            if( (lf -> num) < (notlf -> num) ){
                mi2=lf;
                lf++;
            }else{
                mi2=notlf;
                notlf++;
            }
        }

    }
}
// Sorting comparator
bool nodecmp(node a, node b){
    return a.num<b.num;
}
// Main function
int compress(string path,string extension=".cmp",int checkpwd=0,string pwd="",int check=1){
    long bits=0,tot_bits=0;
    int letter_cnt=0;
    long num[256];

    fill(num,num+256,0);
    
    long size=-1;
    // If file doesn't exist, return an error code
    if(!fs::exists(fs::path(path)))
        return DOESNT_EXIST;
    else size=fs::file_size(fs::path(path));
    
    // Input file stream for reading binary files
    ifstream file(path,ios_base::binary);

    // Calculating the frequency of bytes
    for(long i=0;i<size;i++){
        num[file.get()]++;
    }

    // Filename for compressed file
    string cmp_fname=path+extension;

    // Calculating alphabet size
    for(auto i=num;i<num+256;i++){
        if(*i){
            letter_cnt++;
        }
    }
    
    
    node arr[letter_cnt*2-1];
    node *n_ptr=arr;
    for(auto i=num;i<num+256;i++){
        if(*i){
            n_ptr->right=NULL;
            n_ptr->left=NULL;
            n_ptr->num=*i;
            n_ptr->ch=i-num;
            n_ptr++;
        }
    }

    sort(arr,arr+letter_cnt,nodecmp);

    buildTree(arr,letter_cnt);

    // Building huffman codes for the characters
    for(n_ptr=arr+letter_cnt*2-2;n_ptr>arr-1;n_ptr--){
        if(n_ptr->right){
            n_ptr->right->bit = (n_ptr->bit) + (n_ptr->right)->bit;
        }
        if(n_ptr->left){
            n_ptr->left->bit = (n_ptr->bit) + (n_ptr->left)->bit; 
        }
    }
    
    // Output file stream for writing to binary files
    ofstream cmpFile(cmp_fname,ios_base::binary);
    
    u_char tmp;
    long tmpSize=size;
    tot_bits+=64;
    // Writing original file size to the first 8 bytes
    for(uint i=0;i<8;i++){
        tmp=tmpSize%256;
        tmpSize/=256;
        cmpFile.put(tmp);
    }

    // Next byte will have the alphabet size
    cmpFile.put(letter_cnt);
    tot_bits+=8;

    // Checking if a password is given or not
    // If there is a password one byte will be for password_length 
    // and another few bytes depending on password
    if(checkpwd){
        int pwd_len=pwd.length();
        if(pwd_len==0){
            cmpFile.close();
            file.close();
            fs::remove(fs::path(cmp_fname));
            return EMPTY_PASSWORD;
        }else if(pwd_len>60){
            cmpFile.close();
            file.close();
            fs::remove(fs::path(cmp_fname));
            return LONG_PASSWORD;
        }
        cmpFile.put(u_char(pwd_len));
        cmpFile.write(pwd.c_str(),pwd_len);
        tot_bits+=8+8*pwd_len;
    }else{
        cmpFile.put(checkpwd);
        tot_bits+=8;
    }

    u_char curByte,len,curChar;
    int curBitCount=0;
    string arrstr[256];
    char *sptr;

    // Writing Huffman coding tree to the file for decompression
    for(n_ptr=arr;n_ptr<arr+letter_cnt;n_ptr++){
        curChar=n_ptr->ch;
        len=n_ptr->bit.length();
        arrstr[curChar]=n_ptr->bit;
        
        
        writeuChar(curChar,curBitCount,curByte,&cmpFile);
        writeuChar(len,curBitCount,curByte,&cmpFile);
        tot_bits+=len+16;
        sptr=&(n_ptr->bit[0]);
        while(*sptr){
            
            if(curBitCount==8){
                cmpFile.put(curByte);
                curBitCount=0;
            }
            if((*sptr)=='1'){
                curByte<<=1;
                curByte|=1;
                curBitCount++;
            }else if((*sptr)=='0'){
                curByte<<=1;
                curBitCount++;
            }else{
                cmpFile.close();
                file.close();
                fs::remove(fs::path(cmp_fname));
                return STD_ERROR;
            }
            
            sptr++;
        }
        
        bits+=n_ptr->num * len;
    }

    tot_bits+=bits;
    u_char lastByte=tot_bits%8;
    (lastByte)?tot_bits=(tot_bits/8+1)*8:tot_bits=tot_bits;
    file.clear();
    file.seekg(0);// Return to the beginning
    
    // Writing the original content to the file using Huffman codes
    for(long i=0;i<bits;){
        u_char x=file.get();
        sptr=&(arrstr[x][0]);
        
        
        while(*sptr){
            if(curBitCount==8){
                cmpFile.put(curByte);
                curBitCount=0;
            }   
            if((*sptr)=='1'){
                i++;
                curByte<<=1;
                curByte|=1;
                curBitCount++;
            }else if((*sptr)=='0'){
                i++;
                curByte<<=1;
                curBitCount++;
            }
            sptr++;
        }
        
    }
    // Checking if we should merge last character or write it seperately
    if(curBitCount==8){
        cmpFile.put(curByte);
    }else{
        curByte<<=(8-curBitCount);
        cmpFile.put(curByte);
    }

    // Closing file streams
    file.close();
    cmpFile.close();

    // Returning encoded file's size
    return tot_bits/8;
}
