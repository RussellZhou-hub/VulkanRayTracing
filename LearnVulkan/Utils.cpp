#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <string>
#include "Utils.h"
#include <glm/ext/matrix_transform.hpp>

using namespace std;

string GetExePath()
{
	char szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
	/*
	strrchr:�������ܣ�����һ���ַ�c����һ���ַ���str��ĩ�γ��ֵ�λ�ã�Ҳ���Ǵ�str���Ҳ࿪ʼ�����ַ�c�״γ��ֵ�λ�ã���
	���������λ�õĵ�ַ�����δ���ҵ�ָ���ַ�����ô����������NULL��
	ʹ�������ַ���ش����һ���ַ�c��strĩβ���ַ�����
	*/
	(strrchr(szFilePath, '\\'))[0] = 0; // ɾ���ļ�����ֻ���·���ִ�//
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

glm::vec4& operator+(glm::vec4 a, float* pB)
{
	if (pB == NULL || (pB + 1) == NULL || (pB + 2) == NULL || (pB + 3) == NULL) {
		cout << "glm::vec4& operator-( float* pB,glm::vec4 a):             pB must not be NULL\n";
		return a;
	}
	a.x = pB[0] + a.x;
	a.y = pB[1] + a.y;
	a.z = pB[2] + a.z;
	a.w = pB[3] + a.w;
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

glm::mat4 lookAt(glm::vec4 pos, glm::vec4 posAddForward, glm::vec4 up)
{
	glm::vec3 pos3;
	pos3.x = pos.x;
	pos3.y = pos.y;
	pos3.z = pos.y;

	glm::vec3 posAdd3;
	posAdd3.x = posAddForward.x;
	posAdd3.y = posAddForward.y;
	posAdd3.z = posAddForward.z;

	glm::vec3 up3;
	up3.x = up.x;
	up3.y = up.y;
	up3.z = up.z;

	return glm::lookAtRH(pos3, posAdd3, up3);
}

glm::mat4 lookAt(glm::vec4 pos, glm::vec4 posAddForward, float* up)
{
	glm::vec3 pos3;
	pos3.x = pos.x;
	pos3.y = pos.y;
	pos3.z = pos.y;

	glm::vec3 posAdd3;
	posAdd3.x = posAddForward.x;
	posAdd3.y = posAddForward.y;
	posAdd3.z = posAddForward.z;

	glm::vec3 up3;
	up3.x = up[0];
	up3.y = up[1];
	up3.z = up[2];

	return glm::lookAtRH(pos3, posAdd3, up3);
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


