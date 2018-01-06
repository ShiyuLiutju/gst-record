/*
 * config.h
 *
 *  Created on: May 5, 2017
 *      Author: chengsq
 */

#ifndef SRC_UTIL_CONFIG_H_
#define SRC_UTIL_CONFIG_H_

#include "anyconversion.h"
#include <string>
#include <map>
#include <vector>
struct configData
{
    char *default_ip;
    int default_port;
    int camera_number;
    string uri_v4l2;
    int default_bitrate;
    int default_fps;
    string u_dir;
    char* camera_open;
    std::vector<std::string> group;

};
//namespace calmcar{
class Config
{
public:
	virtual ~Config();

	public:
	  bool Load(string configFile);
	  AnyConversion const& Value(string const& section, string const& entry) const;

	  bool KeyExist(string const& section, string const& entry);
	  static Config* Instance() {
	    if (!instance_) instance_ = new Config();
	    return instance_;
	  }

	 private:
	  std::map<std::string, AnyConversion> content_;
	  Config();
	  static Config* instance_;
	  void DumpValues();
	  void AddEntry(string key, string value);
	  bool LoadJson(string configFile);
};
//}/* namespace calmcar */

#endif /* SRC_UTIL_CONFIG_H_ */
