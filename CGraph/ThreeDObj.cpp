#include "ThreeDObj.h"

#include<opencv2/highgui/highgui.hpp>

#include<cstdio>
#include<cstring>

#include<exception>
#include<queue>


#define MAXLINESIZE		64
#define MAXFILEPATHSIZE	128
#define MAXFILENAMESIZE 64


ThreeDObj::ThreeDObj(const char *objFilePath):
	graph(NULL), lapMtr(NULL), lapVal(NULL), geoDist(NULL), chosenGNum(-1)
{
	load(objFilePath);
}

ThreeDObj::~ThreeDObj(void)
{
	delete vertex;
	delete texCoord;
	delete vRenderIndex;
	delete tRenderIndex;
	delete gStartRIndex;
	
	delete graph;
	delete lapMtr;
	delete lapVal;
	delete geoDist;
}

void ThreeDObj::load(const char *objFilePath)
{
	FILE* objFile=fopen(objFilePath, "r");
	if(objFile==NULL)
	{
		throw exception("Wavefront Obj file opening failure.");
	}

	char lineBuf[MAXLINESIZE];

	//get mtl file name and then path
	char tempFileName[MAXFILENAMESIZE];
	while(true)
	{
		fgets(lineBuf, MAXLINESIZE, objFile);
		if(lineBuf[0]=='m')
		{
			sscanf(lineBuf, "mtllib %s", tempFileName);
			break;
		}
	}
	char tempFilePath[MAXFILEPATHSIZE];
	strcpy(tempFilePath, objFilePath);
	sprintf(strrchr(tempFilePath, '/')+1, "%s", tempFileName);

	//first pass: count v,vt,f,g
	vertexAmt=texCoordAmt=faceAmt=groupAmt=0;
	while(!feof(objFile))
	{
		fgets(lineBuf, MAXLINESIZE, objFile);

		switch(lineBuf[0])
		{
		case 'v':
			switch(lineBuf[1])
			{
			case ' ':
				vertexAmt++;
				break;

			case 't':
				texCoordAmt++;
				break;

			default:
				break;
			}
			break;

		case 'f':
			faceAmt++;
			break;

		case 'g':
			groupAmt++;
			break;

		default:
			break;
		}
	}
	
	vertex=new Vertex[vertexAmt+1];
	texCoord=new TexCoord[texCoordAmt+1];
	vRenderIndex=new int[faceAmt*3];
	tRenderIndex=new int[faceAmt*3];
	gStartRIndex=new int[groupAmt+1];

	//second pass: get v,vt,f,g data
	rewind(objFile);
	fgets(lineBuf, MAXLINESIZE, objFile);
	while(lineBuf[0]!='v')
	{
		fgets(lineBuf, MAXLINESIZE, objFile);
	}
		
	sscanf(lineBuf, "v %f %f %f\n", &vertex[1].x, &vertex[1].y, &vertex[1].z);
	for(int i=2;i<=vertexAmt;i++)
	{
		fscanf(objFile, "v %f %f %f\n", &vertex[i].x, &vertex[i].y, &vertex[i].z);
	}

	for(int i=1;i<=texCoordAmt;i++)
	{
		fscanf(objFile, "vt %f %f\n", &texCoord[i].u, &texCoord[i].v);
	}

	int rIndexI, temp;
	for(int faceI=0, groupI=0; faceI<faceAmt; )
	{
		fgets(lineBuf, MAXLINESIZE, objFile);
		switch(lineBuf[0])
		{
		case 'f':
			rIndexI=faceI*3;
			sscanf(lineBuf, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
				vRenderIndex+rIndexI,   tRenderIndex+rIndexI,   &temp,
				vRenderIndex+rIndexI+1, tRenderIndex+rIndexI+1, &temp,
				vRenderIndex+rIndexI+2, tRenderIndex+rIndexI+2, &temp);
			faceI++;
			break;

		case 'g':
			gStartRIndex[groupI++]=faceI*3;
			break;

		default:
			break;	
		}
	}
	gStartRIndex[groupAmt]=faceAmt*3;
	fclose(objFile);

	//read mtl file:
	FILE* mtlFile=fopen(tempFilePath, "r");
	if(mtlFile==NULL)
	{
		throw exception("mtl file opening failure.");
	}

	char tempStr[16];
	while(!feof(mtlFile))
	{
		fgets(lineBuf, MAXLINESIZE, mtlFile);
		sscanf(lineBuf, "%s", tempStr);
		switch(tempStr[0])
		{
		case 'm':
			sscanf(lineBuf, "%s %s", tempStr, tempFileName);
			break;

		default:
			break;
		}
	}
	sprintf(strrchr(tempFilePath, '/')+1, "%s", tempFileName);

	//load texture image:
	Mat tempImg=imread(tempFilePath);
	if(tempImg.empty())
	{
		throw exception("Texture image opening failure.");
	}
	texImg=Mat(tempImg.rows, tempImg.cols, tempImg.type());
	//imread will read image upside down, so we mend it handly:
	for(int rowI=0;rowI<tempImg.rows;rowI++)
	{
		for(int colI=0;colI<tempImg.cols;colI++)
		{
			texImg.at<cv::Vec3b>(rowI, colI)=tempImg.at<cv::Vec3b>(tempImg.rows-rowI-1, colI);
		}
	}
}

