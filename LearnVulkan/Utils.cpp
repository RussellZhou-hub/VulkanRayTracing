#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <string>
#include "Utils.h"

using namespace std;

string GetExePath()
{
	char szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
	/*
	strrchr:函数功能：查找一个字符c在另一个字符串str中末次出现的位置（也就是从str的右侧开始查找字符c首次出现的位置），
	并返回这个位置的地址。如果未能找到指定字符，那么函数将返回NULL。
	使用这个地址返回从最后一个字符c到str末尾的字符串。
	*/
	(strrchr(szFilePath, '\\'))[0] = 0; // 删除文件名，只获得路径字串//
	string path = szFilePath;
	return path;
}

glm::vec4& operator-(glm::vec4 a, float* pB) {
	if (pB == NULL || (pB + 1) == NULL || (pB + 2) == NULL || (pB + 3) == NULL) {
		std::cout << "glm::vec4& operator-(glm::vec4 a, float* pB):             pB must not be NULL\n";
		return a;
	}
	a.x -= pB[0];
	a.y -= pB[1];
	a.z -= pB[2];
	a.w -= pB[3];
	return a;
}

glm::vec4& operator-(float* pB, glm::vec4 a) {
	if (pB == NULL || (pB + 1) == NULL || (pB + 2) == NULL || (pB + 3) == NULL) {
		cout << "glm::vec4& operator-( float* pB,glm::vec4 a):             pB must not be NULL\n";
		return a;
	}
	a.x = pB[0] - a.x;
	a.y = pB[1] - a.y;
	a.z = pB[2] - a.z;
	a.w = pB[3] - a.w;
	return a;
}

float dot(float* pB, glm::vec4 a) {
	if (pB == NULL || (pB + 1) == NULL || (pB + 2) == NULL || (pB + 3) == NULL) {
		cout << "glm::vec4& operator-( float* pB,glm::vec4 a):             pB must not be NULL\n";
		return 0;
	}
	glm::vec4 b(pB[0], pB[1], pB[2], pB[3]);
	return glm::dot(a, b);
}

//int main_exePathTest(void)
/*
int main()
{
	string str = GetExePath();
	system("pause");
	return 0;
}
*/


