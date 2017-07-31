#include <boost/property_tree/ptree.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <exception>

using namespace std;
using namespace boost::property_tree;
using namespace boost::gregorian;
using namespace boost::posix_time;

struct Config {
    string log_file_name;
    long last_position;
    int lock;
    vector<string> urls;
    void load(const char *f);
    void save(const char *f);
};

void Config::load(const char *f) {
    ptree pt;
    read_json(f, pt);
    log_file_name = pt.get<string>("log_file_name");
    last_position = pt.get<long>("last_position");
    lock = pt.get<long>("lock");
    
    BOOST_FOREACH(ptree::value_type &v, pt.get_child("urls")) {
	urls.push_back(v.second.data());
    }
}

void Config::save(const char *f) {
    ptree pt;
    ptree pt_urls;

    for (vector<string>::iterator itr = urls.begin(); itr != urls.end() ; ++itr) {
	pt_urls.push_back(make_pair("", *itr));
    }

    pt.put("log_file_name", log_file_name);
    pt.put("last_position", last_position);
    pt.put("lock", lock);
    pt.add_child("urls", pt_urls);

    write_json(f, pt);
}

ifstream::pos_type get_file_size(const char *f) {
    ifstream in(f, ifstream::ate);
    return in.tellg();
}

string get_last_minute() {
    return to_iso_extended_string(second_clock::local_time() - minutes(1)).substr(0, 16);
}

long get_timestamp() {
    return (second_clock::local_time() - ptime(date(1970, 1, 1))).total_seconds();
}

int main (int argc, char *argv[]) {
    if (argc != 3) {
	cout << "Usage : " << argv[0] << " configuration output" << endl;
	return 1;
    }

    const char *config_name = argv[1];
    Config conf;
    conf.load(config_name);

    if (1 == conf.lock) {
	cout << argv[0] << " already started!" << endl;
	return 0;
    }

    ifstream ifs(conf.log_file_name.c_str());
    if (!ifs.good()) {
	cout << conf.log_file_name << " can not be read!" << endl;
	return 2;
    }

    conf.lock = 1;
    conf.save(config_name);

    long file_size = get_file_size(conf.log_file_name.c_str());
    long start_position = conf.last_position > 0L && conf.last_position < file_size ? conf.last_position : 0L;
    
    string last_minute = get_last_minute();
    string line;
    map<pair<string, string>, vector<int> > calc_map;

    for (vector<string>::iterator itr = conf.urls.begin(); itr != conf.urls.end(); ++itr) {
	for (int i = 1; i <= 5; ++i) {
	    ostringstream ss;
	    ss << i;
	    calc_map.insert(make_pair(make_pair(*itr, ss.str()), vector<int>(2, 0)));
	}
    }

    for (ifs.seekg(start_position, ios::beg); getline(ifs, line) ; conf.last_position = ifs.tellg()) {
	istringstream iss(line);
	ptree pt;
	try {
	    read_json(iss, pt);
	} catch (exception &e) {
	    cout << "processing [" << line << "] faild." << endl;
	    cout << e.what() << endl;
	    continue;
	}
	string ts = pt.get<string>("time_iso8601").substr(0, 16);

	if (ts < last_minute) {
	    continue;
	}

	if (ts > last_minute) {
	    break;
	}

	string request = pt.get<string>("request");
	
	string url_key = "";
	for (vector<string>::iterator itr = conf.urls.begin(); itr != conf.urls.end(); ++itr) {
	    if (itr->length() <= request.length() && request.substr(0, itr->length()) == *itr) {
		url_key = *itr;
	    }
	}
	if ("" == url_key) {
	    continue;
	}
	string status_prefix = pt.get<string>("status").substr(0, 1);
	int request_time = pt.get<double>("request_time") * 1000;

	pair<string, string> key(url_key, status_prefix);
	if (calc_map.find(key) == calc_map.end()) {
	    cout << "unexpected line [" << line << "]." << endl;
	    continue;
	}

	calc_map[key][0] += 1; // count
	calc_map[key][1] += request_time; // sum
    }
    
    ptree opt;
    opt.put("ts", last_minute);
    for (map<pair<string, string>, vector<int> >::iterator itr = calc_map.begin();
    	itr != calc_map.end(); ++itr) {
        string key_prefix = itr->first.first + ":" + itr->first.second + "xx:";
        opt.put(key_prefix + "cnt", itr->second[0]);
        opt.put(key_prefix + "avg", boost::format("%.2f") % ((double)itr->second[1] / (itr->second[0] > 0 ? itr->second[0] : 1)));
    }
    write_json(argv[2], opt);

    conf.lock = 0;
    conf.save(config_name);
}
