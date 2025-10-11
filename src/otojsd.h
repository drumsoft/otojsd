#ifndef OTOJSD_H
#define OTOJSD_H
// otojsd - sound processing server for Otojs.

#include <string>
#include <vector>

typedef struct {
	int port;
	bool findfreeport;
	const char *allow_pattern;
	int channel;
	int sample_rate;
	bool verbose;
	const char *output;
	bool enable_input;
	const char *document_root;
	bool level_meter;
} otojsd_options;

#define OTOJSD_DEFAULT_IPMASK "127.0.0.1"

#define OTOJSD_OPTIONS_DEFAULTS {\
	14609,\
	false,\
	OTOJSD_DEFAULT_IPMASK,\
	2,\
	48000,\
	false,\
	NULL,\
	false,\
	NULL,\
	false\
}

void otojsd_start(otojsd_options *options, std::vector<std::string> start_codes, const char *exec_path, char **env);

#endif