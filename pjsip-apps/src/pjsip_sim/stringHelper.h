#pragma once
#include <string>
#include <algorithm>
#include<iostream>
#include<cctype>
#include <functional>



namespace helper
{


	/// <summary>
	/// helper namespace, all methods static final.
	/// 
	/// </summary>
	namespace string 
	{

		static inline std::string &ltrim(std::string &s)
		{
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
				return !std::isspace(ch); }));
			return s;
		}

		static inline std::wstring& ltrim(std::wstring& s)
		{
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
				return !std::isspace(ch); }));
			return s;
		}

		static inline std::string &rtrim(std::string &s)
		{
			s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
				return !std::isspace(ch); }).base(), s.end());
			return s;
		}
		static inline std::wstring& rtrim(std::wstring& s)
		{
			s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
				return !std::isspace(ch); }).base(), s.end());
			return s;
		}

		static inline std::string &trim(std::string &s)
		{
			return ltrim(rtrim(s));
		}
		static inline std::wstring& trim(std::wstring& s)
		{
			return ltrim(rtrim(s));
		}

		static inline bool isStringEmpty(const std::string &attr)
		{
			std::string strAttr = attr;
			if (attr == "" || (trim(strAttr).length() == 0))
			{
				return true;
			}
			return false;
		}

		// relplace string content;
		static inline std::string & replaceString(std::string& str,const std::string& old_value,const std::string& new_value){
			for(std::string::size_type pos = 0;pos!=std::string::npos; pos+=new_value.length())   {   
				if( (pos=str.find(old_value,pos))!=std::string::npos   )   
					str.replace(pos,old_value.length(),new_value);   
				else   
					break;   
			}   
			return   str;  

		}

		//transition a string to upper
		static inline std::string & toUpper(std::string & str)
		{
			transform(str.begin(), str.end(), str.begin(), ::toupper);
			return str;
		}

		static inline std::wstring& toUpper(std::wstring& str)
		{
			transform(str.begin(), str.end(), str.begin(), ::towupper);
			return str;
		}

		//transition a string to lower
		static inline std::string & toLower(std::string &str)
		{
			transform(str.begin(), str.end(), str.begin(), ::tolower);
			return str;
		}

		static inline std::wstring& toLower(std::wstring& str)
		{
			transform(str.begin(), str.end(), str.begin(), ::towlower);
			return str;
		}

		static inline std::vector<std::string> split(const std::string& str, const std::string& pattern)
		{
			std::vector<std::string> ret;
			if (pattern.empty()) return ret;
			size_t start = 0, index = str.find_first_of(pattern, 0);
			while (index != str.npos)
			{
				if (start != index)
					ret.push_back(str.substr(start, index - start));
				start = index + 1;
				index = str.find_first_of(pattern, start);
			}
			if (!str.substr(start).empty())
				ret.push_back(str.substr(start));
			return ret;
		}

		static inline bool startsWith(const std::string& str, const std::string& prefix) {
			return str.size() >= prefix.size() &&
				str.substr(0, prefix.size()) == prefix;
		}
	}
}//end namespace helper
