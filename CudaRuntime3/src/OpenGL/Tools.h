
#include<string>
#include<vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace std;
using namespace glm;

class Tools {
public:
	static string TailToken(string line) {
		int index = line.find_first_of(" ");
		return line.substr(index+1, line.size()-index+1);
	}

	
	static string FirstToken(string line) {
		int index = line.find_first_of(" ");
		return line.substr(0,index);

	}


	static vec3 getElement(vector<vec3>& arr, string pos) {
		
		int num = atoi(pos.c_str())-1;
	/*	if (num < arr.size())*/
			return arr[num];
		//else
		//	return vec3(0, 0, 0);
	}

	static vec2 getElement(vector<vec2>& arr, string pos) {
		int num = atoi(pos.c_str())-1;
		/*if (num < arr.size())*/
			return arr[num];
		//else
		//	return vec2(0, 0);

	}

	static vector<string> split(string str, vector<string>res, string delim) {

		if ("" == str) 
			return res;
		//先将要切割的字符串从string类型转换为char*类型
		char* strs = new char[str.length() + 1]; //不要忘了
		strcpy(strs, str.c_str());

		char* d = new char[delim.length() + 1];
		strcpy(d, delim.c_str());


		char* p = strtok(strs, d);
		while (p) {
			string s = p; //分割得到的字符串转换为string类型
			res.push_back(s); //存入结果数组
			p = strtok(NULL, d);
		}

		return res;
	}

};