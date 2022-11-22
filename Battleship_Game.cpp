#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <utility>
#include <string>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/wait.h>
using namespace std;
typedef pair<int,int> Point;
class Data{
public:
	Point point;
	int turnNum;
	bool childPrint=false;
	bool parentPrint=false;
	int winBomb;
	int childPid;
};
pair<Point,Point> setGunboat(int randSeed){
	srand(randSeed);
	int x=rand()%4,y=rand()%4,randNum=rand()%4;//得到第一個隨機位置xy和隨機變數
	//總共4種情況會出界
	int move[4][2]={{1,0},{0,1},{-1,0},{0,-1}},dx,dy;
	if((x>2&&randNum==0)||(y>2&&randNum==1)||(x<1&&randNum==2)||(y<1&&randNum==3)){
		randNum=(randNum+2)%4;
	}
	dx=x+move[randNum][0],dy=y+move[randNum][1];//第二個位置
	Point p1(x,y),p2(dx,dy);
	return pair<Point,Point>(p1,p2);
}
void printGunboat(int pid,pair<Point,Point> p){
	if(pid==0){
		cout<<"["<<getpid()<<" Child]:The gunboat: ";
	}
	else{
		cout<<"["<<getpid()<<" Parent]:The gunboat: ";
	}
	cout<<"("<<p.first.first<<","<<p.first.second<<")("<<p.second.first<<","<<p.second.second<<")\n";
}
Point setBomb(int& randSeed){
	srand(randSeed);
	int x=rand()%4,y=rand()%4;//隨機炸
	randSeed++;
	return Point(x,y);
}
void printBomb(int pid,Point p){
	if(pid==0){
		cout<<"["<<getpid()<<" Child]:bombing: ";
	}
	else{
		cout<<"["<<getpid()<<" Parent]:bombing: ";
	}
	cout<<"("<<p.first<<","<<p.second<<")\n";
}
int main(void){
	pid_t pid;//process id;
	int i=100;
	int parentSeed,childSeed;
	cout<<"請輸入Parent和Child的亂數種子和0(基本功能)\n";
	cin>>parentSeed>>childSeed;//輸入亂數種子
	int mode;
	cin>>mode;
	pid = fork();	
	int sharedSpace=shm_open("space",O_CREAT|O_TRUNC|O_RDWR,0666);//設置shared memory
	size_t regionSize=sizeof(Data);
	int r=ftruncate(sharedSpace,regionSize);
	if(r==-1){
		perror("size error");
		exit(1);
	}
	Data* position=(Data*)mmap(NULL,sizeof(class Data),PROT_READ|PROT_WRITE,MAP_SHARED,sharedSpace,0);//在shared memory裡放Data
	position->turnNum=0;
	int childBombNum=0;
	int parentBombNum=0;
	bool isParentWin=true;
	if (pid < 0) { /* error occurred */
		fprintf(stderr, "Fork Failed");
		exit(-1);
	}
	else if (pid == 0) { /* child process */
		cout<<"["<<getpid()<<" Child]:Random Seed "<<childSeed<<endl;
		srand(childSeed);
		i = (rand() % 10) +1;
		pair<bool,bool> HP(true,true);
		Point bombPosition;
		pair<Point,Point> gunboatPosition;
		position->childPid=getpid();
		while(true){
			if(position->turnNum==1&&!position->childPrint){
				gunboatPosition=setGunboat(childSeed);//設砲艇位置
				printGunboat(pid,gunboatPosition);//印砲艇位置		
				position->turnNum--;
				position->childPrint=true;
				break;
			}
		}
		bool change=false;
		while(true){
			if(position->turnNum==1){
				if(change){
					change=false;
					if(gunboatPosition.first==bombPosition){
						HP.first=false;//第一個位置被擊中
						if(!HP.first&&!HP.second){//兩個都被擊中
							cout<<"["<<getpid()<<" Child]: "<<"hit and sinking\n";
							isParentWin=true;
							position->winBomb=childBombNum;
							break;
						}
						else{
							cout<<"["<<getpid()<<" Child]: "<<"hit\n";
						}
					}
					else if(gunboatPosition.second==bombPosition){
						HP.second=false;//第二個位置被擊中
						if(!HP.first&&!HP.second){//兩個都被擊中
							cout<<"["<<getpid()<<" Child]: "<<"hit and sinking\n";
							isParentWin=true;
							position->winBomb=childBombNum;
							break;
						}
						else{
							cout<<"["<<getpid()<<" Child]: "<<"hit\n";
						}
					}
					else{ 
						cout<<"["<<getpid()<<" Child]: "<<"missed\n";
					}
					bombPosition=setBomb(childSeed);//設炸彈
					printBomb(pid,bombPosition);//印炸彈
					position->point=bombPosition;//傳炸彈位置進shared memory
					childBombNum++;
				}
				position->turnNum--;//換對方
				change=true;
			}
		}
		
	}
	else { /* parent process */
		/* parent will wait for the child to complete */
		cout<<"["<<getpid()<<" Parent]:Random Seed "<<parentSeed<<endl;
		srand(parentSeed);
		i = (rand() % 10) +1;
		pair<bool,bool> HP(true,true);
		Point bombPosition;
		pair<Point,Point> gunboatPosition;
		while(true){
			if(position->turnNum==0&&!position->parentPrint){
				gunboatPosition=setGunboat(parentSeed);//設砲艇位置
				printGunboat(pid,gunboatPosition);//印砲艇位置
				position->turnNum++;
				position->parentPrint=true;
				break;	
			}
		}
		bool change=false;
		while(true){
			if(position->turnNum==0){
				if(change){
					change=false;
					if(gunboatPosition.first==bombPosition){
						HP.first=false;//第一個位置被擊中
						if(!HP.first&&!HP.second){//兩個都被擊中
							cout<<"["<<getpid()<<" Parent]: "<<"hit and sinking\n";
							isParentWin=false;
							position->winBomb=parentBombNum;
							break;
						}
						else{//只擊中一個
							cout<<"["<<getpid()<<" Parent]: "<<"hit\n";
						}
					}
					else if(gunboatPosition.second==bombPosition){
						HP.second=false;//第二個位置被擊中
						if(!HP.first&&!HP.second){//兩個都被擊中
							cout<<"["<<getpid()<<" Parent]: "<<"hit and sinking\n";
							isParentWin=false;
							position->winBomb=parentBombNum;
							break;
						}
						else{//只擊中一個
							cout<<"["<<getpid()<<" Parent]: "<<"hit\n";
						}
					}
					else{	
						if(parentBombNum>0){
							cout<<"["<<getpid()<<" Parent]: "<<"missed\n";
						}
					}
					bombPosition=setBomb(parentSeed);//設炸彈
					printBomb(pid,bombPosition);//印炸彈
					position->point=bombPosition;//傳炸彈位置進shared memory
					parentBombNum++;
				}
				position->turnNum++;//換對方
				change=true;
			}
		}
		cout<<"["<<getpid()<<" Parent]: ";
		if(isParentWin){
			cout<<getpid()<<" wins with "<<(int)position->winBomb<<" bombs";
		}
		else{
			cout<<position->childPid<<" wins with "<<(int)position->winBomb<<" bombs";
		}
		exit(0);
	}
}
