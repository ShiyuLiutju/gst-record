/*
 * config.cpp
 *
 *  Created on: May 5, 2017
 *      Author: chengsq
 */

#include "config.h"
#include "json.h"
#include <string>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
//namespace calmcar{
using namespace std;
Config* Config::instance_ =NULL;
Config::Config()
{

}

Config::~Config()
{
	// TODO Auto-generated destructor stub
}

bool Config::Load(string  config_file)
{
	return LoadJson(config_file);
}

bool Config::LoadJson(string config_file)
{
	printf("load json config file.\n");
	Json::Value root;
	Json::Reader reader;
	ifstream ifs(config_file.c_str(),ifstream::binary);
	bool parse_success = reader.parse(ifs, root, false);
	if (!parse_success)
	{
		fprintf(stderr, "Parse %s failed.\n", config_file.c_str());
		printf("failed\n");
		return false;
	}

	Json::Value::Members section_list = root.getMemberNames();
	for (vector<string>::iterator it_section = section_list.begin();
			it_section != section_list.end(); ++it_section)
	{
		Json::Value section = root[it_section->c_str()];
		Json::Value::Members keys = section.getMemberNames();
		for (vector<string>::iterator it_key = keys.begin();
				it_key != keys.end(); ++it_key)
		{
			string key = *it_section + '/' + *it_key;
			string val;
			if (section[*it_key].isInt())
			{
				int flag = section[*it_key].asInt();
				stringstream ss;
				ss << flag;
				ss >> val;
			}
			else
			{
				val = section[*it_key].asString();
			}
			AddEntry(key, val);
		}
	}
	return true;
}

void Config::AddEntry(string key, string value)
{
	content_[key] = AnyConversion(value);
}

AnyConversion const& Config::Value(string const& section,
		string const& entry) const
{

	std::map<string, AnyConversion>::const_iterator ci = content_.find(
			section + '/' + entry);
	if (ci == content_.end())
	{
		printf("ConfigFile::value() error: %s/%s does not exist\n",
				section.c_str(), entry.c_str());
		return AnyConversion("");
	}

	return ci->second;
}
bool Config::KeyExist(string const& section, string const& entry) {
  std::map<string, AnyConversion>::const_iterator ci =
      content_.find(section + '/' + entry);
  return !(ci == content_.end());
}
//}
/* namespace calmcar */

