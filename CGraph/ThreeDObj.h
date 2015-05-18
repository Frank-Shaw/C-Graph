#pragma once

#include<opencv/cv.h>

#include<set>
#include<vector>

using namespace cv;
using namespace std;


struct Vertex
{
	float x;
	float y;
	float z;
};

typedef Vertex LaplacianVal;

struct TexCoord
{
	float u;
	float v;
};

class MtrRow
{
public:
	int index;
	float value;

	MtrRow(void) {}
	MtrRow(int index, float value): index(index), value(value) {}
	~MtrRow() {}
	void set(int i, float v)
	{
		index=i;
		value=v;
	}
};

class ThreeDObj
{
private:
	int vertexAmt;
	int texCoordAmt;
	int faceAmt;
	int groupAmt;

	Vertex *vertex;
	TexCoord *texCoord;
	int *vRenderIndex;
	int *tRenderIndex;
	int *gStartRIndex;

	Mat texImg;

	void load(const char *objFilePath);

	set<int> *graph;
	vector<MtrRow> *lapMtr;
	LaplacianVal *lapVal;
	int chosenGNum;
	set<int> controlVIndex;
	int maxGeoDist, *geoDist;
	Mat cumuTransMtr;

	void buildGraph(void);
	void computeLapMtr(void);
	void calcLap(void);
	void setControlVIndex(const int groupNum);
	void calcGeoDist(void);

private:
	ThreeDObj(const ThreeDObj&) {}

public:
	ThreeDObj(const char *objFilePath);
	~ThreeDObj(void);
	
	void setControl(const int groupNum, const Mat& transform);
	void deform(void);
	
	//just for generating test object
	//void remeshing(void);
	//void write(const char *objFilePath);

	const int& getVertexAmt(void) const
	{
		return vertexAmt;
	}
	const int& getTexCoordAmt(void) const
	{
		return texCoordAmt;
	}
	const int& getFaceAmt(void) const
	{
		return faceAmt;
	}
	const int& getGroupAmt(void) const
	{
		return groupAmt;
	}

	const Vertex* getVertex(void) const
	{
		return vertex;
	}
	const TexCoord* getTexCoord(void) const
	{
		return texCoord;
	}
	const int* getVRenderIndex(void) const
	{
		return vRenderIndex;
	}
	const int* getTRenderIndex(void) const
	{
		return tRenderIndex;
	}
	const int* getgStartRIndex(void) const
	{
		return gStartRIndex;
	}

	const Mat& getTexImg(void) const
	{
		return texImg;
	}
};