void ThreeDObj::buildGraph(void)
{
	if(graph!=NULL)
	{
		return ;
	}
	//only calculate when graph==NULL

	graph=new set<int>[vertexAmt+1];
	int index;
	for(int i=0;i<faceAmt;i++)
	{
		index=3*i;

		graph[vRenderIndex[index]].insert(vRenderIndex[index+1]);
		graph[vRenderIndex[index]].insert(vRenderIndex[index+2]);

		graph[vRenderIndex[index+1]].insert(vRenderIndex[index]);
		graph[vRenderIndex[index+1]].insert(vRenderIndex[index+2]);

		graph[vRenderIndex[index+2]].insert(vRenderIndex[index]);
		graph[vRenderIndex[index+2]].insert(vRenderIndex[index+1]);
	}
	/*for(int i=1;i<=vertexAmt;i++)
	{
		printf("%d:", i);
		for(set<int>::iterator iter=graph[i].begin();iter!=graph[i].end();iter++)
		{
			printf(" %d", *iter);
		}
		puts("");
	}*/
}

void ThreeDObj::computeLapMtr(void)
{
	if(lapMtr!=NULL)
	{
		return ;
	}
	//only calculate when lapMtr==NULL

	if(graph==NULL)
	{
		buildGraph();
	}

	lapMtr=new vector<MtrRow>[vertexAmt+1];
	float weight;
	set<int>::iterator connIter;
	MtrRow tempMtrRow;
	for(int vertexI=1;vertexI<=vertexAmt;vertexI++)
	{
		tempMtrRow.set(vertexI, 1.0f);
		lapMtr[vertexI].push_back(tempMtrRow);
		weight=1.0f/graph[vertexI].size();
		for(connIter=graph[vertexI].begin();connIter!=graph[vertexI].end();connIter++)
		{
			tempMtrRow.set(*connIter, -weight);
			lapMtr[vertexI].push_back(tempMtrRow);
		}
	}
	//for(int vertexI=1;vertexI<=vertexAmt;vertexI++)
	//{
	//	printf("No.%d row:", vertexI);
	//	for(vector<MtrRow>::iterator iter=lapMtr[vertexI].begin();iter!=lapMtr[vertexI].end();iter++)
	//	{
	//		printf(" %d(%f)", iter->index, iter->value);
	//	}
	//	puts("");
	//}
}

void ThreeDObj::calcLap(void)
{
	if(lapVal!=NULL)
	{
		return ;
	}
	//only calculate when lapVal==NULL:

	if(lapMtr==NULL)
	{
		computeLapMtr();
	}

	lapVal=new LaplacianVal[vertexAmt+1];	
	vector<MtrRow>::iterator mtrRowIter;
	for(int vertexI=1;vertexI<=vertexAmt;vertexI++)
	{
		lapVal[vertexI].x=lapVal[vertexI].y=lapVal[vertexI].z=0;
		for(mtrRowIter=lapMtr[vertexI].begin();mtrRowIter!=lapMtr[vertexI].end();mtrRowIter++)
		{
			lapVal[vertexI].x+=mtrRowIter->value*vertex[mtrRowIter->index].x;
			lapVal[vertexI].y+=mtrRowIter->value*vertex[mtrRowIter->index].y;
			lapVal[vertexI].z+=mtrRowIter->value*vertex[mtrRowIter->index].z;
		}
	}
	//for(int i=1;i<=vertexAmt;i++)
	//{
	//	printf("%d: %f %f %f\n", i, lapVal[i].x, lapVal[i].y, lapVal[i].z);
	//}
}

