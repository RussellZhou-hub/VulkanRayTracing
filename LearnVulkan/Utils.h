#pragma once
#ifndef UTILS
#define UTILS

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <string>
#include<iostream>

using namespace std;

string GetExePath();

//int main_exePathTest(void)
/*
int main()
{
	string str = GetExePath();
	system("pause");
	return 0;
}
*/

glm::vec4& operator-(glm::vec4 a, float* pB);

glm::vec4& operator+(glm::vec4 a, float* pB);

glm::vec4& operator-(float* pB, glm::vec4 a);

float dot(float* pB, glm::vec4 a);

glm::mat4 lookAt(glm::vec4 pos, glm::vec4 posAddForward, glm::vec4 up);

glm::mat4 lookAt(glm::vec4 pos, glm::vec4 posAddForward, float* up);

#endif