void ThreeDObj::setControlVIndex(const int groupNum)
{
	if(groupNum==chosenGNum)
	{
		return ;
	}
	//only calculate when chosenGNum has changed

	chosenGNum=groupNum;
	controlVIndex.clear();

	for(int i=gStartRIndex[chosenGNum];i<gStartRIndex[chosenGNum+1];i++)
	{
		controlVIndex.insert(vRenderIndex[i]);
	}
}

void ThreeDObj::calcGeoDist(void)
{
	static int lastGNum=-1;
	if(lastGNum==chosenGNum)
	{
		return ;
	}
	//only calculate when chosenGNum is different from last deformation

	//prepare:
	lastGNum=chosenGNum;
	if(geoDist==NULL)
	{
		geoDist=new int[vertexAmt+1];
	}
	if(graph==NULL)
	{
		buildGraph();
	}

	//turn chosen vertices into one vertex:
	set<int> *tempGraph=new set<int>[vertexAmt+1];
	set<int>::iterator connI;
	bool pass;
	int markV=vRenderIndex[gStartRIndex[chosenGNum]];
	for(int vertexI=1;vertexI<=vertexAmt;vertexI++)
	{
		if(controlVIndex.find(vertexI)==controlVIndex.end())
		{
			pass=false;
			for(connI=graph[vertexI].begin(); connI!=graph[vertexI].end(); connI++)
			{
				if(controlVIndex.find(*connI)!=controlVIndex.end())
				{
					if(!pass)
					{
						tempGraph[vertexI].insert(markV);
						tempGraph[markV].insert(vertexI);
						pass=true;
					}
				}
				else
				{
					tempGraph[vertexI].insert(*connI);
				}
			}
		}
	}

	//calculate unweighted shortest path from chosen to others:
	bool *hasCalced=new bool[vertexAmt+1];
	for(int i=1;i<=vertexAmt;i++)
	{
		geoDist[i]=vertexAmt;
		hasCalced[i]=false;
	}
	queue<int> candidate;

	geoDist[markV]=0;
	candidate.push(markV);
	hasCalced[markV]=true;
	while(!candidate.empty())
	{
		for(connI=tempGraph[candidate.front()].begin();connI!=tempGraph[candidate.front()].end();connI++)
		{
			if(geoDist[candidate.front()]+1<geoDist[*connI])
			{
				maxGeoDist=geoDist[*connI]=geoDist[candidate.front()]+1;
				if(!hasCalced[*connI])
				{
					candidate.push(*connI);
					hasCalced[*connI]=true;
				}
			}
		}

		candidate.pop();
	}
	//for(int i=1;i<=vertexAmt;i++)
	//{
	//	printf("%d ", geoDist[i]);
	//}

	delete tempGraph;
	delete hasCalced;
}

void ThreeDObj::setControl(const int groupNum, const Mat& transform)
{
	calcLap();
	setControlVIndex(groupNum);	//不能同时有两个control curve变了！
	if(cumuTransMtr.empty())
	{
		cumuTransMtr=transform.clone();
	}
	else
	{
		cumuTransMtr=transform*cumuTransMtr;
	}

	Mat tempV=Mat(4, 1, CV_32FC1);
	for(set<int>::iterator setI=controlVIndex.begin();setI!=controlVIndex.end();setI++)
	{
		tempV.at<float>(0, 0)=vertex[*setI].x;
		tempV.at<float>(1, 0)=vertex[*setI].y;
		tempV.at<float>(2, 0)=vertex[*setI].z;
		tempV.at<float>(3, 0)=1.0f;
		tempV=transform*tempV;
		vertex[*setI].x=tempV.at<float>(0, 0);
		vertex[*setI].y=tempV.at<float>(1, 0);
		vertex[*setI].z=tempV.at<float>(2, 0);
	}
}

void ThreeDObj::deform(void)
{
	calcGeoDist();

	//calculate transformed laplacian value:
	Mat idenMtr(4, 4, CV_32FC1);
	idenMtr.at<float>(0, 0)=idenMtr.at<float>(1, 1)=
		idenMtr.at<float>(2, 2)=idenMtr.at<float>(3, 3)=1.0f;
	Mat transMtr(4, 4, CV_32FC1);
	float transRatio;
	Mat tempLap=Mat(4, 1, CV_32FC1);
	for(int vertexI=1;vertexI<=vertexAmt;vertexI++)
	{
		if(controlVIndex.find(vertexI)==controlVIndex.end())
		{
			transRatio=((float)geoDist[vertexAmt])/maxGeoDist;
		}
		else
		{
			transRatio=0.0f;
		}
		transMtr=cumuTransMtr*(1.0f-transRatio)+idenMtr*transRatio;

		tempLap.at<float>(0, 0)=lapVal[vertexI].x;
		tempLap.at<float>(1, 0)=lapVal[vertexI].y;
		tempLap.at<float>(2, 0)=lapVal[vertexI].z;
		tempLap.at<float>(3, 0)=1.0f;
		tempLap=transMtr*tempLap;
		lapVal[vertexI].x=tempLap.at<float>(0, 0);
		lapVal[vertexI].y=tempLap.at<float>(1, 0);
		lapVal[vertexI].z=tempLap.at<float>(2, 0);
	}
	//for(int i=1;i<=vertexAmt;i++)
	//{
	//	printf("%d: %f %f %f\n", i, lapVal[i].x, lapVal[i].y, lapVal[i].z);
	//}

	//calculate coefficient matrix("A" for Ax=b) and "b" for Ax=b:
	vector<MtrRow> *coeMtr=new vector<MtrRow>[vertexAmt+1];
	for(int i=1;i<=vertexAmt;i++)
	{
		coeMtr[i]=lapMtr[i];
	}
	for(set<int>::iterator iter=controlVIndex.begin();iter!=controlVIndex.end();iter++)
	{
		coeMtr[*iter][0].value+=0.2f;
		lapVal[*iter].x+=0.2f*vertex[*iter].x;
		lapVal[*iter].y+=0.2f*vertex[*iter].y;
		lapVal[*iter].z+=0.2f*vertex[*iter].z;
	}
	//for(int vertexI=1;vertexI<=vertexAmt;vertexI++)
	//{
	//	printf("No.%d row:", vertexI);
	//	for(vector<MtrRow>::iterator iter=coeMtr[vertexI].begin();iter!=coeMtr[vertexI].end();iter++)
	//	{
	//		printf(" %d(%f)", iter->index, iter->value);
	//	}
	//	puts("");
	//}

	//solve x(the new vertex coordinates) of Ax=b
	//delete vertex;
	//vertex=solve(coeMtr, lapVal);
	
	delete coeMtr;
	delete lapVal;
	lapVal=NULL;
}
/*
//just for generating test object
int powOf2(int exp)
{
	int result=2;
	while(exp>1)
	{
		result*=2;
		exp--;
	}
	return result;
}
	
void ThreeDObj::remeshing(void)	//only vertex, no texture
{
	//must have graph to remeshing
	if(graph==NULL)
	{
		buildGraph();
	}

	//delete expired data for consistency:
	if(lapMtr!=NULL)
	{
		vector<MtrRow*>::iterator rowNodeIter;
		for(int vertexI=1;vertexI<=vertexAmt;vertexI++)
		{
			for(rowNodeIter=lapMtr[vertexI].begin(); rowNodeIter!=lapMtr[vertexI].end(); rowNodeIter++)
			{
				delete (*rowNodeIter);
			}
		}
		delete lapMtr;
		lapMtr=NULL;
	}
	delete lapVal;
	lapVal=NULL;

	//add mid point for every edge (vertex)
	map<int, int> *midPoint=new map<int, int>[vertexAmt+1];
	vector<Vertex> newVertex(vertex, vertex+vertexAmt+1);
	Vertex tempV;
	int curNewVertexNum=vertexAmt+1;
	for(int vertexI=1;vertexI<=vertexAmt;vertexI++)
	{
		for(set<int>::iterator connVI=graph[vertexI].begin(); connVI!=graph[vertexI].end();connVI++)
		{
			if(*connVI>vertexI)
			{
				tempV.x=(vertex[*connVI].x+vertex[vertexI].x)/2.0f;
				tempV.y=(vertex[*connVI].y+vertex[vertexI].y)/2.0f;
				tempV.z=(vertex[*connVI].z+vertex[vertexI].z)/2.0f;
				newVertex.push_back(tempV);

				midPoint[vertexI][*connVI]=curNewVertexNum;
				midPoint[*connVI][vertexI]=curNewVertexNum;
				curNewVertexNum++;
			}
		}
	}
	delete vertex;
	vertexAmt=newVertex.size()-1;
	vertex=new Vertex[vertexAmt+1];
	for(int i=1;i<=vertexAmt;i++)
	{
		vertex[i].x=newVertex[i].x;
		vertex[i].y=newVertex[i].y;
		vertex[i].z=newVertex[i].z;
	}

	//cut every triangle into four: (vertex)
	int *newVRIndex=new int[faceAmt*12];
	int newStartPos;
	for(int i=0;i<faceAmt*3;i+=3)
	{
		newStartPos=i*4;
		newVRIndex[newStartPos+8]=newVRIndex[newStartPos+4]=newVRIndex[newStartPos]=
			midPoint[vRenderIndex[i]][vRenderIndex[i+1]];
		newVRIndex[newStartPos+11]=newVRIndex[newStartPos+7]=newVRIndex[newStartPos+1]=
			midPoint[vRenderIndex[i+1]][vRenderIndex[i+2]];
		newVRIndex[newStartPos+10]=newVRIndex[newStartPos+5]=newVRIndex[newStartPos+2]=
			midPoint[vRenderIndex[i+2]][vRenderIndex[i]];
		newVRIndex[newStartPos+3]=vRenderIndex[i];
		newVRIndex[newStartPos+6]=vRenderIndex[i+1];
		newVRIndex[newStartPos+9]=vRenderIndex[i+2];
	}
	delete vRenderIndex;
	faceAmt*=4;
	vRenderIndex=newVRIndex;

	//reset group start render index
	for(int i=0;i<=groupAmt;i++)
	{
		gStartRIndex[i]*=4;
	}

	//delete old graph for consistency:
	delete graph;
	graph=NULL;	
}

void ThreeDObj::write(const char *objFilePath)	//only vertex, no texture
{
	FILE *objFile=fopen(objFilePath, "w");

	for(int i=1;i<=vertexAmt;i++)
	{
		fprintf(objFile, "v %f %f %f\n", vertex[i].x, vertex[i].y, vertex[i].z);
	}

	for(int i=1;i<=texCoordAmt;i++)
	{
		fprintf(objFile, "vt %f %f\n", texCoord[i].u, texCoord[i].v);
	}

	for(int groupI=0;groupI<groupAmt;groupI++)
	{
		fprintf(objFile, "g object%d\n", groupI);
		for(int renderIndex=gStartRIndex[groupI];renderIndex<gStartRIndex[groupI+1];renderIndex+=3)
		{
		//	fprintf(objFile, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", 
		//		vRenderIndex[renderIndex],   tRenderIndex[renderIndex],   vRenderIndex[renderIndex], 
		//		vRenderIndex[renderIndex+1], tRenderIndex[renderIndex+1], vRenderIndex[renderIndex+1],
		//		vRenderIndex[renderIndex+2], tRenderIndex[renderIndex+2], vRenderIndex[renderIndex+2]);
			fprintf(objFile, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", 
				vRenderIndex[renderIndex],   0,   vRenderIndex[renderIndex], 
				vRenderIndex[renderIndex+1], 0, vRenderIndex[renderIndex+1],
				vRenderIndex[renderIndex+2], 0, vRenderIndex[renderIndex+2]);
		}
	}

	fclose(objFile);
}
*/
